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
#include "flea_lib.h"

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

void PrintFormat7Settings( fc2Format7ImageSettings* img, uint packet_size, float percentage)
{
    printf("\n***Format 7 Image Settings***\n"
        "Packet size - %u\n"
        "percentage - %f\n"
        "Width - %u\n"
        "Height - %u\n"
        "offsetX - %u\n"
        "offsetY - %u\n"
        "PixelFormat - %d\n"
        "mode  - %d\n",
        packet_size,
        percentage,
        img->width,
        img->height,
        img->offsetX,
        img->offsetY,
        img->pixelFormat,
        img->mode);
}

void PrintFormat7Info( fc2Format7Info* img)
{
    printf("\n***Format 7 Image Info***\n"
        "maxWidth - %u\n"
        "maxHeight - %u\n"
        "offsetHStepSize - %u\n"
        "offsetVStepSize - %u\n"
        "imageHStepSize - %u\n"
        "imageVStepSize - %u\n"
        "PixelFormatBitField - %u\n"
        "VendorPixelFormatBitField - %u\n"
        "packetSize - %u\n"
        "minPacketSize - %u\n"
        "maxPacketSize - %u\n"
        "mode  - %d\n",
        img->maxWidth,
        img->maxHeight,
        img->offsetHStepSize,
        img->offsetVStepSize,
        img->imageHStepSize,
        img->imageVStepSize,
        img->pixelFormatBitField,
        img->vendorPixelFormatBitField,
        img->packetSize,
        img->minPacketSize,
        img->maxPacketSize,
        img->mode);
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

fleaCamera* open_camera_gigE()
{
        fc2Error error;
        fleaCamera* camera = calloc(1, sizeof(fleaCamera));
        fc2PGRGuid guid;
        fc2TriggerMode trigger_mode;
        fc2GigEImageSettings image_settings;
    
        printf("Creating context\n");
	camera->useTrigger = 0;
        error = fc2CreateGigEContext( &camera->context );
        if ( error != FC2_ERROR_OK )
        {
            printf( "Error in fc2CreateContext: %d\n", error );
            free(camera);
            return NULL;
        }        
    
        // Get the 0th camera
        fc2GetCameraFromIndex( camera->context, 0, &guid );
        
        error = fc2Connect( camera->context, &guid );
        if ( error != FC2_ERROR_OK )
        {
            printf( "Error in fc2Connect: %d\n", error );
            close_camera(camera);
            return NULL;
        }
    
        //PrintCameraInfo( camera->context );  
        SetTimeStamping( camera->context, TRUE );      
        error = fc2GetGigEImageSettings(camera->context, &image_settings);
        if ( error != FC2_ERROR_OK )
        {
            printf( "Error getting image settings settings: %d\n", error );
            return NULL;
        }
    
    
        image_settings.width = 2448;
        image_settings.height = 2048;
        image_settings.offsetX = 0;
        image_settings.offsetY = 0;
        image_settings.pixelFormat = FC2_PIXEL_FORMAT_RAW8;
    
        error = fc2SetGigEImageSettings(camera->context, &image_settings);
        if ( error != FC2_ERROR_OK )
        {
            printf( "Error setting format7 settings: %d\n", error );
            return NULL;
        }
        sleep(0.5);
    
        error = fc2StartCapture( camera->context );
        if ( error != FC2_ERROR_OK )
        {
            printf( "Error in fc2StartCapture: %d\n", error );
        }
    
        sleep(0.5);

    return camera;
}



fleaCamera* open_camera_f7()
{
    fc2Error error;
    fleaCamera* camera = calloc(1, sizeof(fleaCamera));
    fc2PGRGuid guid;
    fc2TriggerMode trigger_mode;
    fc2Format7ImageSettings image_settings;
    fc2Format7PacketInfo packet_info;
    BOOL supported;
    uint packet_size = 0;
    float percentage = 0.0;

    camera->useTrigger = 0;
    printf("Creating context\n");
    error = fc2CreateContext( &camera->context );
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2CreateContext: %d\n", error );
        free(camera);
        return NULL;
    }        

    // Get the 0th camera
    fc2GetCameraFromIndex( camera->context, 0, &guid );
    
    error = fc2Connect( camera->context, &guid );
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2Connect: %d\n", error );
        close_camera(camera);
        return NULL;
    }

    //PrintCameraInfo( camera->context );  
    SetTimeStamping( camera->context, TRUE );      

    error = fc2GetFormat7Configuration(camera->context, &image_settings, &packet_size, &percentage);
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error getting format7 settings: %d\n", error );
        return NULL;
    }
    //PrintFormat7Settings(&image_settings, packet_size, percentage);

    image_settings.width = 1280;
    image_settings.height = 960;
    packet_info.recommendedBytesPerPacket = 1400;
    packet_info.maxBytesPerPacket = 2400;
    packet_info.unitBytesPerPacket = 1400;
    error = fc2ValidateFormat7Settings(camera->context, &image_settings, &supported, &packet_info);

    if (supported)
    {
        printf("format7 settings validated\n");
        
        error = fc2SetFormat7Configuration(camera->context, &image_settings, 100.0);
        if ( error != FC2_ERROR_OK )
        {
            printf( "Error setting format7 settings: %d\n", error );
            return NULL;
        }
  
    } else
    {
        printf("Settings Not Validated");
    }

    //PrintFormat7Settings(&image_settings, packet_size, percentage);
/*    error = fc2GetTriggerMode(camera->context, &trigger_mode);
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2GetTriggerMode: %d\n", error );
    }
    trigger_mode.onOff = 1;
    trigger_mode.mode = 0;
    trigger_mode.parameter = 0;
    trigger_mode.source = 7; //software trigger

    error = fc2SetTriggerMode(camera->context, &trigger_mode);
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2SetTriggerMode: %d\n", error );
    }
    
*/
    error = fc2StartCapture( camera->context );
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2StartCapture: %d\n", error );
    }


    return camera;
}

int trigger(fleaCamera* camera)
{
    fc2Error error;

    if (!camera->useTrigger) return 0;

    error = fc2FireSoftwareTrigger( camera->context );
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2FireSoftwareTrigger: %d\n", error);
        return -1;
    }
    return 0;
}

int capture(fleaCamera* camera, void* image_buf, float* frame_time)
{
    fc2Error error;
    fc2Image rawImage;
    fc2TimeStamp ts;

    error = fc2CreateImage( &rawImage );
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2CreateImage: %d\n", error );
    }

    // Retrieve the image
    error = fc2RetrieveBuffer( camera->context, &rawImage );
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in retrieveBuffer: %d\n", error);
        return -1;
    }

    // Get and print out the time stamp
    ts = fc2GetImageTimeStamp( &rawImage);
    (*frame_time) = (ts.cycleSeconds * 8000) + ts.cycleCount;
/*
    diff = (ts.cycleSeconds - prevTimestamp.cycleSeconds) * 8000
                + (ts.cycleCount - prevTimestamp.cycleCount);
    prevTimestamp = ts;
    printf( 
        "timestamp [%d %d] - %d\n", 
        ts.cycleSeconds, 
        ts.cycleCount, 
        diff );
*/
    //PrintImageInfo(&rawImage);
    memcpy(image_buf, rawImage.pData, rawImage.dataSize);

    error = fc2DestroyImage( &rawImage );
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2DestroyImage: %d\n", error );
    }
    return 0;
}

int close_camera(fleaCamera* camera)
{
    fc2Error error;
    error = fc2StopCapture( camera->context );
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2StopCapture: %d\n", error );
    }

    error = fc2DestroyContext( camera->context );
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2DestroyContext: %d\n", error );
    }
    
    free(camera);
    return 0;
}

