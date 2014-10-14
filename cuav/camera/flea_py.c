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
	
	//Trying to stick to the chameleon interface.  Deal with the extra parameters later
	if (!PyArg_ParseTuple(args, "OHH", &colour_obj, &depth, &brightness))
	{
        return NULL;	
    }

	for (i = 0; i < NUM_CAMERA_HANDLES; ++i) {
		if (cameras[i] == NULL) {
			cam = open_camera();
			if (cam != NULL) {
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
	int status;
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
	int stride = PyArray_STRIDE(array, 0);
	//printf("w=%d, h=%d, stride=%d\n", w,h,stride);
	if (w != 1280 || h != 960){
		PyErr_SetString(FleaError, "Invalid array dimensions should be 960x1280");
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
	int gamma=0;
	fleaCamera* cam = NULL;

	if (!PyArg_ParseTuple(args, "ii", &handle, &gamma))
		return NULL;

	if (handle >= 0 && handle < NUM_CAMERA_HANDLES && cameras[handle]) {
		cam = cameras[handle];
	} else {
		PyErr_SetString(FleaError, "Invalid handle");
		return NULL;
	}


	Py_RETURN_NONE;
}

static PyObject *
flea_set_framerate(PyObject *self, PyObject *args)
{
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

	Py_RETURN_NONE;
}

static PyObject *
flea_set_brightness(PyObject *self, PyObject *args)
{
	int handle = -1;
	int brightness=0;
	fleaCamera* cam = NULL;

	if (!PyArg_ParseTuple(args, "ii", &handle, &brightness))
		return NULL;

	if (handle >= 0 && handle < NUM_CAMERA_HANDLES && cameras[handle]) {
		cam = cameras[handle];
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

