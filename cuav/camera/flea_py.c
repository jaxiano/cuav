#include <Python.h>
#include <numpy/arrayobject.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "flea_lib.h"

static PyObject *FleaError;

#define NUM_CAMERA_HANDLES 2
static fleaCamera* cameras[NUM_CAMERA_HANDLES] = {NULL, NULL};

float shutters[NUM_CAMERA_HANDLES] = {
  0.0, 0.0
};


#ifndef Py_RETURN_NONE
#define Py_RETURN_NONE return Py_INCREF(Py_None), Py_None
#endif

#define CHECK_CONTIGUOUS(a) do { if (!PyArray_ISCONTIGUOUS(a)) { \
	PyErr_SetString(FleaError, "array must be contiguous"); \
	return NULL; \
	}} while (0)


static PyObject *
flea_open(PyObject *self, PyObject *args)
{
    fleaCamera* cam;
	int i = 0, handle=-1;
	unsigned short depth = 0;
	unsigned short brightness;
	PyObject *colour_obj;
	unsigned int height = 2048, width = 2448;
	
	//Trying to stick to the chameleon interface.  Deal with the extra parameters later
	if (!PyArg_ParseTuple(args, "OHH|II", &colour_obj, &depth, &brightness, &height, &width))
	{
        return NULL;	
    }
    

	for (i = 0; i < NUM_CAMERA_HANDLES; ++i) {
		if (cameras[i] == NULL) {
			cam = open_camera(brightness, height, width);
			if (cam != NULL) {
			    cam->height = height;
			    cam->width =width;
				cameras[i] = cam;
				handle = i;
				break;
			} else {
				break;
			}
		}
	}
	if (i == NUM_CAMERA_HANDLES) {
		PyErr_SetString(FleaError, "No camera handles available");
		return NULL;
	}
	if (handle < 0) {
		PyErr_SetString(FleaError, "Failed to open device");
		return NULL;
	}

    return PyLong_FromLong(handle);
}

static PyObject *
flea_close(PyObject *self, PyObject *args)
{
    int handle = -1;
    fleaCamera* camera;
    if (!PyArg_ParseTuple(args, "i", &handle))
    {
        return NULL;
    }
    
  	if (handle >= 0 && handle < NUM_CAMERA_HANDLES && cameras[handle]) {
		camera = cameras[handle];
	} else {
		PyErr_SetString(FleaError, "Invalid handle");
		return NULL;
	}

    close_camera(camera);
    cameras[handle] = NULL;
    Py_RETURN_NONE;
}

static PyObject *
flea_trigger(PyObject *self, PyObject *args)
{
	int handle = -1;
	fleaCamera* cam = NULL;
	//bool continuous;
	PyObject *continuous_obj;

	if (!PyArg_ParseTuple(args, "iO", &handle, &continuous_obj))
	{
		return NULL;
    }
    
    //This is a trigger mode setting in the chameleon, not sure if we'll use it or not.
	//continuous = PyObject_IsTrue(continuous_obj);

	if (handle >= 0 && handle < NUM_CAMERA_HANDLES && cameras[handle]) {
		cam = cameras[handle];
	} else {
		PyErr_SetString(FleaError, "Invalid handle");
		return NULL;
	}
    trigger(cam);
    
    Py_RETURN_NONE;
}

static PyObject * flea_capture(PyObject *self, PyObject *args)
{
	int handle = -1;
	int timeout_ms = 0;
	fleaCamera* cam = NULL;
	PyArrayObject* array = NULL;

	if (!PyArg_ParseTuple(args, "iiO", &handle, &timeout_ms, &array))
	{
		return NULL;
    }
	CHECK_CONTIGUOUS(array);

	if (handle >= 0 && handle < NUM_CAMERA_HANDLES && cameras[handle]) {
		cam = cameras[handle];
	} else {
		PyErr_SetString(FleaError, "Invalid handle");
		return NULL;
	}

	int ndim = PyArray_NDIM(array);
	if (ndim != 2){
		PyErr_SetString(FleaError, "Array has invalid number of dimensions");
		return NULL;
	}
	
	int w = PyArray_DIM(array, 1);
	int h = PyArray_DIM(array, 0);
	//int stride = PyArray_STRIDE(array, 0);
	//printf("w=%d, h=%d, stride=%d\n", w,h,stride);
	if (w != cam->width || h != cam->height){
        char description[80];
	    sprintf(description,"Invalid array dimensions. Should be %ux%u.  Found %ux%u", cam->width, cam->height, w, h);
		PyErr_SetString(FleaError, description);
		return NULL;
	}

	void* buf = PyArray_DATA(array);
	float frame_time=0;
	uint32_t frame_counter=0;

    //Just doing trigger for now till we understand the camera module better
    capture(cam, buf, &frame_time);
	
	return Py_BuildValue("flf", 
			     frame_time, 
			     (long)frame_counter,
			     shutters[handle]);
	
    
}

static PyObject *
flea_set_gamma(PyObject *self, PyObject *args)
{
	int handle = -1;
	float gamma=0;
	fleaCamera* cam = NULL;

	if (!PyArg_ParseTuple(args, "if", &handle, &gamma))
		return NULL;

	if (handle >= 0 && handle < NUM_CAMERA_HANDLES && cameras[handle]) {
		cam = cameras[handle];
		set_gamma(cam, gamma);
	} else {
		PyErr_SetString(FleaError, "Invalid handle");
		return NULL;
	}


	Py_RETURN_NONE;
}

static PyObject *
flea_get_gamma(PyObject *self, PyObject *args)
{
	int handle = -1;
	fleaCamera* cam = NULL;
	float gamma;

	if (!PyArg_ParseTuple(args, "i", &handle))
		return NULL;

	if (handle >= 0 && handle < NUM_CAMERA_HANDLES && cameras[handle]) {
		cam = cameras[handle];
		gamma = get_gamma(cam);
	} else {
		PyErr_SetString(FleaError, "Invalid Handle");
		return NULL;
	}

	return Py_BuildValue("f",gamma);
}

static PyObject *
flea_get_brightness(PyObject *self, PyObject *args)
{
	int handle = -1;
	fleaCamera* cam = NULL;
	float brightness;

	if (!PyArg_ParseTuple(args, "i", &handle))
		return NULL;

	if (handle >= 0 && handle < NUM_CAMERA_HANDLES && cameras[handle]) {
		cam = cameras[handle];
		brightness = get_brightness(cam);
	} else {
		PyErr_SetString(FleaError, "Invalid Handle");
		return NULL;
	}

	return Py_BuildValue("f", brightness);
}



static PyObject *
flea_get_auto_setting(PyObject *self, PyObject *args)
{
	int handle = -1;
	fleaCamera* cam = NULL;
	const char* prop_name;
	fleaProperty prop;

	if (!PyArg_ParseTuple(args, "is", &handle, &prop_name))
		return NULL;

	if (handle >= 0 && handle < NUM_CAMERA_HANDLES && cameras[handle]) {
		cam = cameras[handle];
		if (strcmp(prop_name, "exposure") == 0) {
		    prop = get_exposure(cam);
		}
		else if (strcmp(prop_name, "shutter") == 0) {
		    prop = get_shutter(cam);
		}
		else if (strcmp(prop_name, "gain") == 0) {
		    prop = get_gain(cam);
		}
		else {
		    return NULL;
   	  	}
	} else {
		PyErr_SetString(FleaError, "Invalid Handle");
		return NULL;
	}

	return Py_BuildValue("iif", prop.autoMode, prop.on, prop.value);

}

static PyObject *
flea_set_framerate(PyObject *self, PyObject *args)
{
/*
	int handle = -1;
	int framerate=0;
	fleaCamera* cam = NULL;

	if (!PyArg_ParseTuple(args, "ii", &handle, &framerate))
		return NULL;

	if (handle >= 0 && handle < NUM_CAMERA_HANDLES && cameras[handle]) {
		cam = cameras[handle];
	} else {
		PyErr_SetString(FleaError, "Invalid handle");
		return NULL;
	}
*/
	Py_RETURN_NONE;
}

static PyObject *
flea_set_auto_exposure(PyObject *self, PyObject *args)
{
	int handle = -1;
	int autoMode = 0;
	int onOff = 0;
	float value = 0.0;
	fleaCamera* cam = NULL;

	if (!PyArg_ParseTuple(args, "iiif", &handle, &autoMode, &onOff, &value))
		return NULL;

	if (handle >= 0 && handle < NUM_CAMERA_HANDLES && cameras[handle]) {
		cam = cameras[handle];
		set_exposure(cam, autoMode, onOff, value);
	} else {
		PyErr_SetString(FleaError, "Invalid Handle");
		return NULL;
	} 
	Py_RETURN_NONE;
}

static PyObject *
flea_set_auto_shutter(PyObject *self, PyObject *args)
{
	int handle = -1;
	int autoMode = 0;
	int onOff = 0;
	float value = 0.0;
	fleaCamera* cam = NULL;

	if (!PyArg_ParseTuple(args, "iiif", &handle, &autoMode, &onOff, &value))
		return NULL;

	if (handle >= 0 && handle < NUM_CAMERA_HANDLES && cameras[handle]) {
		cam = cameras[handle];
		set_shutter(cam, autoMode, onOff, value);
	} else {
		PyErr_SetString(FleaError, "Invalid Handle");
		return NULL;
	} 
	Py_RETURN_NONE;
}


static PyObject *
flea_set_auto_gain(PyObject *self, PyObject *args)
{
	int handle = -1;
	int autoMode = 0;
	int onOff = 0;
	float value = 0.0;
	fleaCamera* cam = NULL;

	if (!PyArg_ParseTuple(args, "iiif", &handle, &autoMode, &onOff, &value))
		return NULL;

	if (handle >= 0 && handle < NUM_CAMERA_HANDLES && cameras[handle]) {
		cam = cameras[handle];
		set_gain(cam, autoMode, onOff, value);
	} else {
		PyErr_SetString(FleaError, "Invalid Handle");
		return NULL;
	} 
	Py_RETURN_NONE;
}

static PyObject *
flea_set_brightness(PyObject *self, PyObject *args)
{
	int handle = -1;
	float brightness=0.0;
	fleaCamera* cam = NULL;

	if (!PyArg_ParseTuple(args, "if", &handle, &brightness))
		return NULL;

	if (handle >= 0 && handle < NUM_CAMERA_HANDLES && cameras[handle]) {
		cam = cameras[handle];
		set_brightness(cam, brightness);
	} else {
		PyErr_SetString(FleaError, "Invalid handle");
		return NULL;
	}

	Py_RETURN_NONE;
}

/* low level file save routine */
static int _save_file(const char *filename, unsigned size, const char *data)
{
	int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	if (fd == -1) {
		return -1;
	}
	if (write(fd, data, size) != size) {
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

/* low level save routine */
static int _save_pgm(const char *filename, unsigned w, unsigned h, unsigned stride,
		     const char *data)
{
	int fd = open(filename, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	int ret;

	if (fd == -1) {
		return -1;
	}
	dprintf(fd,"P5\n%u %u\n%u\n", 
		w, h, stride==w?255:65535);
	if (__BYTE_ORDER == __LITTLE_ENDIAN && stride == w*2) {
		char *data2 = malloc(w*h*2);
		swab(data, data2, w*h*2);
		ret = write(fd, data2, h*stride);
		free(data2);
	} else {
		ret = write(fd, data, h*stride);
	}
	if (ret != h*stride) {
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}


/*
  save a pgm image 
 */
static PyObject *
save_pgm(PyObject *self, PyObject *args)
{
	int status;
	const char *filename;
	unsigned w, h, stride;
	PyArrayObject* array = NULL;

	if (!PyArg_ParseTuple(args, "sO", &filename, &array))
		return NULL;

	CHECK_CONTIGUOUS(array);

	w = PyArray_DIM(array, 1);
	h = PyArray_DIM(array, 0);
	stride = PyArray_STRIDE(array, 0);

	Py_BEGIN_ALLOW_THREADS;
	status = _save_pgm(filename, w, h, stride, PyArray_DATA(array));
	Py_END_ALLOW_THREADS;
	if (status != 0) {
		PyErr_SetString(FleaError, "pgm save failed");
		return NULL;
	}
	Py_RETURN_NONE;
}

/*
  save a file from a python string
 */
static PyObject *
save_file(PyObject *self, PyObject *args)
{
	int status;
	const char *filename;
	PyByteArrayObject *obj;
	char *data;
	unsigned size;

	if (!PyArg_ParseTuple(args, "sO", &filename, &obj))
		return NULL;
	if (!PyString_Check(obj))
		return NULL;

	data = PyString_AS_STRING(obj);
	size = PyString_GET_SIZE(obj);

	Py_BEGIN_ALLOW_THREADS;
	status = _save_file(filename, size, data);
	Py_END_ALLOW_THREADS;
	if (status != 0) {
		PyErr_SetString(FleaError, "file save failed");
		return NULL;
	}
	Py_RETURN_NONE;
}


static PyMethodDef FleaMethods[] = {
  {"open", flea_open, METH_VARARGS, "Open a gnat like device. Returns handle"},
  {"close", flea_close, METH_VARARGS, "Closes camera connection and destroys context.  Returns Nothing"},
  {"trigger", flea_trigger, METH_VARARGS, "Sends a software trigger and captures an image"},
  {"capture", flea_capture, METH_VARARGS, "Sends a software trigger and captures an image"},
  {"save_pgm", save_pgm, METH_VARARGS, "save to a PGM"},
  {"save_file", save_file, METH_VARARGS, "save to a file from a pystring"},
  {"set_gamma", flea_set_gamma, METH_VARARGS, "set gamma"},
  {"set_framerate", flea_set_framerate, METH_VARARGS, "set framerate"},
  {"set_brightness", flea_set_brightness, METH_VARARGS, "set brightness"},
  {"set_auto_exposure", flea_set_auto_exposure, METH_VARARGS, "set exposure"},
  {"set_auto_shutter", flea_set_auto_shutter, METH_VARARGS, "set shutter"},
  {"set_auto_gain", flea_set_auto_gain, METH_VARARGS, "set gain"},
  {"get_gamma", flea_get_gamma, METH_VARARGS, "Get the gamma value from the camera"},
  {"get_brightness", flea_get_brightness, METH_VARARGS, "Get the brightness value from the camera"},
  {"get_auto_setting", flea_get_auto_setting, METH_VARARGS, "Get auto settings based on property name"},
  {NULL, NULL, 0, NULL}  
};

PyMODINIT_FUNC
initflea(void)
{
  PyObject *m;

  m = Py_InitModule("flea", FleaMethods);
  if (m == NULL)
    return;

    //required for numpy.  Put back in if we need numpy.
  //import_array();

    //We're trying to look like the chameleon module to the calling module...
  FleaError = PyErr_NewException("flea.error", NULL, NULL);
  Py_INCREF(FleaError);
  PyModule_AddObject(m, "error", FleaError);
}

