#include <Python.h>
#include "fleatest_lib.h"

static PyObject *FleaError;

static PyObject *
flea_open(PyObject *self, PyObject *args)
{
    int ret = 12;
    
    return PyLong_FromLong(ret);
}

static PyMethodDef FleaMethods[] = {
  {"open", flea_open, METH_VARARGS, "Open a gnat like device. Returns handle"},
  {NULL, NULL, 0, NULL}        /* Terminus */
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

  FleaError = PyErr_NewException("flea.error", NULL, NULL);
  Py_INCREF(FleaError);
  PyModule_AddObject(m, "error", FleaError);
}

