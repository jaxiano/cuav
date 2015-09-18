#ifndef FLEA_LIB_H
#define FLEA_LIB_H

int open_camera(unsigned int height, unsigned int width);
void capture(char *filename);
void close_camera();

#endif
