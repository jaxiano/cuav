#ifndef FLEA_LIB_H
#define FLEA_LIB_H

#include "C/FlyCapture2_C.h"


typedef struct _fleaCamera
{
    fc2Context  context;
} fleaCamera;


fleaCamera* open_camera();
int trigger(fleaCamera* camera);
int capture(fleaCamera* camera, void* image_buf, float* frame_time);
int close_camera(fleaCamera* camera);

#endif
