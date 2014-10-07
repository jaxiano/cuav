//=============================================================================
// Copyright © 2008 Point Grey Research, Inc. All Rights Reserved.
//
// This software is the confidential and proprietary information of Point
// Grey Research, Inc. ("Confidential Information").  You shall not
// disclose such Confidential Information and shall use it only in
// accordance with the terms of the license agreement you entered into
// with PGR.
//
// PGR MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF THE
// SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE, OR NON-INFRINGEMENT. PGR SHALL NOT BE LIABLE FOR ANY DAMAGES
// SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR DISTRIBUTING
// THIS SOFTWARE OR ITS DERIVATIVES.
//=============================================================================
//=============================================================================
// $Id: FlyCapture2Test_C.c,v 1.29 2010/04/13 21:35:02 hirokim Exp $
//=============================================================================

#if defined(WIN32) || defined(WIN64)
#define _CRT_SECURE_NO_WARNINGS		
#endif

#include <stdio.h>
#include <unistd.h>
#include "C/FlyCapture2_C.h"
#include "fleatest_lib.h"

void SetTimeStamping( fc2Context context, BOOL enableTimeStamp )
{
    fc2Error error;
    fc2EmbeddedImageInfo embeddedInfo;

    error = fc2GetEmbeddedImageInfo( context, &embeddedInfo );
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2GetEmbeddedImageInfo: %d\n", error );
    }

    if ( embeddedInfo.timestamp.available != 0 )
    {       
        embeddedInfo.timestamp.onOff = enableTimeStamp;
    }    

    error = fc2SetEmbeddedImageInfo( context, &embeddedInfo );
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2SetEmbeddedImageInfo: %d\n", error );
    }
}


void PrintImageInfo( fc2Image* img)
{
    printf("\n***Image Information***\n"
        "Rows - %u\n"
        "Cols - %u\n"
        "DataSize - %u\n"
        "ReceivedDataSize - %u\n"
        "PixelFormat - %d\n"
        "BayerFormat - %d\n",
        img->rows,
        img->cols,
        img->dataSize,
        img->receivedDataSize,
        img->format,
        img->bayerFormat);

}

void PrintCameraInfo( fc2Context context )
{
    fc2Error error;
    fc2CameraInfo camInfo;
    error = fc2GetCameraInfo( context, &camInfo );
    if ( error != FC2_ERROR_OK )
    {
        // Error
    }

    printf(
        "\n*** CAMERA INFORMATION ***\n"
        "Serial number - %u\n"
        "Camera model - %s\n"
        "Camera vendor - %s\n"
        "Sensor - %s\n"
        "Resolution - %s\n"
        "Firmware version - %s\n"
        "Firmware build time - %s\n\n",
        camInfo.serialNumber,
        camInfo.modelName,
        camInfo.vendorName,
        camInfo.sensorInfo,
        camInfo.sensorResolution,
        camInfo.firmwareVersion,
        camInfo.firmwareBuildTime );
}


fleaCamera open_camera()
{
    fc2Error error;
    fleaCamera camera;
    fc2PGRGuid guid;
    fc2TriggerMode trigger_mode;
    //fc2VideoMode vm;
    //fc2FrameRate fr;
    //BOOL supported;
    unsigned int trigger_address = 0x62c;
    unsigned int trigger_value;
    int retry = 0;

    printf("Creating context");
    error = fc2CreateContext( &camera.context );
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2CreateContext: %d\n", error );
        return camera;
    }        

    // Get the 0th camera
    fc2GetCameraFromIndex( camera.context, 0, &guid );
printf("Getting camera");
    
    error = fc2Connect( camera.context, &guid );
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2Connect: %d\n", error );
        return camera;
    }

    PrintCameraInfo( camera.context );  
    SetTimeStamping( camera.context, TRUE );      
/*    
    error = fc2GetVideoModeAndFrameRate(context, &vm, &fr);

    error = fc2GetVideoModeAndFrameRateInfo(context, FC2_VIDEOMODE_1600x1200RGB, FC2_FRAMERATE_1_875, &supported);

    if (supported)
    {
        vm = FC2_VIDEOMODE_1600x1200Y8;
        fr = FC2_FRAMERATE_1_875;
        error = fc2SetVideoModeAndFrameRate(context, vm, fr);
        if ( error != FC2_ERROR_OK )
        {
            printf( "Error in fc2SetVideModeAndFrameRate: %d\n", error );
        }
    } else {
        printf("Requested Video Mode not supported\n");
    }
*/
    error = fc2GetTriggerMode(camera.context, &trigger_mode);
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2GetTriggerMode: %d\n", error );
    }
    trigger_mode.onOff = 1;
    trigger_mode.mode = 0;
    trigger_mode.parameter = 0;
    trigger_mode.source = 7; //software trigger

    error = fc2SetTriggerMode(camera.context, &trigger_mode);
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2SetTriggerMode: %d\n", error );
    }
    
    do
    {
        sleep(1);
        fc2ReadRegister(camera.context, trigger_address, &trigger_value);
    } 
    while (((trigger_value >> 31) != 0) && (retry++ < 15));
    if (retry >= 15) printf("trigger not ready\n");
    

    error = fc2StartCapture( camera.context );
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2StartCapture: %d\n", error );
    }


    return camera;
}

int trigger(fleaCamera camera)
{
    fc2Error error;
    fc2Image rawImage;
    fc2Image convertedImage;
    fc2TimeStamp prevTimestamp = {0};
    fc2TimeStamp ts;
    int diff;
    char filename[80];

    error = fc2CreateImage( &rawImage );
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2CreateImage: %d\n", error );
    }

    error = fc2CreateImage( &convertedImage );
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2CreateImage: %d\n", error );
    }


    error = fc2FireSoftwareTrigger( camera.context );
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2FireSoftwareTrigger: %d\n", error);
        return error;
    }

    // Retrieve the image
    error = fc2RetrieveBuffer( camera.context, &rawImage );
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in retrieveBuffer: %d\n", error);
        return error;
    }

    // Get and print out the time stamp
    ts = fc2GetImageTimeStamp( &rawImage);
    diff = (ts.cycleSeconds - prevTimestamp.cycleSeconds) * 8000
                + (ts.cycleCount - prevTimestamp.cycleCount);
    prevTimestamp = ts;
    printf( 
        "timestamp [%d %d] - %d\n", 
        ts.cycleSeconds, 
        ts.cycleCount, 
        diff );

    PrintImageInfo(&rawImage);
    // Convert the final image to RGB
    error = fc2ConvertImageTo(FC2_PIXEL_FORMAT_RGB, &rawImage, &convertedImage);
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2ConvertImageTo: %d\n", error );
    }

    // Save it to PNG
    sprintf(filename, "fc2TestImage%d %d.pgm", ts.cycleSeconds, ts.cycleCount);
    printf("Saving the last image to %s \n", filename);
    PrintImageInfo(&convertedImage);
    
	error = fc2SaveImage( &rawImage, filename, FC2_PGM );
	//error = fc2SaveImage( &convertedImage, filename, FC2_PNG );
	if ( error != FC2_ERROR_OK )
	{
		printf( "Error in fc2SaveImage: %d\n", error );
		printf( "Please check write permissions.\n");
	}

    error = fc2DestroyImage( &rawImage );
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2DestroyImage: %d\n", error );
    }

    error = fc2DestroyImage( &convertedImage );
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2DestroyImage: %d\n", error );
    }

    return 0;
}

int capture()
{
    return 0;
}

int flea_close(fleaCamera camera)
{
    fc2Error error;
    error = fc2StopCapture( camera.context );
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2StopCapture: %d\n", error );
    }

    error = fc2DestroyContext( camera.context );
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2DestroyContext: %d\n", error );
    }
    return error;
}

void save_pgm(const char* filename, fc2Image img)
{
}

void save_file(const char* filename, char* bytes)
{
}

