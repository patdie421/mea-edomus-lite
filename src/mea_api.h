//
//  mea_api.h
//
//  Created by Patrice Dietsch on 09/05/13.
//
//

#ifndef mea_api_h
#define mea_api_h

#ifdef __APPLE__
#include <Python/Python.h>
#else
#include <Python.h>
#endif

void mea_api_init(void);
void mea_api_release(void);

/*
static PyObject *mea_api_getMemory(PyObject *self, PyObject *args);

static PyObject *mea_sendAtCmdAndWaitResp(PyObject *self, PyObject *args);
static PyObject *mea_sendAtCmd(PyObject *self, PyObject *args);
PyObject *mea_sendEnoceanPacketAndWaitResp(PyObject *self, PyObject *args);

static PyObject *mea_xplSendMsg(PyObject *self, PyObject *args);
static PyObject *mea_xplGetVendorID();
static PyObject *mea_xplGetDeviceID();
static PyObject *mea_xplGetInstanceID();
static PyObject *mea_enoceanCRC(PyObject *self, PyObject *args);

static PyObject *mea_addDataToSensorsValuesTable(PyObject *self, PyObject *args);

static PyObject *mea_write(PyObject *self, PyObject *args);
static PyObject *mea_read(PyObject *self, PyObject *args);
*/

//PyObject *mea_xplMsgToPyDict(xPL_MessagePtr xplMsg);

#endif
