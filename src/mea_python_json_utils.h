//
//  python_utils.h
//  mea-eDomus
//
//  Created by Patrice Dietsch on 04/06/13.
//
//
#ifdef __APPLE__
#include <Python/Python.h>
#else
#include <Python.h>
#endif

#ifndef mea_python_json_utils_h
#define mea_python_json_utils_h

#include "cJSON.h"
#include "interfacesServer.h"


#if PY_MAJOR_VERSION >= 3
#define PYSTRING_FROMSTRING PyBytes_FromString
#define PYSTRING_ASSTRING   PyBytes_AsString
#define PYINT_ASLONG        PyLong_AsLong
#define PYSTRING_CHECK      PyBytes_Check
#define PYBUFFER_NEW        PyBytes_New
#else
#define PYSTRING_FROMSTRING PyString_FromString
#define PYSTRING_ASSTRING   PyString_AsString
#define PYINT_ASLONG        PyInt_AsLong
#define PYSTRING_CHECK      PyString_Check
#define PYBUFFER_NEW        PyBuffer_New
#endif


cJSON    *mea_PyObjectToJson(PyObject *p);
PyObject *mea_jsonToPyObject(cJSON *j);

#endif
