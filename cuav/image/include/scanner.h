#define scanner_h

#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <numpy/arrayobject.h>
#include <modsupport.h>

#include "../include/imageutil.h"

//#undef __ARM_NEON__

#define NOINLINE __attribute__((noinline))

#ifdef __ARM_NEON__
#include <arm_neon.h>
#endif

#include <jpeglib.h>

#ifndef Py_RETURN_NONE
#define Py_RETURN_NONE return Py_INCREF(Py_None), Py_None
#endif

#define CHECK_CONTIGUOUS(a) do { if (!PyArray_ISCONTIGUOUS(a)) { \
	PyErr_SetString(ScannerError, "array must be contiguous"); \
	return NULL; \
	}} while (0)

static PyObject *ScannerError;

#define PACKED __attribute__((__packed__))

#define ALLOCATE(p) (p) = malloc(sizeof(*p))

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

#define MAX_REGIONS 4000

#define HISTOGRAM_BITS_PER_COLOR 3
#define HISTOGRAM_BITS (3*HISTOGRAM_BITS_PER_COLOR)
#define HISTOGRAM_BINS (1<<HISTOGRAM_BITS)

struct scan_params {
    bool save_intermediate;
    bool blue_emphasis;
    bool infra_red;
    bool adaptive_rarity;
    uint8_t infra_red_max_threshold;
    uint8_t infra_red_min_threshold;
    uint16_t min_region_area;
    uint16_t max_region_area;
    uint16_t min_region_size_xy;
    uint16_t max_region_size_xy;
    uint16_t region_merge;
    uint16_t min_region_score;
    uint32_t histogram_count_threshold;
};

static const struct scan_params scan_params_640_480 = {
	min_region_area : 8,
    max_region_area : 400,
    min_region_size_xy : 2,
    max_region_size_xy : 30,
    histogram_count_threshold : 50,
    region_merge : 1,
    save_intermediate : false,
    blue_emphasis : false,
    infra_red : false,
    infra_red_max_threshold : 7,
    infra_red_min_threshold : 4,
    min_region_score : 500
};

struct regions {
        uint16_t height;
        uint16_t width;
	unsigned num_regions;
	uint32_t region_size[MAX_REGIONS];
	struct region_bounds {
		uint16_t minx, miny;
		uint16_t maxx, maxy;
	} bounds[MAX_REGIONS];
	float region_score[MAX_REGIONS];
        PyArrayObject *pixel_scores[MAX_REGIONS];
        // data is a 2D array of image dimensions. Each value is the
        // assigned region number or REGION_NONE
        int16_t **data;
};

struct image_header {
	char *type;
	char *dims;
	char *colorspace;
};

#define SHOW_TIMING 0

/*
  lookup a key in a dictionary and return value as a float, or
  default_value if not found
 */
static float dict_lookup(PyObject *parm_dict, const char *key, float default_value)
{
    PyObject *obj = PyDict_GetItemString(parm_dict, key);
    if (obj == NULL || !PyFloat_Check(obj)) {
        return default_value;
    }
    return PyFloat_AsDouble(obj);
}

/*
  scale the scan parameters for the image being scanned
 */
static void scale_scan_params_user(struct scan_params *scan_params, uint32_t height, uint32_t width, PyObject *parm_dict)
{
    float meters_per_pixel = dict_lookup(parm_dict, "MetersPerPixel", 0.1);
    float meters_per_pixel2 = meters_per_pixel * meters_per_pixel;
    *scan_params = scan_params_640_480;
    float mra = dict_lookup(parm_dict, "MinRegionArea", 1.0) / meters_per_pixel2;
    scan_params->min_region_area = MAX((uint16_t)mra, 1);
    float xra = dict_lookup(parm_dict, "MaxRegionArea", 4.0) / meters_per_pixel2;
    scan_params->max_region_area = MAX((uint16_t)xra, 1);
    float mrs = dict_lookup(parm_dict, "MinRegionSize", 0.25) / meters_per_pixel;
    scan_params->min_region_size_xy = MAX((uint16_t)mrs, 1);
    float xrs = dict_lookup(parm_dict, "MaxRegionSize", 4.0) / meters_per_pixel;
    scan_params->max_region_size_xy = MAX((uint16_t)xrs, 1);
    float hct = dict_lookup(parm_dict, "MaxRarityPct", 0.016) * (width*height)/100.0;
    scan_params->histogram_count_threshold = MAX((uint16_t)hct, 1);
    float rm = dict_lookup(parm_dict, "RegionMergeSize", 0.5) / meters_per_pixel;
    scan_params->region_merge = MAX((uint16_t)rm, 1);
    scan_params->save_intermediate = dict_lookup(parm_dict, "SaveIntermediate", 0);
    scan_params->blue_emphasis = dict_lookup(parm_dict, "BlueEmphasis", 0);
    scan_params->infra_red = dict_lookup(parm_dict, "DetectInfraRed", 0);
    scan_params->adaptive_rarity = dict_lookup(parm_dict, "ForceScanImage", 1);
    scan_params->infra_red_max_threshold = (uint8_t)dict_lookup(parm_dict, "InfraRedMaxThreshold", 7);
    scan_params->infra_red_min_threshold = (uint8_t)dict_lookup(parm_dict, "InfraRedMinThreshold", 3);
    scan_params->min_region_score = (uint16_t)dict_lookup(parm_dict, "MinRegionScore", 0);
    if (scan_params->save_intermediate) {
        printf("mpp=%f mpp2=%f min_region_area=%u max_region_area=%u min_region_size_xy=%u max_region_size_xy=%u histogram_count_threshold=%u region_merge=%u\n",
               meters_per_pixel,
               meters_per_pixel2,
               scan_params->min_region_area,
               scan_params->max_region_area,
               scan_params->min_region_size_xy,
               scan_params->max_region_size_xy,
               scan_params->histogram_count_threshold,
               scan_params->region_merge);
    }
}


// Prototypes
int
	processImage(char *file, bool isThermal, bool ir);

bool
	isSupportedImageFile(char *file);

void
	removeDir(char *saveDir);

PyMODINIT_FUNC
	initscanner(void);

struct regions *
	scanner_scan_c(uint16_t width, uint16_t height, char *img_in, PyObject *parm_dict, char *saveDir);

void
	scanner_debayer_c(uint16_t width, uint16_t height, char *img_in, char *img_out);

void
	scanner_thermal_convert_16_bit_c(int width, int height, unsigned short threshold, float blue_threshold, float green_threshold, char *img_in, char *img_out);

void
	scanner_thermal_convert_8_bit_c(int width, int height, unsigned short threshold, float blue_threshold, float green_threshold, char *img_in, char *img_out);

struct regions *
	image_processor(uint16_t width, uint16_t height, PyObject *parm_dict, const struct bgr_image *in, char *saveDir, bool fromPython);
