/*
 * pic: high level picture I/O routines
 *
 * Michael Garland	Jan 1996	written
 *
 * Paul Heckbert	Oct 1998	modified
 *   compile normally and you don't need libtiff (good for only PGM/PPM)
 *   compile with -DPIC_TIFF_ENABLE -ltiff if you want TIFFs
 */

#include <stdio.h>
#include <string.h>
#include "include/pic.h"

/*
 * pic_alloc: allocate picture memory.
 * If opic!=0, then memory from opic->pix is reused (after checking that
 *    size is sufficient), else a new pixel array is allocated.
 * Great caution should be used when freeing, if pix memory is reused!
 */
Pic *pic_alloc(int nx, int ny, int bytes_per_pixel, Pic *opic) {
    Pic *p;
    int size = ny*nx*bytes_per_pixel;

    ALLOC(p, Pic, 1);
    p->nx = nx;
    p->ny = ny;
    p->bpp = bytes_per_pixel;
    if (opic && opic->nx*opic->ny*opic->bpp >= p->nx*p->ny*p->bpp) {
		p->pix = opic->pix;
		/* now opic and p have a common pix array */
    }
    else
    	ALLOC(p->pix, Pixel1, size);
    return p;
}

void pic_free(Pic *p) {
    free(p->pix);
    free(p);
}



/*
 * pic_xxx routines will call pnm_xxx or tiff_xxx depending on the
 * type of file.
 *
 * Currently, this is rather inefficient.  We open the file, check the
 * magic number, close it, and then reopen for the actual operation.
 * It would probably be good to add a further level of interface which
 * allowed us to pass around file descriptors in addition to file names.
 *
 * Michael Garland      17 Jan 96
 *
 */

Pic_file_format pic_file_type(char *file)
{
    unsigned char byte1, byte2;

    FILE *pic = fopen(file, "r");
    if( !pic )
	return PIC_UNKNOWN_FILE;

    byte1 = getc(pic);
    byte2 = getc(pic);

    fclose(pic);

    if( byte1=='P' && byte2>='1' && byte2<='6' )
    	return PIC_PNM_FILE;
    else if( (byte1==0x4d && byte2==0x4d) || (byte1==0x49 && byte2==0x49) )
    	return PIC_TIFF_FILE;
    else
    	return PIC_UNKNOWN_FILE;
}

Pic_file_format pic_filename_type(char *file)
{
    char *suff;

    suff = strrchr(file, '.');
    if (!strcmp(suff, ".tiff") || !strcmp(suff, ".tif")) return PIC_TIFF_FILE;
    if (!strcmp(suff, ".pgm") || !strcmp(suff, ".ppm")) return PIC_PNM_FILE;
    return PIC_UNKNOWN_FILE;
}

int pic_get_size(char *file, int *nx, int *ny)
{
    switch( pic_file_type(file) )
    {
		case PIC_TIFF_FILE:
	#	ifdef PIC_TIFF_ENABLE
			return tiff_get_size(file, nx, ny);
	#	else
			printf("pic was compiled with TIFF disabled\n");
			return FALSE;
	#	endif
		break;

		case PIC_PNM_FILE:
			return pnm_get_size(file, nx, ny);
		break;

		default:
			return FALSE;
    }
}

/*
 * pic_read: read a TIFF or PNM file into memory.
 * Normally, you should use opic==NULL.
 * If opic!=NULL, then picture is read into opic->pix (after checking that
 * size is sufficient), else a new Pic is allocated.
 * Returns Pic pointer on success, NULL on failure.
 */
Pic *pic_read(char *file, Pic *opic)
{
    switch( pic_file_type(file) )
    {
		case PIC_TIFF_FILE:
		#	ifdef PIC_TIFF_ENABLE
				return tiff_read(file, opic);
		#	else
				printf("pic was compiled with TIFF disabled\n");
				return NULL;
		#	endif
			break;

		case PIC_PNM_FILE:
			return pnm_read(file, opic);
			break;

		default:
			return NULL;
    }

}

/*
 * pic_write: write pic to file in the specified format
 * returns TRUE on success, FALSE on failure
 */
int pic_write(char *file, Pic *pic, Pic_file_format format)
{
    switch( format )
    {
		case PIC_TIFF_FILE:
		#	ifdef PIC_TIFF_ENABLE
				return tiff_write(file, pic);
		#	else
				printf("pic was compiled with TIFF disabled\n");
				return FALSE;
		#	endif
			break;

		case PIC_PNM_FILE:
			return pnm_write(file, pic);
			break;

		default:
			fprintf(stderr, "pic_write: can't write %s, unknown format\n", file);
			return FALSE;
    }
}
