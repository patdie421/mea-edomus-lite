//
//  python_utils.h
//  mea-eDomus
//
//  Created by Patrice Dietsch on 04/06/13.
//
//

#ifndef mea_eDomus_python_utils_h
#define mea_eDomus_python_utils_h

#include "cJSON.h"
#include "interfacesServer.h"

#define mea_python_lock() \
   { \
   pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL); \
   PyEval_AcquireLock(); \
   PyThreadState *__mainThreadState=PyThreadState_Get(); \
   PyThreadState *__myThreadState = PyThreadState_New(__mainThreadState->interp); \
   PyThreadState *__tempState = PyThreadState_Swap(__myThreadState)

#define mea_python_unlock() \
   PyThreadState_Swap(__tempState); \
   PyThreadState_Clear(__myThreadState); \
   PyThreadState_Delete(__myThreadState); \
   PyEval_ReleaseLock(); \
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL); \
   }


PyObject *mea_getMemory(PyObject *self, PyObject *args, PyObject *mea_memory);

void mea_addLong_to_pydict(PyObject *data_dict, char *key, long value);
void mea_addDouble_to_pydict(PyObject *data_dict, char *key, double value);
void mea_addString_to_pydict(PyObject *data_dict, char *key, char *value);
void mea_addpydict_to_pydict(PyObject *data_dict, char *key, PyObject *adict);

PyObject *mea_jsonToPyDict(cJSON *j);

PyObject *mea_device_info_to_pydict_device(struct device_info_s *device_info);

//PyObject *mea_xplMsgToPyDict(xPL_MessagePtr xplMsg);
PyObject *mea_xplMsgToPyDict2(cJSON *xplMsgJson);

int mea_call_python_function(char *plugin_name, char *plugin_func, PyObject *plugin_params_dict);
int mea_call_python_function2(PyObject *pFunc, PyObject *plugin_params_dict);
int mea_call_python_function3(PyObject *pFunc, PyObject *plugin_params_dict, PyObject **res);

int mea_call_python_function_from_module(PyObject *module, char *plugin_func, PyObject *plugin_params_dict);

#endif
