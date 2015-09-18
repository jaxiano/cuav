#include <Python.h>
#include <numpy/arrayobject.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "ids_lib.h"

static PyObject *IdsError;

#ifndef Py_RETURN_NONE
#define Py_RETURN_NONE return Py_INCREF(Py_None), Py_None
#endif

#define CHECK_CONTIGUOUS(a) do { if (!PyArray_ISCONTIGUOUS(a)) { \
	PyErr_SetString(IdsError, "array must be contiguous"); \
	return NULL; \
	}} while (0)


static PyObject *
ids_open(PyObject *self, PyObject *args)
{
	unsigned int height = 2048, width = 2048;
	
	//Trying to stick to the chameleon interface.  Deal with the extra parameters later
	if (!PyArg_ParseTuple(args, "II", &height, &width))
	{
        return NULL;	
    }
    
	int nRet = open_camera(height, width);

	return Py_BuildValue("i", nRet);
}

static PyObject *
ids_close(PyObject *self, PyObject *args)
{
    close_camera();

    Py_RETURN_NONE;
}

static PyObject * ids_capture(PyObject *self, PyObject *args)
{
    PyStringObject *save;

	if (!PyArg_ParseTuple(args, "S", &save))
	{
		return NULL;
    }
	char *filename = PyString_AS_STRING(save);

    //Just doing trigger for now till we understand the camera module better
    capture(filename);
	
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
		PyErr_SetString(IdsError, "pgm save failed");
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
		PyErr_SetString(IdsError, "file save failed");
		return NULL;
	}
	Py_RETURN_NONE;
}


static PyMethodDef IdsMethods[] = {
  {"open", ids_open, METH_VARARGS, "Open a gnat like device. Returns handle"},
  {"close", ids_close, METH_VARARGS, "Closes camera connection and destroys context.  Returns Nothing"},
  {"capture", ids_capture, METH_VARARGS, "Sends a software trigger and captures an image"},
  {"save_pgm", save_pgm, METH_VARARGS, "save to a PGM"},
  {"save_file", save_file, METH_VARARGS, "save to a file from a pystring"},
  {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC
initlibids(void)
{
  PyObject *m;

  m = Py_InitModule("libids", IdsMethods);
  if (m == NULL)
    return;

    //required for numpy.  Put back in if we need numpy.
  //import_array();

    //We're trying to look like the chameleon module to the calling module...
  IdsError = PyErr_NewException("ids.error", NULL, NULL);
  Py_INCREF(IdsError);
  PyModule_AddObject(m, "error", IdsError);
}

