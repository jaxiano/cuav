/*
  scan an image for regions of unusual colour values
  Andrew Tridgell, October 2011
 */

#ifndef scanner_h
	#include <png.h>
	#include "include/scanner.h"
	#include "include/pngutil.h"
#endif

#if SHOW_TIMING
struct timeval tp1,tp2;

static void start_timer()
{
	gettimeofday(&tp1,NULL);
}

static double end_timer()
{
	gettimeofday(&tp2,NULL);
	return((tp2.tv_sec - tp1.tv_sec) + 
	       (tp2.tv_usec - tp1.tv_usec)*1.0e-6);
}
#endif // SHOW_TIMING

/*
  save a bgr image as a P6 pnm file
 */
static bool colour_save_pnm(const char *filename, const struct bgr_image *image)
{
	FILE *fd;
	fd = fopen(filename, "wb");
	if (fd == NULL) return false;

    /*
      PNM P6 is in RGB format not BGR format
     */
    struct bgr_image *rgb = allocate_bgr_image8(image->height, image->width, NULL);
    uint16_t x, y;
    for (y=0; y<rgb->height; y++) {
        for (x=0; x<rgb->width; x++) {
            rgb->data[y][x].r = image->data[y][x].b;
            rgb->data[y][x].g = image->data[y][x].g;
            rgb->data[y][x].b = image->data[y][x].r;
                }
        }
        
	char header[64];
	snprintf(header, sizeof(header), "P6\n%u %u\n255\n", image->width, image->height);
	if (fwrite(header, strlen(header), 1, fd) != 1) {
        free(rgb);
		fclose(fd);
		return false;                
    }
    size_t size = image->width*image->height*sizeof(struct bgr);
	if (fwrite(&rgb->data[0][0], size, 1, fd) != 1) {
        free(rgb);
		fclose(fd);
		return false;
	}
    free(rgb);
	fclose(fd);
	return true;
}

static bool colour_save_pnm_bgr(const char *filename, const struct bgr *bgr, uint16_t width, uint16_t height)
{
	FILE *fd;
	fd = fopen(filename, "wb");
	if (fd == NULL) return false;

    /*
      PNM P6 is in RGB format not BGR format
     */
	char header[64];
	snprintf(header, sizeof(header), "P6\n%u %u\n255\n", width, height);
	if (fwrite(header, strlen(header), 1, fd) != 1) {
		fclose(fd);
		return false;
    }
    size_t size = width*height*sizeof(struct bgr);
	if (fwrite(bgr, size, 1, fd) != 1) {
		fclose(fd);
		return false;
	}
	fclose(fd);
	return true;
}

static bool colour_save_pnm_grey(const char *filename, const uint8_t *grey, uint16_t width, uint16_t height)
{
	FILE *fd;
	fd = fopen(filename, "wb");
	if (fd == NULL) return false;

    /*
      PNM P6 is in RGB format not BGR format
     */
	char header[64];
	snprintf(header, sizeof(header), "P5\n%u %u\n255\n", width, height);
	if (fwrite(header, strlen(header), 1, fd) != 1) {
		fclose(fd);
		return false;
    }
    size_t size = width*height*sizeof(uint8_t);
	if (fwrite(grey, size, 1, fd) != 1) {
		fclose(fd);
		return false;
	}
	fclose(fd);
	return true;
}

/*
  roughly convert a 8 bit colour chameleon image to colour at half
  the resolution. No smoothing is done
 */
static void colour_convert_half(const struct grey_image8 *in, struct bgr_image *out)
{
	unsigned x, y;
	/*
	  layout in the input image is in blocks of 4 values. The top
	  left corner of the image looks like this
             G B
	     R G
	 */
        assert(in->width/2 == out->width);
        assert(in->height/2 == out->height);

	for (y=0; y<out->height; y++) {
		for (x=0; x<out->width; x++) {
			out->data[y][x].g = (in->data[y*2+0][x*2+0] + 
					     (uint16_t)in->data[y*2+1][x*2+1]) / 2;
			out->data[y][x].b = in->data[y*2+0][x*2+1];
			out->data[y][x].r = in->data[y*2+1][x*2+0];
		}
	}
}


/*
  convert a 8 bit colour chameleon image to 8 bit colour at full
  resolution. No smoothing is done

  This algorithm emphasises speed over colour accuracy
 */
static void colour_convert(const struct grey_image8 *in, struct bgr_image *out)
{
	unsigned x, y;
	/*
	  layout in the input image is in blocks of 4 values. The top
	  left corner of the image looks like this
             G B G B
	     R G R G
	     G B G B
	     R G R G
	 */
	for (y=1; y<out->height-2; y += 2) {
		for (x=1; x<out->width-2; x += 2) {
			out->data[y+0][x+0].g = in->data[y][x];
			out->data[y+0][x+0].b = ((uint16_t)in->data[y-1][x+0] + (uint16_t)in->data[y+1][x+0]) >> 1;
			out->data[y+0][x+0].r = ((uint16_t)in->data[y+0][x-1] + (uint16_t)in->data[y+0][x+1]) >> 1;

			out->data[y+0][x+1].g = ((uint16_t)in->data[y+0][x+0] + (uint16_t)in->data[y-1][x+1] +
						 (uint16_t)in->data[y+0][x+2] + (uint16_t)in->data[y+1][x+1]) >> 2;
			out->data[y+0][x+1].b = ((uint16_t)in->data[y-1][x+0] + (uint16_t)in->data[y-1][x+2] +
						 (uint16_t)in->data[y+1][x+0] + (uint16_t)in->data[y+1][x+2]) >> 2;
			out->data[y+0][x+1].r = in->data[y+0][x+1];

			out->data[y+1][x+0].g = ((uint16_t)in->data[y+0][x+0] + (uint16_t)in->data[y+1][x-1] +
						 (uint16_t)in->data[y+1][x+1] + (uint16_t)in->data[y+2][x+0]) >> 2;
			out->data[y+1][x+0].b = in->data[y+1][x+0];
			out->data[y+1][x+0].r = ((uint16_t)in->data[y+0][x-1] + (uint16_t)in->data[y+0][x+1] +
						 (uint16_t)in->data[y+2][x-1] + (uint16_t)in->data[y+2][x+1]) >> 2;

			out->data[y+1][x+1].g = in->data[y+1][x+1];
			out->data[y+1][x+1].b = ((uint16_t)in->data[y+1][x+0] + (uint16_t)in->data[y+1][x+2]) >> 1;
			out->data[y+1][x+1].r = ((uint16_t)in->data[y+0][x+1] + (uint16_t)in->data[y+2][x+1]) >> 1;
		}
		out->data[y+0][0] = out->data[y+0][1];
		out->data[y+1][0] = out->data[y+1][1];
		out->data[y+0][out->width-1] = out->data[y+0][out->width-2];
		out->data[y+1][out->width-1] = out->data[y+1][out->width-2];
	}
	memcpy(out->data[0], out->data[1], out->width*3);
	memcpy(out->data[out->height-1], out->data[out->height-2], out->width*3);
}


/*
  convert a 24 bit BGR colour image to 8 bit bayer grid

  this is used by the fake chameleon code
 */
static void rebayer_1280_960_8(const struct bgr_image *in, struct grey_image8 *out)
{
	unsigned x, y;
	/*
	  layout in the input image is in blocks of 4 values. The top
	  left corner of the image looks like this
             G B
	     R G
	 */
	for (y=1; y<in->height-1; y += 2) {
		for (x=1; x<in->width-1; x += 2) {
			// note that this is used with images from
			// opencv which are BGR, whereas we normally
			// use BGR, so we reverse R and B in the
			// conversion
			out->data[y+0][x+0] = in->data[y][x].g;
			out->data[y+0][x+1] = in->data[y][x].r;
			out->data[y+1][x+0] = in->data[y][x].b;
			out->data[y+1][x+1] = in->data[y][x].g;
		}
	}
}


#define HISTOGRAM_BITS_PER_COLOR 3
#define HISTOGRAM_BITS (3*HISTOGRAM_BITS_PER_COLOR)
#define HISTOGRAM_BINS (1<<HISTOGRAM_BITS)

struct histogram {
	uint32_t count[(1<<HISTOGRAM_BITS)];
};


#ifdef __ARM_NEON__
static void NOINLINE get_min_max_neon(const struct bgr * __restrict in, 
				      uint32_t size,
				      struct bgr *min, 
				      struct bgr *max)
{
	const uint8_t *src;
	uint32_t i;
	uint8x8_t rmax, rmin, gmax, gmin, bmax, bmin;
	uint8x8x3_t bgr;

	rmin = gmin = bmin = vdup_n_u8(255);
	rmax = gmax = bmax = vdup_n_u8(0);

	src = (const uint8_t *)in;
	for (i=0; i<size/8; i++) {
		bgr = vld3_u8(src);
		bmin = vmin_u8(bmin, bgr.val[0]);
		bmax = vmax_u8(bmax, bgr.val[0]);
		gmin = vmin_u8(gmin, bgr.val[1]);
		gmax = vmax_u8(gmax, bgr.val[1]);
		rmin = vmin_u8(rmin, bgr.val[2]);
		rmax = vmax_u8(rmax, bgr.val[2]);
		src += 8*3;
	}

	min->r = min->g = min->b = 255;
	max->r = max->g = max->b = 0;
	/*
	  we split this into 3 parts as gcc 4.8.1 on ARM runs out of
	  registers and gives a spurious const error if we leave it as
	  one chunk
	 */
	for (i=0; i<8; i++) {
		if (min->b > vget_lane_u8(bmin, i)) min->b = vget_lane_u8(bmin, i);
		if (min->g > vget_lane_u8(gmin, i)) min->g = vget_lane_u8(gmin, i);
	}
	for (i=0; i<8; i++) {
		if (min->r > vget_lane_u8(rmin, i)) min->r = vget_lane_u8(rmin, i);
		if (max->b < vget_lane_u8(bmax, i)) max->b = vget_lane_u8(bmax, i);
	}
	for (i=0; i<8; i++) {
		if (max->g < vget_lane_u8(gmax, i)) max->g = vget_lane_u8(gmax, i);
		if (max->r < vget_lane_u8(rmax, i)) max->r = vget_lane_u8(rmax, i);
	}
}
#endif

/*
  find the min and max of each color over an image. Used to find
  bounds of histogram bins
 */
static void get_min_max(const struct bgr * __restrict in, 
			uint32_t size,
			struct bgr *min, 
			struct bgr *max)
{
	uint32_t i;

	min->r = min->g = min->b = 255;
	max->r = max->g = max->b = 0;

	for (i=0; i<size; i++) {
		const struct bgr *v = &in[i];
		if (v->b < min->b) min->b = v->b;
		if (v->g < min->g) min->g = v->g;
		if (v->r < min->r) min->r = v->r;
		if (v->b > max->b) max->b = v->b;
		if (v->g > max->g) max->g = v->g;
		if (v->r > max->r) max->r = v->r;
	}	
}


/*
  quantise an BGR image
 */
static void quantise_image(const struct scan_params *scan_params, 
                           const struct bgr *in,
			   uint32_t size,
			   struct bgr *out,
			   const struct bgr *min, 
			   const struct bgr *bin_spacing)
{
	unsigned i;
	uint8_t btab[0x100], gtab[0x100], rtab[0x100];

	for (i=0; i<0x100; i++) {
		btab[i] = (i - min->b) / bin_spacing->b;
		gtab[i] = (i - min->g) / bin_spacing->g;
		rtab[i] = (i - min->r) / bin_spacing->r;
		if (btab[i] >= (1<<HISTOGRAM_BITS_PER_COLOR)) {
			btab[i] = (1<<HISTOGRAM_BITS_PER_COLOR)-1;
		}
		if (gtab[i] >= (1<<HISTOGRAM_BITS_PER_COLOR)) {
			gtab[i] = (1<<HISTOGRAM_BITS_PER_COLOR)-1;
		}
		if (rtab[i] >= (1<<HISTOGRAM_BITS_PER_COLOR)) {
			rtab[i] = (1<<HISTOGRAM_BITS_PER_COLOR)-1;
		}
	}

	for (i=0; i<size; i++) {
        if (scan_params->blue_emphasis) {
			if (in[i].b > in[i].r+5 && in[i].b > in[i].g+5) {
				// emphasise blue pixels. This works well for
				// some terrain types
				out[i].b = (1<<HISTOGRAM_BITS_PER_COLOR)-1;
				out[i].g = 0;
				out[i].r = 0;
				continue;
			}
		}
		out[i].b = btab[in[i].b];
		out[i].g = gtab[in[i].g];
		out[i].r = rtab[in[i].r];
	}
}

static bool is_zero_bgr(const struct bgr *v)
{
	return v->r == 0 && v->g == 0 && v->b == 0;
}


/*
  unquantise an BGR image, useful for visualising the effect of
  quantisation by restoring the original colour ranges, which makes
  the granularity of the quantisation very clear visually
 */
static void unquantise_image(const struct bgr_image *in,
			     struct bgr_image *out,
			     const struct bgr *min, 
			     const struct bgr *bin_spacing)
{
	unsigned x, y;

	for (y=0; y<in->height; y++) {
		for (x=0; x<in->width; x++) {
			const struct bgr *v = &in->data[y][x];
			if (is_zero_bgr(v)) {
                            out->data[y][x] = *v;
                        } else {
                            out->data[y][x].r = (v->r * bin_spacing->r) + min->r;
                            out->data[y][x].g = (v->g * bin_spacing->g) + min->g;
                            out->data[y][x].b = (v->b * bin_spacing->b) + min->b;
                        }
		}
	}

}

/*
  calculate a histogram bin for a bgr value
 */
static inline uint16_t bgr_bin(const struct bgr *in)
{
	return (in->r << (2*HISTOGRAM_BITS_PER_COLOR)) |
		(in->g << (HISTOGRAM_BITS_PER_COLOR)) |
		in->b;
}

/*
  build a histogram of an image
 */
static void build_histogram(const struct bgr *in,
			    uint32_t size,
			    struct histogram *out)
{
	unsigned i;

	memset(out->count, 0, sizeof(out->count));

	for (i=0; i<size; i++) {
		uint16_t b = bgr_bin(&in[i]);
		out->count[b]++;
	}	
}

/*
  threshold an image by its histogram. Pixels that have a histogram
  count of more than the given threshold are set to zero value
 */
/*static void histogram_threshold(struct bgr_image *in,
				const struct histogram *histogram,
				unsigned threshold)
{
	unsigned x, y;

	for (y=0; y<in->height; y++) {
		for (x=0; x<in->width; x++) {
			struct bgr *v = &in->data[y][x];
			uint16_t b = bgr_bin(v);
			if (histogram->count[b] > threshold) {
				v->r = v->g = v->b = 0;
			}
		}
	}	
}*/

/*
  threshold an image by its histogram, Pixels that have a histogram
  count of more than threshold are set to zero value. 

  This also zeros pixels which have a directly neighboring colour
  value which is above the threshold. That makes it much more
  expensive to calculate, but also makes it much less susceptible to
  edge effects in the histogram
 */
static void histogram_threshold_neighbours(const struct bgr *in,
					   uint32_t size,
					   struct bgr *out,
					   const struct histogram *histogram,
					   const struct scan_params *scan_params)
{
	uint32_t i;

	uint8_t red_threshold = 1<< HISTOGRAM_BITS_PER_COLOR;
	uint8_t green_threshold = 1<< HISTOGRAM_BITS_PER_COLOR;
	uint8_t blue_threshold = 1<< HISTOGRAM_BITS_PER_COLOR;

	for (i=0; i<size; i++) {
		struct bgr v = in[i];
		int8_t rofs, gofs, bofs;

		if (histogram->count[bgr_bin(&v)] > scan_params->histogram_count_threshold) {
			goto zero;
		}

		for (rofs=-1; rofs<= 1; rofs++) {
			for (gofs=-1; gofs<= 1; gofs++) {
				for (bofs=-1; bofs<= 1; bofs++) {
					struct bgr v2 = { .b=v.b+bofs, .g=v.g+gofs, .r=v.r+rofs };
					if(scan_params->infra_red){
						if(v2.r <= scan_params->infra_red_min_threshold || v2.r >= scan_params->infra_red_max_threshold){
							goto zero;
						}
						continue;
					}
					if (v2.r >= red_threshold ||
					    v2.g >= green_threshold ||
					    v2.b >= blue_threshold) {
						continue;
					}
					if (histogram->count[bgr_bin(&v2)] > scan_params->histogram_count_threshold) {
						goto zero;
					}
				}
			}
		}
		out[i] = in[i];

		if(scan_params->infra_red){
			out[i].r = (1<<HISTOGRAM_BITS_PER_COLOR) - 1;
			out[i].b = out[i].g = 0;
		}
		continue;
	zero:
		out[i].b = out[i].g = out[i].r = 0;
	}	
}

static void histogram_neighbors(const struct scan_params *scan_params,
                             const struct bgr_image *in, struct bgr_image *out,
                             struct bgr_image *quantised,
                             struct histogram *histogram,
                             char *saveDir)
{
	struct bgr min, max;
	struct bgr bin_spacing;
	unsigned num_bins = (1<<HISTOGRAM_BITS_PER_COLOR);

#ifdef __ARM_NEON__
	get_min_max_neon(&in->data[0][0], in->width*in->height, &min, &max);
#else
	get_min_max(&in->data[0][0], in->width*in->height, &min, &max);
#endif

	bin_spacing.r = 1 + (max.r - min.r) / num_bins;
	bin_spacing.g = 1 + (max.g - min.g) / num_bins;
	bin_spacing.b = 1 + (max.b - min.b) / num_bins;

	struct bgr_image *neighbours = allocate_bgr_image8(in->height, in->width, NULL);

	struct bgr_image *unquantised = NULL;
    if (scan_params->save_intermediate) {
            unquantised = allocate_bgr_image8(in->height, in->width, NULL);
    }

	printf("histogram threshold\n");
	histogram_threshold_neighbours(&quantised->data[0][0], in->width*in->height,
				   &neighbours->data[0][0], histogram, scan_params);


	if (scan_params->save_intermediate) {
			unquantise_image(neighbours, unquantised, &min, &bin_spacing);
			char filepath[256] = {0};
			strcpy(filepath, saveDir);
			strcat(filepath, "1-neighbours.pnm");
			colour_save_pnm(filepath, unquantised);
			free(unquantised);
	}

	copy_bgr_image8(neighbours, out);

	free(neighbours);
}

static void histogram_quantization(const struct scan_params *scan_params,
                             const struct bgr_image *in,
                             struct bgr_image *quantised,
                             struct histogram *histogram,
                             char *saveDir)
{
	struct bgr min, max;
	struct bgr bin_spacing;
	unsigned num_bins = (1<<HISTOGRAM_BITS_PER_COLOR);

#ifdef __ARM_NEON__
	get_min_max_neon(&in->data[0][0], in->width*in->height, &min, &max);
#else
	get_min_max(&in->data[0][0], in->width*in->height, &min, &max);
#endif

#if 0
        printf("red %u %u  green %u %u  blue %u %u\n",
               min.r, max.r,
               min.g, max.g,
               min.b, max.b);
#endif

#if 0
	struct bgr min2, max2;
	if (!bgr_equal(&min, &min2) ||
	    !bgr_equal(&max, &max2)) {
		printf("get_min_max_neon failure\n");
	}
#endif


	bin_spacing.r = 1 + (max.r - min.r) / num_bins;
	bin_spacing.g = 1 + (max.g - min.g) / num_bins;
	bin_spacing.b = 1 + (max.b - min.b) / num_bins;

#if 0
	// try using same spacing on all axes
	if (bin_spacing.r < bin_spacing.g) bin_spacing.r = bin_spacing.g;
	if (bin_spacing.r < bin_spacing.b) bin_spacing.r = bin_spacing.b;
	bin_spacing.g = bin_spacing.r;
	bin_spacing.b = bin_spacing.b;
#endif


	if (scan_params->save_intermediate) {
		printf("saving 0-rgb.pnm to %s\n", saveDir);
		char filepath[256] = {0};
		strcpy(filepath, saveDir);
		strcat(filepath, "0-rgb.pnm");
		printf("--- path: %s\n", filepath);
		colour_save_pnm(filepath, in);
	}

	printf("quantize image\n");
	quantise_image(scan_params, &in->data[0][0], in->width*in->height, 
                       &quantised->data[0][0], &min, &bin_spacing);

/*	if (scan_params->save_intermediate) {
			unquantise_image(quantised, unquantised, &min, &bin_spacing);
			char filepath[256] = {0};
			strcpy(filepath, saveDir);
			strcat(filepath, "unquantised.pnm");
			colour_save_pnm(filepath, unquantised);
	}*/

	printf("build histogram\n");
	build_histogram(&quantised->data[0][0], in->width*in->height, histogram);

/*	if (scan_params->save_intermediate) {
			copy_bgr_image8(quantised, qsaved);
			histogram_threshold(quantised, histogram, scan_params->histogram_count_threshold);
			unquantise_image(quantised, unquantised, &min, &bin_spacing);
			char filepath[256] = {0};
			strcpy(filepath, saveDir);
			strcat(filepath, "thresholded.pnm");
			colour_save_pnm(filepath, unquantised);
			copy_bgr_image8(qsaved, quantised);
	}*/
}

static void colour_histogram(const struct scan_params *scan_params,
                             const struct bgr_image *in, struct bgr_image *out,
                             struct bgr_image *quantised,
                             struct histogram *histogram,
                             char *saveDir)
{
	histogram_quantization(scan_params, in, quantised, histogram, saveDir);
	histogram_neighbors(scan_params, in, out, quantised, histogram, saveDir);
}

#define REGION_UNKNOWN -2
#define REGION_NONE -1

/*
  find a region number for a pixel by looking at the surrounding pixels
  up to scan_params.region_merge
 */
static unsigned find_region(const struct scan_params *scan_params, 
                            const struct bgr_image *in, struct regions *out,
			    int y, int x)
{
	int yofs, xofs;
        uint16_t m = MAX(1, scan_params->region_merge/10);

	/*
	  we only need to look up or directly to the left, as this function is used
	  from assign_regions() where we scan from top to bottom, left to right
	 */
	for (yofs=-m; yofs <= 0; yofs++) {
		for (xofs=-m; xofs <= m; xofs++) {
                        if (yofs+y < 0) continue;
                        if (xofs+x < 0) continue;
                        if (xofs+x >= in->width) continue;

			if (out->data[y+yofs][x+xofs] >= 0) {
				return out->data[y+yofs][x+xofs];
			}
		}
	}
	for (xofs=-m; xofs < 0; xofs++) {
		if (xofs+x < 0) continue;
		if (out->data[y][x+xofs] >= 0) {
			return out->data[y][x+xofs];
		}
	}
	return REGION_NONE;
}

/*
  assign region numbers to contigouus regions of non-zero data in an
  image
 */
static void assign_regions(const struct scan_params *scan_params, 
                           const struct bgr_image *in, struct regions *out)
{
	unsigned x, y;

	out->num_regions = 0;
	memset(out->region_size, 0, sizeof(out->region_size));
	memset(out->bounds, 0, sizeof(out->bounds));
	for (y=0; y<in->height; y++) {
		for (x=0; x<in->width; x++) {
			out->data[y][x] = REGION_UNKNOWN;
		}
	}

	for (y=0; y<in->height; y++) {
		for (x=0; x<in->width; x++) {
			if (out->data[y][x] != REGION_UNKNOWN) {
				/* already assigned a region */
				continue;
			}
			if (is_zero_bgr(&in->data[y][x])) {
				out->data[y][x] = REGION_NONE;
				continue;
			}

			if (out->num_regions == MAX_REGIONS) {
				return;
			}

			unsigned r;
			r = find_region(scan_params, in, out, y, x);
			if (r == REGION_NONE) {
				/* a new region */
				r = out->num_regions;
				out->num_regions++;
				out->bounds[r].minx = x;
				out->bounds[r].maxx = x;
				out->bounds[r].miny = y;
				out->bounds[r].maxy = y;
				out->region_size[r] = 1;
			} else {
				struct bgr c = in->data[y][x];

				/* an existing region */
				out->bounds[r].minx = MIN(out->bounds[r].minx, x);
				out->bounds[r].miny = MIN(out->bounds[r].miny, y);
				out->bounds[r].maxx = MAX(out->bounds[r].maxx, x);
				out->bounds[r].maxy = MAX(out->bounds[r].maxy, y);
				out->region_size[r] =
					(1+out->bounds[r].maxx - out->bounds[r].minx) *
					(1+out->bounds[r].maxy - out->bounds[r].miny);
			}

			out->data[y][x] = r;
		}
	}	
}

/*
  remove a region
 */
static void remove_region(struct regions *in, unsigned i)
{
	if (i < in->num_regions-1) {
			memmove(&in->region_size[i], &in->region_size[i+1],
					sizeof(in->region_size[i])*(in->num_regions-(i+1)));
			memmove(&in->bounds[i], &in->bounds[i+1],
					sizeof(in->bounds[i])*(in->num_regions-(i+1)));
			memmove(&in->region_score[i], &in->region_score[i+1],
					sizeof(in->region_score[i])*(in->num_regions-(i+1)));
	}
	in->num_regions--;
}

/*
  determine if two regions overlap, taking into account the
  region_merge size
 */
static bool regions_overlap(const struct scan_params *scan_params,
                            const struct region_bounds *r1, const struct region_bounds *r2)
{
    uint16_t m = scan_params->region_merge;
    if (r1->maxx+m < r2->minx) return false;
    if (r2->maxx+m < r1->minx) return false;
    if (r1->maxy+m < r2->miny) return false;
    if (r2->maxy+m < r1->miny) return false;
    return true;
}

/*
  merge regions that overlap
 */
static void merge_regions(const struct scan_params *scan_params, struct regions *in)
{
	unsigned i, j;
        bool found_overlapping = true;
        while (found_overlapping) {
                found_overlapping = false;
                for (i=0; i<in->num_regions; i++) {
                        for (j=i+1; j<in->num_regions; j++) {
                            if (regions_overlap(scan_params, &in->bounds[i], &in->bounds[j])) {
                                        struct region_bounds *b1 = &in->bounds[i];
                                        struct region_bounds *b2 = &in->bounds[j];
                                        struct region_bounds b3 = in->bounds[i];  
                                        b3.minx = MIN(b1->minx, b2->minx);
                                        b3.maxx = MAX(b1->maxx, b2->maxx);
                                        b3.miny = MIN(b1->miny, b2->miny);
                                        b3.maxy = MAX(b1->maxy, b2->maxy);
                                        unsigned new_size = (1+b3.maxx - b3.minx) * (1+b3.maxy - b3.miny);
                                        if ((new_size <= scan_params->max_region_area &&
                                             (b3.maxx - b3.minx) <= scan_params->max_region_size_xy &&
                                             (b3.maxy - b3.miny) <= scan_params->max_region_size_xy) ||
                                            in->num_regions>20) {
                                            *b1 = b3;
                                            // new size is sum of the
                                            // two regions, not
                                            // area. This prevents two
                                            // single pixel regions
                                            // appearing to be large enough
                                            in->region_size[i] += in->region_size[j];
                                            remove_region(in, j);
                                            j--;
                                            found_overlapping = true;
                                        }
                            }
                        }
                }
        }
}

static bool region_too_large(const struct scan_params *scan_params, struct regions *in, unsigned i)
{
	uint32_t xlen = in->bounds[i].maxx - in->bounds[i].minx;
	uint32_t ylen = in->bounds[i].maxy - in->bounds[i].miny;
    if ((1+xlen)*(1*ylen) > scan_params->max_region_area ||
        xlen > scan_params->max_region_size_xy ||
        ylen > scan_params->max_region_size_xy) {
        return true;
    }
    return false;
}

/*
  remove any too large regions
 */
static void prune_large_regions(const struct scan_params *scan_params, struct regions *in)
{
	unsigned i;
	for (i=0; i<in->num_regions; i++) {
            if (region_too_large(scan_params, in, i)) {
#if 0
                    printf("prune1 size=%u xsize=%u ysize=%u range=(min:%u,max:%u,minxy:%u,maxxy:%u)\n",
                           in->region_size[i], 
                           in->bounds[i].maxx - in->bounds[i].minx,
                           in->bounds[i].maxy - in->bounds[i].miny,
                           scan_params->min_region_area, 
                           scan_params->max_region_area,
                           scan_params->min_region_size_xy, 
                           scan_params->max_region_size_xy);
#endif
                    remove_region(in, i);
                    i--;
            }
        }
}

/*
  remove any too small regions
 */
static void prune_small_regions(const struct scan_params *scan_params, struct regions *in)
{
	unsigned i;
	for (i=0; i<in->num_regions; i++) {
		uint32_t xlen = in->bounds[i].maxx - in->bounds[i].minx;
		uint32_t ylen = in->bounds[i].maxy - in->bounds[i].miny;
		if ((1+xlen)*(1+ylen) < scan_params->min_region_area ||
		    xlen < scan_params->min_region_size_xy ||
		    ylen < scan_params->min_region_size_xy) {
#if 0
                        printf("prune2 size=%u xsize=%u ysize=%u range=(min:%u,max:%u,minxy:%u,maxxy:%u)\n",
                               in->region_size[i], 
                               in->bounds[i].maxx - in->bounds[i].minx,
                               in->bounds[i].maxy - in->bounds[i].miny,
                               scan_params->min_region_area, 
                               scan_params->max_region_area,
                               scan_params->min_region_size_xy, 
                               scan_params->max_region_size_xy);
#endif
                        remove_region(in, i);
			i--;
		}
		    
	}
}

static void prune_low_score_regions(struct scan_params *scan_params, struct regions *in)
{

	if(scan_params->adaptive_rarity){
		if(in->num_regions > 0){
			uint32_t score = 0;
			for(int i=0; i<in->num_regions; i++){
				score += in->region_score[i];
			}

			// floor to the lowest int.
			// Ex: 695 -> 600
			scan_params->min_region_score = score/in->num_regions;
			scan_params->min_region_score = (scan_params->min_region_score/100)*100;
			printf("Calculated MinRegionScore: %i\n", scan_params->min_region_score);
		}
	}

	printf("prune_low_score_regions:\n");
    if(scan_params->min_region_score > 0){
    	printf("  MinRegionScore: %i\n", scan_params->min_region_score);
        for(int i=0; i<in->num_regions; i++){
            if(in->region_score[i] < scan_params->min_region_score){
                printf("    Removing region score: %f\n", in->region_score[i]);
                remove_region(in, i);
                i--;
            }
        }
    }
}

/*
  score one region in an image

  A score of 1000 is maximum, and means that every pixel that was
  below the detection threshold was maximally rare
 */
static float score_one_region(const struct scan_params *scan_params, 
                              const struct region_bounds *bounds, 
                              const struct bgr_image *quantised,
                              const struct histogram *histogram,
                              PyArrayObject **pixel_scores)
{
        float score = 0;
        uint16_t count = 0;
        uint16_t width, height;
        width  = 1 + bounds->maxx - bounds->minx;
        height = 1 + bounds->maxy - bounds->miny;
        int dims[2] = { height, width };
//        (*pixel_scores) = (PyArrayObject *)PyArray_FromDims(2, dims, NPY_INT);
        for (uint16_t y=bounds->miny; y<=bounds->maxy; y++) {
                for (uint16_t x=bounds->minx; x<=bounds->maxx; x++) {
			const struct bgr *v = &quantised->data[y][x];                        
			uint16_t b = bgr_bin(v);
//                        double *scorep = PyArray_GETPTR2(*pixel_scores, y-bounds->miny, x-bounds->minx);
                        if (histogram->count[b] >= scan_params->histogram_count_threshold) {
//                                *scorep = 0;
                                continue;
                        }
                        uint32_t diff = (scan_params->histogram_count_threshold - histogram->count[b]);
                        count++;
                        score += diff;
//                        *scorep = diff;
                }
        }
        if (count == 0) {
                return 0;
        }
        return MIN(1000.0, 1000.0 * score / (count * scan_params->histogram_count_threshold));
}

/*
  score the regions based on their histogram.
  Score is the sum of the distance below the histogram theshold for
  all pixels in the region, divided by the number of pixels that were
  below the threshold
 */
static void score_regions(const struct scan_params *scan_params, 
                          struct regions *in, 
                          const struct bgr_image *quantised, const struct histogram *histogram)
{
	unsigned i;
	for (i=0; i<in->num_regions; i++) {
                in->region_score[i] = score_one_region(scan_params, 
                                                       &in->bounds[i], quantised, histogram, 
                                                       &in->pixel_scores[i]);
        }
}

/*
  draw a square on an image
 */
static void draw_square(struct bgr_image *img,
			const struct bgr *c,
			uint16_t left, 
			uint16_t top,
			uint16_t right, 
			uint16_t bottom)
{
	uint16_t x, y;
	for (x=left; x<= right; x++) {
		img->data[top][x] = *c;
		img->data[top+1][x] = *c;
		img->data[bottom][x] = *c;
		img->data[bottom-1][x] = *c;
	}
	for (y=top; y<= bottom; y++) {
		img->data[y][left] = *c;
		img->data[y][left+1] = *c;
		img->data[y][right] = *c;
		img->data[y][right-1] = *c;
	}
}


/*
  mark regions in an image with a blue square
 */
static void mark_regions(struct bgr_image *img, const struct regions *r)
{
	unsigned i;
	struct bgr c = { 255, 0, 0 };
	for (i=0; i<r->num_regions; i++) {
		draw_square(img, 
			    &c,
			    MAX(r->bounds[i].minx-2, 0),
			    MAX(r->bounds[i].miny-2, 0),
			    MIN(r->bounds[i].maxx+2, (img->width)-1),
			    MIN(r->bounds[i].maxy+2, (img->height)-1));
	}
}

/*
  debayer a 8 bit image to half size 24 bit
 */
static PyObject *
scanner_debayer_half(PyObject *self, PyObject *args)
{
	PyArrayObject *img_in, *img_out;
	bool use_16_bit = false;

	if (!PyArg_ParseTuple(args, "OO", &img_in, &img_out))
		return NULL;

	CHECK_CONTIGUOUS(img_in);
	CHECK_CONTIGUOUS(img_out);

        uint16_t width  = PyArray_DIM(img_in, 1);
        uint16_t height = PyArray_DIM(img_in, 0);
	use_16_bit = (PyArray_STRIDE(img_in, 0) == width*2);
        if (use_16_bit) {
		PyErr_SetString(ScannerError, "16 bit images not supported");		
		return NULL;
        }
	if (PyArray_DIM(img_out, 1) != width/2 ||
	    PyArray_DIM(img_out, 0) != height/2 ||
	    PyArray_STRIDE(img_out, 0) != 3*(width/2)) {
		PyErr_SetString(ScannerError, "output must be half size 24 bit");		
		return NULL;
	}
	
	const struct grey_image8 *in = allocate_grey_image8(height, width, PyArray_DATA(img_in));
	struct bgr_image *out = allocate_bgr_image8(height/2, width/2, NULL);

	Py_BEGIN_ALLOW_THREADS;
        colour_convert_half(in, out);
	Py_END_ALLOW_THREADS;

        memcpy(PyArray_DATA(img_out), &out->data[0][0], out->width*out->height*sizeof(struct bgr));

        free(out);
        free((void*)in);

	Py_RETURN_NONE;
}


/*
  debayer a image to a 24 bit image of the same size
 */
static PyObject *
scanner_debayer(PyObject *self, PyObject *args)
{
	PyArrayObject *img_in, *img_out;

	if (!PyArg_ParseTuple(args, "OO", &img_in, &img_out))
		return NULL;

	CHECK_CONTIGUOUS(img_in);
	CHECK_CONTIGUOUS(img_out);

        uint16_t height = PyArray_DIM(img_in, 0);
        uint16_t width  = PyArray_DIM(img_in, 1);
	if (PyArray_DIM(img_out, 1) != width ||
	    PyArray_DIM(img_out, 0) != height ||
	    PyArray_STRIDE(img_out, 0) != 3*width) {
		PyErr_SetString(ScannerError, "output must be same shape as input and 24 bit");
		return NULL;
	}
	bool eightbit = PyArray_STRIDE(img_in, 0) == PyArray_DIM(img_in, 1);
        if (!eightbit) {
		PyErr_SetString(ScannerError, "input must be 8 bit");
		return NULL;
        }
	const struct grey_image8 *in8 = allocate_grey_image8(height, width, PyArray_DATA(img_in));
        struct bgr_image *out = allocate_bgr_image8(height, width, PyArray_DATA(img_out));

	Py_BEGIN_ALLOW_THREADS;
        colour_convert(in8, out);
	Py_END_ALLOW_THREADS;

        memcpy(PyArray_DATA(img_out), &out->data[0][0], height*width*sizeof(out->data[0][0]));
        free(out);
        free((void*)in8);

	Py_RETURN_NONE;
}


/*
  rebayer a 1280x960 image from 1280x960 24 bit colour image
 */
static PyObject *
scanner_rebayer(PyObject *self, PyObject *args)
{
	PyArrayObject *img_in, *img_out;

	if (!PyArg_ParseTuple(args, "OO", &img_in, &img_out))
		return NULL;

	CHECK_CONTIGUOUS(img_in);
	CHECK_CONTIGUOUS(img_out);

        uint16_t height = PyArray_DIM(img_in, 0);
        uint16_t width  = PyArray_DIM(img_in, 1);
	if (PyArray_STRIDE(img_in, 0) != 3*width) {
		PyErr_SetString(ScannerError, "input must be 24 bit");
		return NULL;
	}
	if (PyArray_DIM(img_out, 1) != width ||
	    PyArray_DIM(img_out, 0) != height ||
	    PyArray_STRIDE(img_out, 0) != width) {
		PyErr_SetString(ScannerError, "output must same size and 8 bit");
		return NULL;
	}

	const struct bgr_image *in = allocate_bgr_image8(height, width, PyArray_DATA(img_in));
	struct grey_image8 *out = allocate_grey_image8(height, width, NULL);

	Py_BEGIN_ALLOW_THREADS;
	rebayer_1280_960_8(in, out);
	Py_END_ALLOW_THREADS;

        memcpy(PyArray_DATA(img_out), &out->data[0][0], out->width*out->height*sizeof(out->data[0][0]));

        free(out);
        free((void*)in);

	Py_RETURN_NONE;
}


/*
  convert a 16 bit thermal image to a colour image
 */
static PyObject *
scanner_thermal_convert(PyObject *self, PyObject *args)
{
	PyArrayObject *img_in, *img_out;
        unsigned short clip_high, clip_low;
        float blue_threshold, green_threshold;

	if (!PyArg_ParseTuple(args, "OOHff", 
                              &img_in, &img_out, 
                              &clip_high,
                              &blue_threshold, &green_threshold))
		return NULL;

	CHECK_CONTIGUOUS(img_in);
	CHECK_CONTIGUOUS(img_out);

        uint16_t height = PyArray_DIM(img_in, 0);
        uint16_t width  = PyArray_DIM(img_in, 1);
	if (PyArray_STRIDE(img_in, 0) != 2*width) {
		PyErr_SetString(ScannerError, "input must be 16 bit");
		return NULL;
	}
	if (PyArray_DIM(img_out, 1) != width ||
	    PyArray_DIM(img_out, 0) != height ||
	    PyArray_STRIDE(img_out, 0) != 3*width) {
		PyErr_SetString(ScannerError, "output must be same shape as input and 24 bit");
		return NULL;
	}

        const uint16_t *data = PyArray_DATA(img_in);
        struct bgr *rgb = PyArray_DATA(img_out);
        uint16_t mask = 0, minv = 0xFFFF, maxv = 0;
	Py_BEGIN_ALLOW_THREADS;

	for (uint32_t i=0; i<width*height; i++) {
            uint16_t value = data[i];
            swab(&value, &value, 2);
            value >>= 2;
            mask |= value;
            if (value > maxv) maxv = value;
            if (value < minv) minv = value;
        }

        clip_low = minv + (clip_high-minv)/10;

	for (uint32_t i=0; i<width*height; i++) {
            uint16_t value = data[i];
            swab(&value, &value, 2);
            value >>= 2;
            uint8_t map_value(float v, const float threshold) {
                if (v > threshold) {
                    float p = 1.0 - (v - threshold) / (1.0 - threshold);
                    return 255*p;
                }
                float p = 1.0 - (threshold - v) / threshold;
                return 255*p;	    
            }
            float v = 0;
            if (value >= clip_high) {
                v = 1.0;
            } else if (value > clip_low) {
                v = (value - clip_low) / (float)(clip_high - clip_low);
            }
            rgb[i].r = v*255;
            rgb[i].b = map_value(v, blue_threshold);
            rgb[i].g = map_value(v, green_threshold);
	}
	Py_END_ALLOW_THREADS;
	Py_RETURN_NONE;
}

/*
  scale the scan parameters for the image being scanned
 */
static void scale_scan_params(struct scan_params *scan_params, uint32_t height, uint32_t width)
{
    float wscale = width/640.0;
    float ascale = (width*height)/(640.0*480.0);
    if (ascale < 1.0) ascale = 1.0;
    if (wscale < 1.0) wscale = 1.0;
    *scan_params = scan_params_640_480;
    scan_params->min_region_area *= ascale;
    scan_params->max_region_area *= ascale;
    scan_params->min_region_size_xy *= wscale;
    scan_params->max_region_size_xy *= wscale;
    scan_params->histogram_count_threshold *= ascale;
    scan_params->region_merge *= wscale;
}

/*
  scan a 24 bit image for regions of interest and return the markup as
  a set of tuples
 */
static PyObject *
scanner_scan(PyObject *self, PyObject *args)
{
	PyArrayObject *img_in;
        PyObject *parm_dict = NULL;

#if SHOW_TIMING
        start_timer();
#endif
        
	if (!PyArg_ParseTuple(args, "O|O", &img_in, &parm_dict))
		return NULL;

	CHECK_CONTIGUOUS(img_in);
        
        uint16_t height = PyArray_DIM(img_in, 0);
        uint16_t width  = PyArray_DIM(img_in, 1);
	if (PyArray_STRIDE(img_in, 0) != 3*width) {
		PyErr_SetString(ScannerError, "input must be BGR 24 bit");		
		return NULL;
	}

        const struct bgr_image *in = allocate_bgr_image8(height, width, PyArray_DATA(img_in));


	struct regions *regions = any_matrix(2, 
                                             sizeof(int16_t), 
                                             offsetof(struct regions, data), 
                                             height, 
                                             width);

        /*
          we need to allocate the histogram and quantised structures
          here and pass them into colour_histogram() so that they can
          be kept around for the score_regions() code.
         */
        struct histogram *histogram;
        struct bgr_image *quantised;
        struct scan_params scan_params;

        quantised = allocate_bgr_image8(height, width, NULL);
        ALLOCATE(histogram);

        if (parm_dict != NULL) {
            scale_scan_params_user(&scan_params, height, width, parm_dict);
        } else {
            scale_scan_params(&scan_params, height, width);
        }

	Py_BEGIN_ALLOW_THREADS;

        struct bgr_image *himage = allocate_bgr_image8(height, width, NULL);
        struct bgr_image *jimage = allocate_bgr_image8(height, width, NULL);
        regions->height = height;
        regions->width = width;

	colour_histogram(&scan_params, in, himage, quantised, histogram, NULL);
	assign_regions(&scan_params, himage, regions);

        if (scan_params.save_intermediate) {
                struct bgr_image *marked;
                marked = allocate_bgr_image8(height, width, NULL);
                copy_bgr_image8(in, marked);
                mark_regions(marked, regions);
                colour_save_pnm("regions.pnm", marked);
                free(marked);
        }

	prune_large_regions(&scan_params, regions);
        if (scan_params.save_intermediate) {
                struct bgr_image *marked;
                marked = allocate_bgr_image8(height, width, NULL);
                copy_bgr_image8(in, marked);
                mark_regions(marked, regions);
                colour_save_pnm("prunelarge.pnm", marked);
                free(marked);
        }
	merge_regions(&scan_params, regions);
        if (scan_params.save_intermediate) {
                struct bgr_image *marked;
                marked = allocate_bgr_image8(height, width, NULL);
                copy_bgr_image8(in, marked);
                mark_regions(marked, regions);
                colour_save_pnm("merged.pnm", marked);
                free(marked);
        }
	prune_small_regions(&scan_params, regions);

        if (scan_params.save_intermediate) {
                struct bgr_image *marked;
                marked = allocate_bgr_image8(height, width, NULL);
                copy_bgr_image8(in, marked);
                mark_regions(marked, regions);
                colour_save_pnm("pruned.pnm", marked);
                free(marked);
        }

	free(himage);
	free(jimage);
        free((void*)in);
	Py_END_ALLOW_THREADS;

        score_regions(&scan_params, regions, quantised, histogram);

        free(histogram);
        free(quantised);

	PyObject *list = PyList_New(regions->num_regions);
	for (unsigned i=0; i<regions->num_regions; i++) {
		PyObject *t = Py_BuildValue("(iiiifO)",
					    regions->bounds[i].minx,
					    regions->bounds[i].miny,
					    regions->bounds[i].maxx,
					    regions->bounds[i].maxy,
                                            regions->region_score[i],
                                            regions->pixel_scores[i]);
		PyList_SET_ITEM(list, i, t);
	}

	free(regions);

#if SHOW_TIMING
        printf("dt=%f\n", end_timer());
#endif

	return list;
}

/*
  compress a 24 bit BGR image to a jpeg, returning as python bytes (a
  string in python 2.x)
 */
static PyObject *
scanner_jpeg_compress(PyObject *self, PyObject *args)
{
	PyArrayObject *img_in;
	unsigned short quality = 20;

	if (!PyArg_ParseTuple(args, "OH", &img_in, &quality))
		return NULL;

	CHECK_CONTIGUOUS(img_in);

	if (PyArray_STRIDE(img_in, 0) != 3*PyArray_DIM(img_in, 1)) {
		PyErr_SetString(ScannerError, "input must 24 bit BGR");
		return NULL;
	}
	const uint16_t w = PyArray_DIM(img_in, 1);
	const uint16_t h = PyArray_DIM(img_in, 0);
//	const struct bgr *bgr_in = PyArray_DATA(img_in);

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	unsigned char *dest = NULL;
	unsigned long dest_size = 0;

	struct bgr_image *rgb = allocate_bgr_image8(h, w, NULL);
	struct bgr_image *image = allocate_bgr_image8(h, w, PyArray_DATA(img_in));
	uint16_t x, y;
	for (y=0; y<rgb->height; y++) {
		for (x=0; x<rgb->width; x++) {
			rgb->data[y][x].r = image->data[y][x].b;
			rgb->data[y][x].g = image->data[y][x].g;
			rgb->data[y][x].b = image->data[y][x].r;
		}
    }
	const struct bgr *bgr_in = &rgb->data[0][0];

	Py_BEGIN_ALLOW_THREADS;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	jpeg_mem_dest(&cinfo, &dest, &dest_size);

	cinfo.image_width = w;
	cinfo.image_height = h;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);

	jpeg_set_quality(&cinfo, quality, TRUE);

	jpeg_start_compress(&cinfo, TRUE);

	while(cinfo.next_scanline < cinfo.image_height){
		JSAMPROW row_pointer = (JSAMPROW)&bgr_in[cinfo.next_scanline * w];
		jpeg_write_scanlines(&cinfo, &row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	Py_END_ALLOW_THREADS;

	PyObject *ret = PyString_FromStringAndSize((const char *)dest, dest_size);
	jpeg_destroy_compress(&cinfo);
	free(dest);
	free(rgb);
	free(image);

	return ret;
}

/*
  downsample a 24 bit colour image by 2x
 */
static PyObject *
scanner_downsample(PyObject *self, PyObject *args)
{
	PyArrayObject *img_in, *img_out;

	if (!PyArg_ParseTuple(args, "OO", &img_in, &img_out))
		return NULL;

	CHECK_CONTIGUOUS(img_in);
	CHECK_CONTIGUOUS(img_out);

        uint16_t height = PyArray_DIM(img_in, 0);
        uint16_t width  = PyArray_DIM(img_in, 1);

	if (PyArray_STRIDE(img_in, 0) != width*3) {
		PyErr_SetString(ScannerError, "input must be 24 bit");
		return NULL;
	}
	if (PyArray_DIM(img_out, 1) != width/2 ||
	    PyArray_DIM(img_out, 0) != height/2 ||
	    PyArray_STRIDE(img_out, 0) != 3*(width/2)) {
		PyErr_SetString(ScannerError, "output must be half-size 24 bit");
		return NULL;
	}

        const struct bgr_image *in = allocate_bgr_image8(height, width, PyArray_DATA(img_in));
	struct bgr_image *out = allocate_bgr_image8(height/2, width/2, NULL);

	Py_BEGIN_ALLOW_THREADS;
	for (uint16_t y=0; y<height/2; y++) {
		for (uint16_t x=0; x<width/2; x++) {
			const struct bgr *p0 = &in->data[y*2+0][x*2+0];
			const struct bgr *p1 = &in->data[y*2+0][x*2+1];
			const struct bgr *p2 = &in->data[y*2+1][x*2+0];
			const struct bgr *p3 = &in->data[y*2+1][x*2+1];
			struct bgr *d = &out->data[y][x];
			d->b = ((uint16_t)p0->b + (uint16_t)p1->b + (uint16_t)p2->b + (uint16_t)p3->b)/4;
			d->g = ((uint16_t)p0->g + (uint16_t)p1->g + (uint16_t)p2->g + (uint16_t)p3->g)/4;
			d->r = ((uint16_t)p0->r + (uint16_t)p1->r + (uint16_t)p2->r + (uint16_t)p3->r)/4;
		}
	}
	Py_END_ALLOW_THREADS;

        memcpy(PyArray_DATA(img_out), &out->data[0][0], out->width*out->height*sizeof(struct bgr));

        free(out);
        free((void*)in);

	Py_RETURN_NONE;
}


/*
  reduce bit depth of an image from 16 bit to 8 bit
 */
static PyObject *
scanner_reduce_depth(PyObject *self, PyObject *args)
{
	PyArrayObject *img_in, *img_out;
	uint16_t w, h;

	if (!PyArg_ParseTuple(args, "OO", &img_in, &img_out))
		return NULL;

	CHECK_CONTIGUOUS(img_in);
	CHECK_CONTIGUOUS(img_out);

	w = PyArray_DIM(img_out, 1);
	h = PyArray_DIM(img_out, 0);

	if (PyArray_STRIDE(img_in, 0) != w*2) {
		PyErr_SetString(ScannerError, "input must be 16 bit");
		return NULL;
	}
	if (PyArray_STRIDE(img_out, 0) != w) {
		PyErr_SetString(ScannerError, "output must be 8 bit");
		return NULL;
	}
	if (PyArray_DIM(img_out, 1) != w ||
	    PyArray_DIM(img_out, 0) != h) {
		PyErr_SetString(ScannerError, "input and output sizes must match");
		return NULL;
	}

	const uint16_t *in = PyArray_DATA(img_in);
	uint8_t *out = PyArray_DATA(img_out);

	Py_BEGIN_ALLOW_THREADS;
	for (uint32_t i=0; i<w*h; i++) {
		out[i] = in[i]>>8;
	}
	Py_END_ALLOW_THREADS;

	Py_RETURN_NONE;
}


/*
  rotate a 24 bit image by 180 degrees in place
 */
static PyObject *
scanner_rotate_180(PyObject *self, PyObject *args)
{
	PyArrayObject *img;
	uint16_t w, h;

	if (!PyArg_ParseTuple(args, "O", &img))
		return NULL;

	CHECK_CONTIGUOUS(img);

	w = PyArray_DIM(img, 1);
	h = PyArray_DIM(img, 0);

	if (PyArray_STRIDE(img, 0) != w*3) {
		PyErr_SetString(ScannerError, "input must be 24 bit");
		return NULL;
	}
        struct bgr *bgr = PyArray_DATA(img);
        uint32_t size = w*h;

	Py_BEGIN_ALLOW_THREADS;
	for (uint32_t i=0; i<size/2; i++) {
            struct bgr tmp = bgr[i];
            bgr[i] = bgr[size-i];
            bgr[size-i] = tmp;
	}
	Py_END_ALLOW_THREADS;

	Py_RETURN_NONE;
}

/*
  reduce bit depth of an image from 16 bit to 8 bit, applying gamma
 */
static PyObject *
scanner_gamma_correct(PyObject *self, PyObject *args)
{
	PyArrayObject *img_in, *img_out;
	uint16_t w, h;
	uint8_t lookup[0x1000];
	unsigned short gamma;

	if (!PyArg_ParseTuple(args, "OOH", &img_in, &img_out, &gamma))
		return NULL;

	CHECK_CONTIGUOUS(img_in);
	CHECK_CONTIGUOUS(img_out);

	w = PyArray_DIM(img_out, 1);
	h = PyArray_DIM(img_out, 0);

	if (PyArray_STRIDE(img_in, 0) != w*2) {
		PyErr_SetString(ScannerError, "input must be 16 bit");
		return NULL;
	}
	if (PyArray_STRIDE(img_out, 0) != w) {
		PyErr_SetString(ScannerError, "output must be 8 bit");
		return NULL;
	}
	if (PyArray_DIM(img_out, 1) != w ||
	    PyArray_DIM(img_out, 0) != h) {
		PyErr_SetString(ScannerError, "input and output sizes must match");
		return NULL;
	}

	const uint16_t *in = PyArray_DATA(img_in);
	uint8_t *out = PyArray_DATA(img_out);

	Py_BEGIN_ALLOW_THREADS;
	uint32_t i;
	double p = 1024.0 / gamma;
	double z = 0xFFF;
	for (i=0; i<0x1000; i++) {
		double v = ceil(255 * pow(i/z, p));
		if (v >= 255) {
			lookup[i] = 255;
		} else {
			lookup[i] = v;
		}
	}
	for (i=0; i<w*h; i++) {
		out[i] = lookup[in[i]>>4];
	}
	Py_END_ALLOW_THREADS;

	Py_RETURN_NONE;
}


/*
  extract a rectange from a 24 bit BGR image
  img_in is a 24 bit large image
  img_out is a 24 bit small target image
  x1, y1 are top left coordinates of target in img1
 */
static PyObject *
scanner_rect_extract(PyObject *self, PyObject *args)
{
	PyArrayObject *img_in, *img_out;
	unsigned short x1, y1, x2, y2;
	unsigned short x, y, w, h, w_out, h_out;

	if (!PyArg_ParseTuple(args, "OOHH", &img_in, &img_out, &x1, &y1))
		return NULL;

	CHECK_CONTIGUOUS(img_in);
	CHECK_CONTIGUOUS(img_out);

	w = PyArray_DIM(img_in, 1);
	h = PyArray_DIM(img_in, 0);

	w_out = PyArray_DIM(img_out, 1);
	h_out = PyArray_DIM(img_out, 0);

	if (PyArray_STRIDE(img_in, 0) != w*3) {
		PyErr_SetString(ScannerError, "input must be 24 bit");
		return NULL;
	}
	if (PyArray_STRIDE(img_out, 0) != w_out*3) {
		PyErr_SetString(ScannerError, "output must be 24 bit");
		return NULL;
	}
	if (x1 >= w || y1 >= h) {
		PyErr_SetString(ScannerError, "corner must be inside input image");
		return NULL;		
	}

	const struct bgr *in = PyArray_DATA(img_in);
	struct bgr *out = PyArray_DATA(img_out);

	Py_BEGIN_ALLOW_THREADS;
	x2 = x1 + w_out - 1;
	y2 = y1 + h_out - 1;       

	if (x2 >= w) x2 = w-1;
	if (y2 >= h) y2 = h-1;

	for (y=y1; y<=y2; y++) {
		const struct bgr *in_y = in + y*w;
		struct bgr *out_y = out + (y-y1)*w_out;
		for (x=x1; x<=x2; x++) {
			out_y[x-x1] = in_y[x];
		}
	}
	Py_END_ALLOW_THREADS;

	Py_RETURN_NONE;
}

/*
  overlay a rectange on a 24 bit BGR image
  img1 is a large image
  img2 is a small image to be overlayed on top of img1
  x1, y1 are top left coordinates of target in img1
 */
static PyObject *
scanner_rect_overlay(PyObject *self, PyObject *args)
{
	PyArrayObject *img1, *img2;
	unsigned short x1, y1, x2, y2;
	unsigned short x, y, w1, h1, w2, h2;
	PyObject *skip_black_obj;
	bool skip_black;

	if (!PyArg_ParseTuple(args, "OOHHO", &img1, &img2, &x1, &y1, &skip_black_obj))
		return NULL;

	CHECK_CONTIGUOUS(img1);
	CHECK_CONTIGUOUS(img2);

	skip_black = PyObject_IsTrue(skip_black_obj);

	w1 = PyArray_DIM(img1, 1);
	h1 = PyArray_DIM(img1, 0);
	w2 = PyArray_DIM(img2, 1);
	h2 = PyArray_DIM(img2, 0);

	if (PyArray_STRIDE(img1, 0) != w1*3) {
		PyErr_SetString(ScannerError, "image 1 must be 24 bit");
		return NULL;
	}
	if (PyArray_STRIDE(img2, 0) != w2*3) {
		PyErr_SetString(ScannerError, "image 2 must be 24 bit");
		return NULL;
	}
	if (x1 >= w1 || y1 >= h1) {
		PyErr_SetString(ScannerError, "corner must be inside image1");
		return NULL;		
	}

	struct bgr *im1 = PyArray_DATA(img1);
	const struct bgr *im2 = PyArray_DATA(img2);

	Py_BEGIN_ALLOW_THREADS;
	x2 = x1 + w2 - 1;
	y2 = y1 + h2 - 1;       

	if (x2 >= w1) x2 = w1-1;
	if (y2 >= h1) y2 = h1-1;

	if (skip_black) {
		for (y=y1; y<=y2; y++) {
			struct bgr *im1_y = im1 + y*w1;
			const struct bgr *im2_y = im2 + (y-y1)*w2;
			for (x=x1; x<=x2; x++) {
				const struct bgr *px = &im2_y[x-x1];
				if (px->b == 0 && 
				    px->g == 0 && 
				    px->r == 0) continue;
				im1_y[x] = im2_y[x-x1];
			}
		}
	} else {
		for (y=y1; y<=y2; y++) {
			struct bgr *im1_y = im1 + y*w1;
			const struct bgr *im2_y = im2 + (y-y1)*w2;
			for (x=x1; x<=x2; x++) {
				im1_y[x] = im2_y[x-x1];
			}
		}
	}

	Py_END_ALLOW_THREADS;

	Py_RETURN_NONE;
}

/*
  scan a 24 bit image for regions of interest and return the markup as
  a set of tuples
 */
static PyObject *
scanner_scan_python(PyObject *self, PyObject *args)
{
	PyArrayObject *img_in;
    PyObject *parm_dict = NULL;
    PyStringObject *save;

#if SHOW_TIMING
        start_timer();
#endif

	if (!PyArg_ParseTuple(args, "OS|O", &img_in, &save, &parm_dict))
		return NULL;

	const char *saveDir = PyString_AS_STRING(save);

	CHECK_CONTIGUOUS(img_in);

        uint16_t height = PyArray_DIM(img_in, 0);
        uint16_t width  = PyArray_DIM(img_in, 1);

	if (PyArray_STRIDE(img_in, 0) != 3*width) {
		PyErr_SetString(ScannerError, "input must be BGR 24 bit");
		return NULL;
	}

	const struct bgr_image *in = allocate_bgr_image8(height, width, PyArray_DATA(img_in));

	struct regions *regions = image_processor(width, height, parm_dict, in, saveDir, true);
	PyObject *list;
	if(regions->num_regions > 0){
		list = PyList_New(regions->num_regions);
//		printf("Found a good region, as follows:\n");
		for (unsigned i=0; i<regions->num_regions; i++) {
//			printf("--- score: %u\n", regions->region_score[i]);
			PyObject *t = Py_BuildValue("(fffff)",
					regions->bounds[i].minx*1.0,
					regions->bounds[i].miny*1.0,
					regions->bounds[i].maxx*1.0,
					regions->bounds[i].maxy*1.0,
					regions->region_score[i]);
			PyList_SET_ITEM(list, i, t);
		}
	}
	else{
		printf("--- No region found for this image.\n");
		list = PyList_New(0);
	}

	int region_size = PyList_Size(list);
	printf("Region list size: ");
	printf("%i\n", region_size);

	free(regions);

	return list;
}

static PyObject *
scanner_save_pnm_grey(PyObject *self, PyObject *args)
{
	PyArrayObject *img_in;
    PyStringObject *save;
    unsigned short width, height;

	if (!PyArg_ParseTuple(args, "OSHH", &img_in, &save, &width, &height))
		return NULL;

	const char *saveDir = PyString_AS_STRING(save);

	CHECK_CONTIGUOUS(img_in);

	printf("Testing...");
	char filepath[256] = {0};
	strcpy(filepath, saveDir);
	strcat(filepath, "0-rgb-grey.pnm");
	if(colour_save_pnm_grey(filepath, PyArray_DATA(img_in), width, height))
    	printf("Save grey succeeded.");
    else
        printf("Save grey failed.");

	Py_RETURN_NONE;
}


static PyObject *
scanner_png_raw_to_bgr(PyObject *self, PyObject *args)
{
	PyArrayObject *img_out;
	PyStringObject *file;

	printf("Parsing args tuple\n");
	if(!PyArg_ParseTuple(args, "OS", &img_out, &file))
		return NULL;

	CHECK_CONTIGUOUS(img_out);

	printf("Casting arguments\n");
	const char *path = PyString_AS_STRING(file);
	uint16_t height = PyArray_DIM(img_out, 0);
	uint16_t width = PyArray_DIM(img_out, 1);
	
	printf("filename: %s, width: %i, height: %i\n", path, width, height);

	if(PyArray_STRIDE(img_out, 0) != 3*width){
		PyErr_SetString(ScannerError, "output must be BGR 24 bit");
		return NULL;
	}

	FILE *fp = fopen(path, "rb");
	if(fp == 0){
		PyErr_SetString(ScannerError, "path not found");
		return NULL;
	}
	
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if(png_ptr == 0){
		PyErr_SetString(ScannerError, "png_create_read_struct failed");
		fclose(fp);
		return NULL;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if(info_ptr == 0){
		PyErr_SetString(ScannerError, "png_create_info_struct failed");
		png_destroy_read_struct(&png_ptr, 0, 0);
		fclose(fp);
		return NULL;
	}

	if(setjmp(png_jmpbuf(png_ptr))){
		PyErr_SetString(ScannerError, "png_jmpbuf failed");
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		fclose(fp);
		return NULL;
	}

	png_init_io(png_ptr, fp);
	printf("initialized png_init_io\n");

	//png_unknown_chunk userChunk;
	//png_set_read_user_chunk_fn(png_ptr, (void *)&userChunk, read_chunk_callback);
	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND, 0);
	
	printf("Calling doExtractCanonicalData\n");
	unsigned char *bgr = doExtractCanonicalData(png_ptr, info_ptr);
	if(!bgr){
		PyErr_SetString(ScannerError, "doExtractCanonicalData failed");
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		fclose(fp);
		return NULL;
	}
	int size = height*width*3;
	printf("Prepping img_out size: %i\n", size);
	memcpy(PyArray_DATA(img_out), bgr, size);

	png_destroy_read_struct(&png_ptr, &info_ptr, 0);
	free(bgr);
	fclose(fp);

	printf("img_out is ready to go\n");
	Py_RETURN_NONE;	
}

static PyMethodDef ScannerMethods[] = {
	{"debayer_half", scanner_debayer_half, METH_VARARGS, "simple debayer of image to half size 24 bit"},
	{"debayer", scanner_debayer, METH_VARARGS, "debayer of image to full size 24 bit image"},
	{"rebayer", scanner_rebayer, METH_VARARGS, "rebayer of image"},
	{"scan", scanner_scan, METH_VARARGS, "histogram scan a colour image"},
	{"jpeg_compress", scanner_jpeg_compress, METH_VARARGS, "compress a colour image to a jpeg image as a python string"},
	{"downsample", scanner_downsample, METH_VARARGS, "downsample a 24 bit BGR colour image to half size"},
	{"reduce_depth", scanner_reduce_depth, METH_VARARGS, "reduce greyscale bit depth from 16 bit to 8 bit"},
	{"gamma_correct", scanner_gamma_correct, METH_VARARGS, "reduce greyscale, applying gamma"},
	{"rect_extract", scanner_rect_extract, METH_VARARGS, "extract a rectange from a 24 bit BGR image"},
	{"rect_overlay", scanner_rect_overlay, METH_VARARGS, "overlay a image with another smaller image at x,y"},
	{"rotate180", scanner_rotate_180, METH_VARARGS, "rotate 24 bit image by 180 degrees in place"},
	{"thermal_convert", scanner_thermal_convert, METH_VARARGS, "convert 16 bit thermal image to colour"},
	{"scan_python", scanner_scan_python, METH_VARARGS, "histogram scan a color image"},
	{"save_pnm_grey", scanner_save_pnm_grey, METH_VARARGS, "Save image as greyscale"},
	{"png_raw_to_bgr", scanner_png_raw_to_bgr, METH_VARARGS, "Convert Sightline Raw 16-bit PNG to 24-bit BGR"},
	{NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initscanner(void)
{
	PyObject *m;

	m = Py_InitModule("scanner", ScannerMethods);
	if (m == NULL)
		return;

	import_array();
	
	ScannerError = PyErr_NewException("scanner.error", NULL, NULL);
	Py_INCREF(ScannerError);
	PyModule_AddObject(m, "error", ScannerError);
}


/*
 C-Interface
 */
void scanner_thermal_convert_16_bit_c(int width, int height, unsigned short threshold, float blue_threshold, float green_threshold, char *img_in, char *img_out)
{
	unsigned short clip_low;
	struct bgr *rgb = (struct bgr *)img_out;
    const uint16_t *data = (uint16_t *)img_in;
    uint16_t mask = 0, minv = 0xFFFF, maxv = 0;

	for (uint32_t i=0; i<width*height; i++) {
		uint16_t value = data[i];
      	swab(&value, &value, 2);
		value >>= 2;
		mask |= value;
		if (value > maxv) maxv = value;
		if (value < minv) minv = value;
	}

    clip_low = minv + (threshold-minv)/10;

    for (uint32_t i=0; i<width*height; i++) {
        uint16_t value = data[i];
        swab(&value, &value, 2);
        value >>= 2;
        uint8_t normalize(float v, const float threshold) {
        	float p;
            if (v > threshold) {
                p = 1.0 - (v - threshold) / (1.0 - threshold);
            }
            else{
                p = 1.0 - (threshold - v) / threshold;
            }

            return p * 255;
        }
        float v = 0;
        if (value >= threshold) {
            v = 1.0;
        } else if (value > clip_low) {
            v = (value - clip_low) / (float)(threshold - clip_low);
        }
        rgb[i].r = v * 255;
        rgb[i].b = normalize(v, blue_threshold);
        rgb[i].g = normalize(v, green_threshold);
    }
}

void scanner_thermal_convert_8_bit_c(int width, int height, unsigned short threshold, float blue_threshold, float green_threshold, char *img_in, char *img_out)
{
	unsigned short clip_low;
	struct bgr *rgb = (struct bgr *)img_out;
    const uint8_t *data = (uint8_t *)img_in;
    uint8_t mask = 0, minv = 0xFF, maxv = 0;

	for (uint32_t i=0; i<width*height; i++) {
		uint8_t value = data[i];

		value >>= 2;
		mask |= value;
		if (value > maxv) maxv = value;
		if (value < minv) minv = value;
	}

    clip_low = minv + (threshold-minv)/10;

    for (uint32_t i=0; i<width*height; i++) {
        uint8_t value = data[i];

        value >>= 2;
        uint8_t normalize(float v, const float threshold) {
        	float p;
            if (v > threshold) {
                p = 1.0 - (v - threshold) / (1.0 - threshold);
            }
            else{
                p = 1.0 - (threshold - v) / threshold;
            }

            return p * 255;
        }
        float v = 0;
        if (value >= threshold) {
            v = 1.0;
        } else if (value > clip_low) {
            v = (value - clip_low) / (float)(threshold - clip_low);
        }
        rgb[i].r = v * 255;
        rgb[i].b = normalize(v, blue_threshold);
        rgb[i].g = normalize(v, green_threshold);
    }
}

void scanner_debayer_c(uint16_t width, uint16_t height, char *img_in, char *img_out)
{
	const struct grey_image8 *in8 = allocate_grey_image8(height, width, (uint8_t *)img_in);
	struct bgr_image *out = allocate_bgr_image8(height, width, (struct bgr *)img_out);

	colour_convert(in8, out);

	memcpy(img_out, &out->data[0][0], height*width*sizeof(out->data[0][0]));
	free(out);
	free((void*)in8);
}

struct regions * scanner_scan_c(uint16_t width, uint16_t height, char *img_in, PyObject *parm_dict, char *saveDir)
{
    const struct bgr_image *in = allocate_bgr_image8(height, width, (struct bgr *)img_in);

    return image_processor(width, height, parm_dict, in, saveDir, false);
}

void generateRegionLog(char *saveDir, char *file, struct regions *regions)
{
	if(regions->num_regions > 0){
		char filepath[256] = {0};
		strcpy(filepath, saveDir);
		strcat(filepath, file);
		FILE *fp = fopen(filepath, "w");

		char aregion[256] = {0};
		for(int i=0;i<regions->num_regions;i++){
			sprintf(aregion, "(%i,%i,%i,%i,%i)\n", regions->bounds[i].minx,regions->bounds[i].miny,regions->bounds[i].maxx,regions->bounds[i].maxy,(int)regions->region_score[i]);
			fwrite(aregion, strlen(aregion), 1, fp);
		}

		fclose(fp);
	}
}

static void compute_regions(const struct scan_params *scan_params,
								const struct bgr_image *in,
								struct bgr_image *quantised,
								struct histogram *histogram,
								struct regions *regions,
								char *saveDir)
{
	struct bgr_image *himage = allocate_bgr_image8(in->height, in->width, NULL);;
	printf("histogram_count_threshold: %i\n", scan_params->histogram_count_threshold);
	histogram_neighbors(scan_params, in, himage, quantised, histogram, saveDir);
	assign_regions(scan_params, himage, regions);
	printf("regions found: %i\n", regions->num_regions);
	free(himage);
}

static uint32_t previous_histogram_threshold = 5000;
struct regions * image_processor(uint16_t width, uint16_t height, PyObject *parm_dict, const struct bgr_image *in, char *saveDir, bool fromPython)
{
	struct regions *regions = any_matrix(2,
                                             sizeof(int16_t),
                                             offsetof(struct regions, data),
                                             height,
                                             width);

	/*
	  we need to allocate the histogram and quantised structures
	  here and pass them into colour_histogram() so that they can
	  be kept around for the score_regions() code.
	 */
	struct histogram *histogram;
	struct bgr_image *quantised;
	struct scan_params scan_params;
	char *file;

	quantised = allocate_bgr_image8(height, width, NULL);
	ALLOCATE(histogram);

	if (parm_dict != NULL) {
		scale_scan_params_user(&scan_params, height, width, parm_dict);
	} else {
		scale_scan_params(&scan_params, height, width);
	}

	printf("width:%i, height:%i, saveDir:%s, save:%i, adaptive_rarity:%i\n", width, height, saveDir, scan_params.save_intermediate, scan_params.adaptive_rarity);

	regions->height = height;
	regions->width = width;

	histogram_quantization(&scan_params, in, quantised, histogram, saveDir);
	float hct = scan_params.histogram_count_threshold;

	uint32_t min, max;
	max = min = scan_params.histogram_count_threshold;
	if(scan_params.adaptive_rarity){
		for(int i=0;i<(1<<HISTOGRAM_BITS-1);i++){
			if(min > histogram->count[i])
				min = histogram->count[i];
			else if (max < histogram->count[i])
				max = histogram->count[i];
		}

		scan_params.histogram_count_threshold = MAX(previous_histogram_threshold, min);
	}

	do{
		compute_regions(&scan_params, in, quantised, histogram, regions, saveDir);
		if(regions->num_regions < MAX_REGIONS*0.4){
			previous_histogram_threshold = scan_params.histogram_count_threshold;
			scan_params.histogram_count_threshold *= 1.2;
			if(scan_params.histogram_count_threshold >= max)
				break;
		}
		else if (regions->num_regions == MAX_REGIONS){
			printf("Oops, exceeded max region... reducing threshold.\n");
			scan_params.histogram_count_threshold *= 0.5;
			previous_histogram_threshold = scan_params.histogram_count_threshold;

			compute_regions(&scan_params, in, quantised, histogram, regions, saveDir);
			break;
		}
		else
			break;
	}while(scan_params.adaptive_rarity);

	scan_params.histogram_count_threshold = scan_params.adaptive_rarity ? previous_histogram_threshold : hct;

	printf("Score regions with hct = %i\n", scan_params.histogram_count_threshold);
	score_regions(&scan_params, regions, quantised, histogram);

	printf("saving 2-regions.pnm\n");
	file = "2-regions.pnm";
	if (scan_params.save_intermediate) {
        struct bgr_image *marked;
        marked = allocate_bgr_image8(height, width, NULL);
        copy_bgr_image8(in, marked);
        mark_regions(marked, regions);
        char filepath[256] = {0};
        strcpy(filepath, saveDir);
        strcat(filepath, file);
        colour_save_pnm(filepath, marked);
        free(marked);
    	generateRegionLog(saveDir, "2-regions.log", regions);
	}

	prune_small_regions(&scan_params, regions);
	file = "5-prunesmall.pnm";
	if (scan_params.save_intermediate) {
        struct bgr_image *marked;
        marked = allocate_bgr_image8(height, width, NULL);
        copy_bgr_image8(in, marked);
        mark_regions(marked, regions);
        char filepath[256] = {0};
        strcpy(filepath, saveDir);
        strcat(filepath, file);
        colour_save_pnm(filepath, marked);
        free(marked);
    	generateRegionLog(saveDir, "5-prunesmall.log", regions);
	}

//	prune_low_score_regions(&scan_params, regions);
	merge_regions(&scan_params, regions);
	score_regions(&scan_params, regions, quantised, histogram);
	file = "3-merged.pnm";
	if (scan_params.save_intermediate) {
        struct bgr_image *marked;
        marked = allocate_bgr_image8(height, width, NULL);
        copy_bgr_image8(in, marked);
        mark_regions(marked, regions);
        char filepath[256] = {0};
        strcpy(filepath, saveDir);
        strcat(filepath, file);
        colour_save_pnm(filepath, marked);
        free(marked);
    	generateRegionLog(saveDir, "3-merged.log", regions);
	}

	prune_large_regions(&scan_params, regions);
	file = "4-prunelarge.pnm";
	if (scan_params.save_intermediate) {
        struct bgr_image *marked;
        marked = allocate_bgr_image8(height, width, NULL);
        copy_bgr_image8(in, marked);
        mark_regions(marked, regions);
        char filepath[256] = {0};
        strcpy(filepath, saveDir);
        strcat(filepath, file);
        colour_save_pnm(filepath, marked);
        free(marked);
    	generateRegionLog(saveDir, "4-prunelarge.log", regions);
	}

	prune_low_score_regions(&scan_params, regions);
	file = "pruned.pnm";
	if (scan_params.save_intermediate) {
        struct bgr_image *marked;
        marked = allocate_bgr_image8(height, width, NULL);
        copy_bgr_image8(in, marked);
        mark_regions(marked, regions);
        char filepath[256] = {0};
        strcpy(filepath, saveDir);
        strcat(filepath, file);
        colour_save_pnm(filepath, marked);
        free(marked);
    	generateRegionLog(saveDir, "pruned.log", regions);
	}

    free((void*)in);

	printf("Scoring regions\n");

	free(histogram);
	free(quantised);

	printf("Number of regions: %i\n", regions->num_regions);

#if SHOW_TIMING
        printf("dt=%f\n", end_timer());
#endif

	return regions;
}

