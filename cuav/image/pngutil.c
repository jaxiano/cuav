#include <png.h>
#include "include/pngutil.h"
#include "include/scanner.h"

int read_chunk_callback(png_structp ptr, png_unknown_chunkp chunk)
{
	if(!strncmp((char *) chunk->name, "mISb", 5)){
		png_unknown_chunkp myChunkPtr = (png_unknown_chunkp) png_get_user_chunk_ptr(ptr);
		if((myChunkPtr != 0) && (chunk->size <= myChunkPtr->size)){
			strncpy((char *)myChunkPtr->name, (char *)chunk->name, 5);
			myChunkPtr->size = chunk->size;
			myChunkPtr->location = chunk->location;
			memcpy(myChunkPtr->data, chunk->data, chunk->size);
			return 1;
		}
	}
	return 0;
}

unsigned char* doConvertRGB8(png_structp PngPtr, png_infop InfoPtr)
{
	return 0;
}

unsigned char* doConvertRGBA8(png_structp PngPtr, png_infop InfoPtr)
{
	return 0;
}

unsigned char* doConvertGray8(png_structp PngPtr, png_infop InfoPtr)
{
	return 0;
}

unsigned char* doConvertGrayA8(png_structp PngPtr, png_infop InfoPtr)
{
	return 0;
}

unsigned char* doConvertGray16(png_structp PngPtr, png_infop InfoPtr)
{
	int width = png_get_image_width(PngPtr, InfoPtr);
	int height = png_get_image_height(PngPtr, InfoPtr);
	int size = width * height * 3;
	printf("width:%i, height:%i, size:%i\n",width,height,size);

	unsigned char *bgr = (unsigned char *)malloc(size);

	png_bytep *row_pointers = png_get_rows(PngPtr, InfoPtr);
	int pos = 0;
	int count = 0;
	unsigned long long SumSq = 0;
	unsigned long long Sum = 0;

	for(int i=0; i<height; i++){
		for(int j=0; j<width*2; j+=2){
			unsigned short pix = (row_pointers[i][j] << 8) + row_pointers[i][j+1];
			Sum += pix;
			SumSq += pix*pix;
			count++;
		}
	}

	if(count == 0)
		count = 1;

	double meanD = Sum/((double)count);
	double varianceD = (double)(SumSq/((double)(count-1)) - (meanD*meanD));
	varianceD = MAX(0.0, varianceD);
	double stdDev = sqrt(varianceD);
	double spread = 2.0f*stdDev;

	unsigned short currentMax = 0;
	unsigned short currentMin = 0xFFFF;

	double peak = meanD + spread;
	peak = MIN(65535.0, peak);
	peak = MAX(0.0, peak);
	currentMax = (unsigned short)peak;

	double clip = meanD - spread;
	clip = MIN(65535.0, clip);
	clip = MAX(0.0, clip);
	currentMin = (unsigned short)clip;

	for(int i=0; i<height; i++){
		for(int j=0; j<width*2; j+=2){
			unsigned short pix = (row_pointers[i][j] << 8) + row_pointers[i][j+1];
			pix = MIN(currentMax, pix);
			pix = MAX(currentMin, pix);
			unsigned long pixScale = ((pix - currentMin)*255)/(currentMax - currentMin);
			bgr[pos++] = (unsigned char)pixScale;	// blue
			bgr[pos++] = (unsigned char)pixScale;	// green
			bgr[pos++] = (unsigned char)pixScale;	// red
		}
	}

	return bgr;
}

unsigned char* doExtractCanonicalData(png_structp PngPtr, png_infop InfoPtr)
{
	int color_type = png_get_color_type(PngPtr, InfoPtr);
	int bit_depth = png_get_bit_depth(PngPtr, InfoPtr);

	switch(color_type){
		case PNG_COLOR_TYPE_RGB:
			return doConvertRGB8(PngPtr, InfoPtr);
			break;
		case PNG_COLOR_TYPE_RGB_ALPHA:
			return doConvertRGBA8(PngPtr, InfoPtr);
			break;
		case PNG_COLOR_TYPE_GRAY:
			if(bit_depth == 8)
				return doConvertGray8(PngPtr, InfoPtr);
			else if(bit_depth == 16)
				return doConvertGray16(PngPtr, InfoPtr);
			break;
		case PNG_COLOR_TYPE_GRAY_ALPHA:
			return doConvertGrayA8(PngPtr, InfoPtr);
			break;
	}

	return 0;
}

