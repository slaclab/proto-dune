//////////////////////////////////////////////////////////////////////////////
// This file is part of 'LLRF Test Software'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'LLRF Test Software', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
//////////////////////////////////////////////////////////////////////////////
#include <Python.h>
#include <ControlCmdMem.h>
#include <DataSharedMem.h>
#include <time.h>

static ControlCmdMemory * cmem;
static DataSharedMemory * dmem;
static PyObject         * DaqError;
static bool               toDisable;
static uint32_t           rdCount;
static uint32_t           rdAddr;

static PyObject *intSendCmd (const char type, const char *argA, const char *argB, bool retString) {
   time_t    ctme;
   time_t    stme;
   char      result[CONTROL_CMD_STR_SIZE];
   uint32_t  ret;

   if ( cmem == NULL ) return(NULL);

   // Send command
   controlCmdSetCommand(cmem,type,argA,argB);

   // Get time
   time(&stme);

   // Wait for ack with timeout 
   while (! controlCmdGetResult(cmem,result)) {
      usleep(1);
      time(&ctme);

      // Timeout after 10 seconds
      if ( (ctme - stme) >= 10) {
         PyErr_SetString(DaqError,"Timeout sending xml string");
         return(NULL);
      }
   }
   usleep(100);

   // Check error buffer
   if ( strlen(controlCmdGetError(cmem)) != 0 ) {
      PyErr_SetString(DaqError,controlCmdGetError(cmem));
      return(NULL);
   }
   usleep(100);

   // Success
   if ( retString ) return(Py_BuildValue("s",result));
   else if ( strlen(result) == 0 ) return(Py_BuildValue("i",1));
   else {
      ret = (uint32_t)strtoul(result,NULL,0);
      return(Py_BuildValue("i",ret));
   }
}

static PyObject *intSendXml (const char *xml ) {
   return(intSendCmd (CONTROL_CMD_TYPE_SEND_XML,xml, NULL, false));
}

static PyObject *daqHardReset (PyObject *self, PyObject *args) {
   return (intSendXml("<system><command><HardReset/></command></system>\n"));
}

static PyObject *daqSoftReset (PyObject *self, PyObject *args) {
   return (intSendXml("<system><command><SoftReset/></command></system>\n"));
}

static PyObject *daqRefreshstate(PyObject *self, PyObject *args) {
   return (intSendXml("<system><command><RefreshState/></command></system>\n"));
}

static PyObject *daqSetDefaults (PyObject *self, PyObject *args) {
   return (intSendXml("<system><command><SetDefaults/></command></system>\n"));
}

static PyObject *daqLoadSettings (PyObject *self, PyObject *args) {
   char         buffer[1024];
   const char * arg;

   if (!PyArg_ParseTuple(args, "s", &arg)) return NULL;

   sprintf(buffer,"<system><command><ReadXmlFile>%s</ReadXmlFile></command></system>\n",arg);
   return (intSendXml(buffer));
}

static PyObject *daqSaveSettings (PyObject *self, PyObject *args) {
   char         buffer[1024];
   const char * arg;

   if (!PyArg_ParseTuple(args, "s", &arg)) return NULL;

   sprintf(buffer,"<system><command><WriteConfigXml>%s</WriteConfigXml></command></system>\n",arg);
   return (intSendXml(buffer));
}

static PyObject *daqOpenData (PyObject *self, PyObject *args) {
   char         buffer[1024];
   const char * arg;

   if (!PyArg_ParseTuple(args, "s", &arg)) return NULL;

   sprintf(buffer,"<system><config><DataFile>%s</DataFile></config><command><OpenDataFile/></command></system>\n",arg);
   return (intSendXml(buffer));
}

static PyObject *daqCloseData (PyObject *self, PyObject *args) {
   return (intSendXml("<system><command><CloseDataFile/></command></system>\n"));
}

static PyObject *daqSetRunParameters (PyObject *self, PyObject *args) {
   char         buffer[1024];
   const char * rate;
   int          count;

   if (!PyArg_ParseTuple(args, "si", &rate,&count)) return NULL;

   sprintf(buffer,"<system><config><RunRate>%s</RunRate><RunCount>%i</RunCount></config></system>\n",rate,count);
   return (intSendXml(buffer));
}

static PyObject *daqSetRunState (PyObject *self, PyObject *args) {
   char         buffer[1024];
   const char * state;

   if (!PyArg_ParseTuple(args, "s", &state)) return NULL;

   sprintf(buffer,"<system><command><SetRunState>%s</SetRunState></command></system>\n",state);
   return (intSendXml(buffer));
}

static PyObject *daqRunState (PyObject *self, PyObject *args) {
   return(intSendCmd (CONTROL_CMD_TYPE_GET_STATUS, "RunState", NULL, true));
}

static PyObject *daqResetCounters (PyObject *self, PyObject *args) {
   return (intSendXml("<system><command><ResetCount/></command></system>\n"));
}

static PyObject *daqSendCommand (PyObject *self, PyObject *args) {
   const char *cmd;
   const char *arg;

   if (!PyArg_ParseTuple(args, "ss", &cmd,&arg)) return NULL;

   return(intSendCmd (CONTROL_CMD_TYPE_EXEC_COMMAND, cmd, arg, false));
}

static PyObject *daqReadStatus (PyObject *self, PyObject *args) {
   return (intSendXml("<system><command><ReadStatus/></command></system>\n"));
}

static PyObject *daqGetStatus (PyObject *self, PyObject *args) {
   const char   *var;

   if (!PyArg_ParseTuple(args, "s", &var)) return NULL;

   return(intSendCmd (CONTROL_CMD_TYPE_GET_STATUS, var, NULL, true));
}

static PyObject *daqReadConfig (PyObject *self, PyObject *args) {
   return (intSendXml("<system><command><ReadConfig/></command></system>\n"));
}

static PyObject *daqVerifyConfig (PyObject *self, PyObject *args) {
   return (intSendXml("<system><command><VerifyConfig/></command></system>\n"));
}

static PyObject *daqSetConfig (PyObject *self, PyObject *args) {
   const char   *var;
   const char   *arg;

   if (!PyArg_ParseTuple(args, "ss", &var,&arg)) return NULL;

   return(intSendCmd (CONTROL_CMD_TYPE_SET_CONFIG, var, arg, false));
}

static PyObject *daqGetConfig (PyObject *self, PyObject *args) {
   const char   *var;

   if (!PyArg_ParseTuple(args, "s", &var)) return NULL;

   return(intSendCmd (CONTROL_CMD_TYPE_GET_CONFIG, var, NULL, true));
}

static PyObject *daqGetError (PyObject *self, PyObject *args) {
   if ( cmem == NULL ) return(NULL);
   return(Py_BuildValue("s",controlCmdGetError(cmem)));
}

static PyObject *daqSendXml (PyObject *self, PyObject *args) {
   const char *xml;

   if (!PyArg_ParseTuple(args, "s", &xml)) return NULL;

   return (intSendXml(xml));
}

static PyObject *daqDisableTimeout (PyObject *self, PyObject *args) {
   toDisable = true;
   return(Py_BuildValue("i",1));
}

static PyObject *daqWriteRegister (PyObject *self, PyObject *args) {
   char          buffer[1024];
   const char *  dev;
   const char *  reg;
   uint32_t      val;

   if (!PyArg_ParseTuple(args, "ssi",&dev,&reg,&val)) return NULL;

   sprintf(buffer,"%s 0x%x",reg,val);
   return(intSendCmd (CONTROL_CMD_TYPE_SET_REGISTER, dev, buffer, false));
}

static PyObject *daqReadRegister (PyObject *self, PyObject *args) {
   const char   *dev;
   const char   *reg;

   if (!PyArg_ParseTuple(args, "ss",&dev,&reg)) return NULL;

   return(intSendCmd (CONTROL_CMD_TYPE_GET_REGISTER, dev, reg, false));
}

static PyObject *daqOpen (PyObject *self, PyObject *args) {
   uint32_t     id;
   const char * system;

   if (!PyArg_ParseTuple(args, "si", &system, &id)) return NULL;

   /* Init shared memory */
   controlCmdOpenAndMap(&cmem,system,id);

   return(Py_BuildValue("i",1));
}

static PyObject *daqSharedDataOpen (PyObject *self, PyObject *args) {
   uint32_t     id;
   int32_t      ret;
   const char * system;

   if (!PyArg_ParseTuple(args, "si", &system, &id)) return NULL;

   /* Init shared memory */
   ret = dataSharedOpenAndMap(&dmem,system,id);
   rdCount = 0;
   rdAddr  = 0;

   printf("open dmem=%x\n",dmem);

   return(Py_BuildValue("i",ret));
}

static PyObject *daqSharedDataRead (PyObject *self, PyObject *args) {
   uint8_t  * data;
   uint32_t   ret;
   uint32_t   flag;
   uint32_t   count;
   uint32_t   type;
   uint32_t * idata;
   uint32_t   icount;
   uint32_t   i;

   ret = dataSharedRead(dmem,&rdAddr,&rdCount,&flag,&data);

   type  = (flag >> 28) & 0xF;
   count = flag & 0x0FFFFFFF;

   PyObject* tupleA = PyTuple_New(3); 

   if ( ret == 0 ) {
      PyTuple_SetItem(tupleA,0,Py_BuildValue("i",0));
      PyTuple_SetItem(tupleA,1,Py_BuildValue("i",0));
   } else {
      PyTuple_SetItem(tupleA,0,Py_BuildValue("i",count));
      PyTuple_SetItem(tupleA,1,Py_BuildValue("i",type));
   }

   if ( ret == 0 ) {
      PyTuple_SetItem(tupleA,2,Py_BuildValue("i",0));
   }
   else if ( type == 0 ) {
      idata  = (uint32_t *)data;
      icount = count;

      PyObject* tupleB = PyTuple_New(icount); 

      for (i=0; i < icount; i++) PyTuple_SetItem(tupleB,i,Py_BuildValue("I",idata[i]));

      PyTuple_SetItem(tupleA,2,tupleB);
   } else {
      data[count] = 0;
      PyTuple_SetItem(tupleA,2,Py_BuildValue("s",data));
   }

   return tupleA; 
}

static PyMethodDef DaqMethods[] = {
   {"daqOpen",             daqOpen,             METH_VARARGS, ""},
   {"daqHardReset",        daqHardReset,        METH_VARARGS, ""},
   {"daqSoftReset",        daqSoftReset,        METH_VARARGS, ""},
   {"daqRefreshState",     daqRefreshstate,     METH_VARARGS, ""},
   {"daqSetDefaults",      daqSetDefaults,      METH_VARARGS, ""},
   {"daqLoadSettings",     daqLoadSettings,     METH_VARARGS, ""},
   {"daqSaveSettings",     daqSaveSettings,     METH_VARARGS, ""},
   {"daqOpenData",         daqOpenData,         METH_VARARGS, ""},
   {"daqCloseData",        daqCloseData,        METH_VARARGS, ""},
   {"daqSetRunParameters", daqSetRunParameters, METH_VARARGS, ""},
   {"daqSetRunState",      daqSetRunState,      METH_VARARGS, ""},
   {"daqGetRunState",      daqRunState,         METH_VARARGS, ""},
   {"daqResetCounters",    daqResetCounters,    METH_VARARGS, ""},
   {"daqSendCommand",      daqSendCommand,      METH_VARARGS, ""},
   {"daqReadStatus",       daqReadStatus,       METH_VARARGS, ""},
   {"daqGetStatus",        daqGetStatus,        METH_VARARGS, ""},
   {"daqReadConfig",       daqReadConfig,       METH_VARARGS, ""},
   {"daqVerifyConfig",     daqVerifyConfig,     METH_VARARGS, ""},
   {"daqSetConfig",        daqSetConfig,        METH_VARARGS, ""},
   {"daqGetConfig",        daqGetConfig,        METH_VARARGS, ""},
   {"daqGetError",         daqGetError,         METH_VARARGS, ""},
   {"daqSendXml",          daqSendXml,          METH_VARARGS, ""},
   {"daqDisableTimeout",   daqDisableTimeout,   METH_VARARGS, ""},
   {"daqReadRegister",     daqReadRegister,     METH_VARARGS, ""},
   {"daqWriteRegister",    daqWriteRegister,    METH_VARARGS, ""},
   {"daqSharedDataOpen",   daqSharedDataOpen,   METH_VARARGS, ""},
   {"daqSharedDataRead",   daqSharedDataRead,   METH_VARARGS, ""},
   {NULL,                  NULL,                0,            NULL} /* Sentinel */
};

PyMODINIT_FUNC initpythonDaq(void) {
   PyObject *m;
   m = Py_InitModule("pythonDaq", DaqMethods);

   DaqError = PyErr_NewException("Daq.error",NULL,NULL);
   Py_INCREF(DaqError);
   PyModule_AddObject(m,"error",DaqError);

   toDisable = false;
   cmem = NULL;
}

