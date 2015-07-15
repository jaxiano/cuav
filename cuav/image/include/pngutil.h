/*
 * pngutil.h
 *
 *  Created on: Jun 19, 2015
 *      Author: johnchu
 */

#ifndef PNGUTIL_H_
#define PNGUTIL_H_

unsigned char* doExtractCanonicalData(png_structp PngPtr, png_infop InfoPtr);
unsigned char* doConvertGray16(png_structp PngPtr, png_infop InfoPtr);
unsigned char* doConvertGrayA8(png_structp PngPtr, png_infop InfoPtr);
unsigned char* doConvertGray8(png_structp PngPtr, png_infop InfoPtr);
unsigned char* doConvertRGBA8(png_structp PngPtr, png_infop InfoPtr);
unsigned char* doConvertRGB8(png_structp PngPtr, png_infop InfoPtr);
int read_chunk_callback(png_structp ptr, png_unknown_chunkp chunk);

#endif /* PNGUTIL_H_ */
