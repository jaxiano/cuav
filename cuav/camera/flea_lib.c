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
#include "include/flea_lib.h"

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

void PrintPropertyInfo( fc2PropertyInfo* prop)
{
    printf("\n***Property Information***\n"
        "Type - %d\n"
        "min - %u\n"
        "max - %u\n",
        prop->type,
        prop->min,
        prop->max);
}

void PrintProperty( fc2Property* prop)
{
    printf("\n***Property Value***\n"
        "Type - %d\n"
        "Value - %u\n",
        prop->type,
        prop->valueA);
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

fleaCamera* open_camera(int brightness, unsigned int height, unsigned int width)
{
        fc2Error error;
        fleaCamera* camera = calloc(1, sizeof(fleaCamera));
        fc2PGRGuid guid;
        fc2GigEImageSettings image_settings;
    
        printf("Creating context\n");
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
    
        set_property(camera, FC2_BRIGHTNESS, brightness);
        //PrintCameraInfo( camera->context );  
        //setup_camera( camera );
        SetTimeStamping( camera->context, TRUE );      
        error = fc2GetGigEImageSettings(camera->context, &image_settings);
        if ( error != FC2_ERROR_OK )
        {
            printf( "Error getting image settings settings: %d\n", error );
            return NULL;
        }
    
       
        image_settings.width = width;
        image_settings.height = height;
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


void chkProperty(fleaCamera* camera, fc2PropertyType type)
{
    fc2PropertyInfo info;
    fc2Property prop;

    info.type = type;
    fc2GetPropertyInfo(camera->context, &info);
    PrintPropertyInfo(&info);
    
    prop.type = type;
    fc2GetProperty(camera->context, &prop);
    PrintProperty(&prop);
    
}

void set_property(fleaCamera* camera, fc2PropertyType type, int value)
{
    fc2Property prop;
    prop.type = type;
    fc2GetProperty(camera->context, &prop);
    prop.valueA = value;
    fc2SetProperty(camera->context, &prop);    

}

void set_gamma(fleaCamera* camera, int value)
{
    set_property(camera, FC2_GAMMA, value);
}

void set_brightness(fleaCamera* camera, int value)
{
    set_property(camera, FC2_BRIGHTNESS, value);
}


int setup_camera(fleaCamera* camera)
{
    
    chkProperty(camera, FC2_BRIGHTNESS);
    chkProperty(camera, FC2_GAMMA);
    chkProperty(camera, FC2_FRAME_RATE);
    chkProperty(camera, FC2_GAIN);
    chkProperty(camera, FC2_AUTO_EXPOSURE);
    
}



int trigger(fleaCamera* camera)
{
/*** We're staying out of trigger mode for now...
    fc2Error error;

    if (!camera->useTrigger) return 0;

    error = fc2FireSoftwareTrigger( camera->context );
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2FireSoftwareTrigger: %d\n", error);
        return -1;
    }
*/
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

    error = fc2Disconnect( camera->context );
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2Disconnect: %d\n", error );
    }

    error = fc2DestroyContext( camera->context );
    if ( error != FC2_ERROR_OK )
    {
        printf( "Error in fc2DestroyContext: %d\n", error );
    }
    
    free(camera);
    return 0;
}


