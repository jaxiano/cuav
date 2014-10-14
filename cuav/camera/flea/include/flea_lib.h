#ifndef FLEA_LIB_H
#define FLEA_LIB_H

#include "C/FlyCapture2_C.h"


typedef struct _fleaCamera
{
    fc2Context  context;
} fleaCamera;

fleaCamera open_camera();
int trigger(fleaCamera camera);
int capture();
int flea_close(fleaCamera camera);
void save_pgm(const char* filename, fc2Image img);
void save_file(const char* filename, char* bytes);

#endif
