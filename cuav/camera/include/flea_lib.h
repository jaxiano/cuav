#ifndef FLEA_LIB_H
#define FLEA_LIB_H

#include "C/FlyCapture2_C.h"


typedef struct _fleaCamera
{
    fc2Context  context;
    unsigned int height;
    unsigned int width;
} fleaCamera;


void SetTimeStamping( fc2Context context, BOOL enableTimeStamp );
void PrintPropertyInfo( fc2PropertyInfo* prop);
void PrintProperty( fc2Property* prop);
void PrintImageInfo( fc2Image* img);
void PrintCameraInfo( fc2Context context );
void chkProperty(fleaCamera* camera, fc2PropertyType type);
void set_property(fleaCamera* camera, fc2PropertyType type, int value);
void set_gamma(fleaCamera* camera, int value);
void set_brightness(fleaCamera* camera, int value);


fleaCamera* open_camera(int brightness, unsigned int height, unsigned int width);
int trigger(fleaCamera* camera);
int capture(fleaCamera* camera, void* image_buf, float* frame_time);
int close_camera(fleaCamera* camera);

#endif
