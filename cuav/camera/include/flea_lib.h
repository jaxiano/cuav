#ifndef FLEA_LIB_H
#define FLEA_LIB_H

#include "C/FlyCapture2_C.h"


typedef struct _fleaCamera
{
    fc2Context  context;
    unsigned int height;
    unsigned int width;
} fleaCamera;

typedef struct _fleaProperty
{
    int autoMode;
    int on;
    float value;
} fleaProperty;

void SetTimeStamping( fc2Context context, BOOL enableTimeStamp );
void PrintPropertyInfo( fc2PropertyInfo* prop);
void PrintProperty( fc2Property* prop);
void PrintImageInfo( fc2Image* img);
void PrintCameraInfo( fc2Context context );
void chkProperty(fleaCamera* camera, fc2PropertyType type);
fc2Property get_property(fleaCamera* camera, fc2PropertyType type);
fleaProperty get_property_as_flea(fleaCamera* camera, fc2PropertyType type);
void set_property_value(fleaCamera* camera, fc2PropertyType type, float value);
void set_gamma(fleaCamera* camera, float value);
void set_brightness(fleaCamera* camera, float value);
void set_exposure(fleaCamera* camera, int autoMode, int onOff, float value);
void set_shutter(fleaCamera* camera, int autoMode, int onOff, float value);
void set_gain(fleaCamera* camera, int autoMode, int onOff, float value);

//exposure, shutter, and gain have auto controls they have more knobs
fleaProperty get_exposure(fleaCamera* camera);
fleaProperty get_shutter(fleaCamera* camera);
fleaProperty get_gain(fleaCamera* camera);

//brightness and gamma are just int values
//  dumbing it down for the library wrapper
float get_brightness(fleaCamera* camera);
float get_gamma(fleaCamera* camera);


fleaCamera* open_camera(int brightness, unsigned int height, unsigned int width);
int trigger(fleaCamera* camera);
int capture(fleaCamera* camera, void* image_buf, float* frame_time);
int close_camera(fleaCamera* camera);

#endif
