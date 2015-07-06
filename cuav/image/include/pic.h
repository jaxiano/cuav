#ifndef PIC_H
#define PIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#ifndef ALLOC
#define ALLOC(ptr, type, n) { \
    ptr = (type *)malloc((n)*sizeof(type)); \
    if (!ptr) { \
	fprintf(stderr, "pic_alloc: Can't allocate %d bytes of memory, aborting\n", n); \
	exit(1); \
    } \
}
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

typedef unsigned char Pixel1;	/* 1-byte pixel */
typedef struct {Pixel1 r, g, b;}	Pixel1_rgb;
typedef short                           Pixel2;
typedef struct {Pixel2 r, g, b;}        Pixel2_rgb;
typedef Pixel2_rgb Rgbcolor;


typedef struct {		/* PICTURE */
    int nx, ny;			/* width & height, in pixels */
    int bpp;			/* bytes per pixel = 1, 3, or 4 */
    Pixel1 *pix;		/* array of pixels */
				/* data is in row-major order,
				    i.e. it has the same memory layout as:
				    if 1 byte per pixel,  then array[ny][nx]
				    if 3 bytes per pixel, then array[ny][nx][3]
				    if 4 bytes per pixel, then array[ny][nx][4] */
} Pic;

/* the following macro is used to read or write individual pixel components */

#define PIC_PIXEL(pic, x, y, chan) \
    (pic)->pix[((y)*(pic)->nx+(x))*(pic)->bpp+(chan)]
    /* returns channel chan of pixel (x,y) of picture pic */
    /* usually chan=0 for red, 1 for green, 2 for blue */

typedef enum {PIC_TIFF_FILE, PIC_PNM_FILE, PIC_UNKNOWN_FILE} Pic_file_format;

/*----------------------Allocation routines--------------------------*/
extern Pic *pic_alloc(int nx, int ny, int bytes_per_pixel, Pic *opic);
extern void pic_free(Pic *p);

/*------------------------- I/O routines ----------------------------*/
extern int tiff_get_size(char *file, int *nx, int *ny);
extern Pic *tiff_read(char *file, Pic *opic);
extern int tiff_write(char *file, Pic *pic);

extern int pnm_get_size(char *file, int *nx, int *ny);
extern Pic *pnm_read(char *file, Pic *opic);
extern int pnm_write(char *file, Pic *pic);

extern int pic_get_size(char *file, int *nx, int *ny);
extern Pic *pic_read(char *file, Pic *opic);
extern int pic_write(char *file, Pic *pic, Pic_file_format format);
extern Pic_file_format pic_file_type(char *file);
extern Pic_file_format pic_filename_type(char *file);

#ifdef __cplusplus
}
#endif
#endif
