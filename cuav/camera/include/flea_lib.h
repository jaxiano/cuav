#ifndef FLEA_LIB_H
#define FLEA_LIB_H

#include "C/FlyCapture2_C.h"

typedef enum _imageResolution
{
    RES_1280x960,
    RES_1920x1080,
    RES_2448x2048
} enum_imageResolution;

typedef struct _fleaCamera
{
    fc2Context  context;
    int useTrigger;
    enum_imageResolution resolution;
} fleaCamera;


fleaCamera* open_camera();
int trigger(fleaCamera* camera);
int capture(fleaCamera* camera, void* image_buf, float* frame_time);
int close_camera(fleaCamera* camera);

#endif
