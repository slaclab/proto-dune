//-----------------------------------------------------------------------------
// File          : DataBuffer.h
// Author        : Ryan Herbst <rherbst@slac.stanford.edu>
// Created       : 06/19/2014
// Project       : LBNE DAQ
//-----------------------------------------------------------------------------
// Description :
//    LBNE Data buffer Control
//-----------------------------------------------------------------------------
// This file is part of 'DUNE Development Software'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'DUNE Development Software', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------


#ifdef ARM

//-----------------------------------------------------------------------------
// Modification history :
//
//       DATE WHO WHAT
// ---------- --- -------------------------------------------------------
// 2017.06.19 jjr Updated to use new definition of RunMode
// 2016.10.28 jjr Added the triggering configuration parameters naccept
//                and nframe
//                Creation date, unknown
//-----------------------------------------------------------------------------

#include <DataBuffer.h>
#include <DaqBuffer.h>
#include <Variable.h>
#include <vector>
#include <string>
#include <stdint.h>
using namespace std;


/* ====================================================================== */
/* DataBufffer                                                            */
/* ---------------------------------------------------------------------- *//*!
 *
 * \brief  DataBuffer Constructor
 *
\* ---------------------------------------------------------------------- */
DataBuffer::DataBuffer (uint32_t linkConfig, 
                        uint32_t      index,
                        Device      *parent) : 
   Device     (linkConfig, 0, "DataBuffer", index, parent),
   daqBuffer_ (new DaqBuffer()),
   sv_        (this),
   cv_        (this)
 {
    static const string sEnabled ("Enabled");
    Variable     *v;

    // Description
    desc_ = "Data Buffer.";

    // Open DMA Interface 
    if ( ! daqBuffer_->open ("/dev/axi_stream_dma_2") ) 
    {
       fprintf (stderr,"\n\n!!!!!! Failed to open dma device !!!!!!!\n\n");
    }


    // Set enable and hide enable control
    v = getVariable (sEnabled);
    v->set          ("True");
    v->setHidden    (true);


   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 * \brief  DataBuffer Deconstructor
 *
\* ---------------------------------------------------------------------- */
DataBuffer::~DataBuffer () 
{  
   delete daqBuffer_; 
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!
 *
 * \brief  Command Processor
 *
 * \param[in]  name  The command verb
 * \param[in]   arg  The command arguments
 *
\* ---------------------------------------------------------------------- */
void DataBuffer::command (string name, string arg) 
{
   printf ("DataBuffer::executing command:%s args:%s\n", 
           name.c_str(), 
           arg.c_str());
   Device::command(name, arg);
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 * \brief  Perform a hard reset.  This resets both the hardware and 
 *         software contexts
 *
\* ---------------------------------------------------------------------- */
void DataBuffer::hardReset () 
{
   Variable *v;
   v = getVariable("BlowOffDmaData"); v->set("False");
   v = getVariable("BlowOffTxEth");   v->set("False");
   v = getVariable("PreTrigger");     v->setInt(2500);
   v = getVariable("Duration");       v->setInt(5000);
   v = getVariable("DaqHost");        v->set("");
   v = getVariable("DaqPort");        v->set("");
   v = getVariable("RunMode");        v->set("Idle"); 

   daqBuffer_->setRunMode   (RunMode::IDLE);
   daqBuffer_->hardReset    ();
   daqBuffer_->disableTx    ();
   daqBuffer_->resetCounters();

   Device::hardReset();
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 * \brief  Perform a soft reset.  This resets only the software context.
 *
\* ---------------------------------------------------------------------- */
void DataBuffer::softReset () 
{
   daqBuffer_->resetCounters();

   Device::softReset();
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 * \brief  Reset both the hardware and software counters
 *
\* ---------------------------------------------------------------------- */
void DataBuffer::countReset () 
{
   daqBuffer_->resetCounters();

   Device::countReset();
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 * \brief  Disable, aka stop, a run
 *
\* ---------------------------------------------------------------------- */
void DataBuffer::disableRun() 
{
   daqBuffer_->setRunMode (RunMode::IDLE);
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 * \brief Enable a run. 
 *
 * \par
 *  Two actions are performed
 *     -# If a TCP/IP host and port are valid, a socket connection
 *        is attempted to be made and transmission enabled
 *
 *     -# The current run mode (IDLE, SCOPE, BURST, etc) is made active
\* ---------------------------------------------------------------------- */
void DataBuffer::enableRun() 
{
   // --------------------------------------------------------------
   // Retrieve the host name and port of the data destination socket
   // --------------------------------------------------------------
   uint16_t    daqPort = cv_.v[ConfigurationVariables::DaqPort]->getInt ();

   // ------------------------------------------
   // Check that a host and port were designated
   // ------------------------------------------
   if ( daqPort != 0 ) 
   {
      string    s_daqHost = cv_.v[ConfigurationVariables::DaqHost]->get ();
      char const *daqHost = s_daqHost.c_str ();

      fprintf (stderr, "DataBuffer::enableRun -> connecting on %s.%u\n", 
               daqHost, daqPort);


      // -------------------------------------------------
      // Have a port, try enabling transmission
      // This attempts to connect a socket to the listener
      // -------------------------------------------------
      if ( ! daqBuffer_->enableTx (daqHost, daqPort) )
      {
         fprintf(stderr,"DataBuffer::enableRun -> Failed to open socket\n");
      }

      else if ( debug_ ) 
      {
         daqBuffer_->startRun ();
         printf("DataBuffer::enableRun -> Connected to host %s, Port %u\n",
                daqHost, daqPort);
      }
   }
   else 
   {
      // --------------------------------------------------------
      // No destination port was designated, disable transmission
      // --------------------------------------------------------
      daqBuffer_->disableTx();
   }

   // -------------------------------------
   // Retrieve and set the current run mode
   // -------------------------------------
   RunMode runMode = 
      static_cast<RunMode>(cv_.v[ConfigurationVariables::RunMode]->getInt ());
   fprintf (stderr, "DataBuffer::enableRun -> runMode = %u\n", (int)runMode);
   daqBuffer_->setRunMode (runMode);


   return;
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 * \brief Driver to poll and post all devices status registers
 *
\* ---------------------------------------------------------------------- */
void DataBuffer::pollStatus () 
{
   // Do the poll for DataBuffer status
   if (pollEnable_) 
   {
      statusPoll();
   }

   Device::pollStatus();
}
/* ---------------------------------------------------------------------- */



/* ---------------------------------------------------------------------- *//*!
 *
 *  \brief Fetch and reprot the status of the Daq buffers
 *
* ---------------------------------------------------------------------- */
void DataBuffer::statusPoll () {

   struct BufferStatus status;

   daqBuffer_->getStatus (&status);
   sv_.set               ( status);

}
/* ---------------------------------------------------------------------- */


// Method to read status registers and update variables
void DataBuffer::readStatus ( ) {
   struct BufferStatus status;

   daqBuffer_->getStatus (&status);
   sv_.set               ( status);
}


/* ---------------------------------------------------------------------- *//*!
 *
 * \brief Write configuration registers
 *
\* ---------------------------------------------------------------------- */
void DataBuffer::writeConfig ( bool ) 
{
   uint32_t blowOffDmaData = cv_.v[ConfigurationVariables::BlowOffDmaData]->getInt ();
   uint32_t blowOffTxData  = cv_.v[ConfigurationVariables::BlowOffTxEth  ]->getInt ();

   uint32_t pretrigger     = cv_.v[ConfigurationVariables::PreTrigger]->getInt();
   uint32_t duration       = cv_.v[ConfigurationVariables::Duration  ]->getInt();
   uint32_t period         = cv_.v[ConfigurationVariables::Period    ]->getInt();


   daqBuffer_->setConfig (blowOffDmaData, 
                          blowOffTxData, 
                          pretrigger, 
                          duration,
                          period);

  return;
}
/* ---------------------------------------------------------------------- */
/* DataBuffer                                                             */
/* ====================================================================== */





/* ====================================================================== */
/* DataBuffer::StatusVariables                                            */
/* ---------------------------------------------------------------------- *//*!
 *
 * \brief  Constructor for all the DataBuffer status variables
 *
 * \param[in] device  The device to add these variables to
 *
\* ---------------------------------------------------------------------- */
DataBuffer::StatusVariables::StatusVariables (Device *device)
{
   /*
    | The specification for a generic StatusVariable.  This is information
    | that is used mainly by the GUI to locate, name and describe the
    | status variable.
    |
    | The name is used to both locate the variable in the Device tree and
    | as the label to in the GUI display
    */
   struct StatusVariableSpec_s
   {
      char const  *name;  /*!< The name of this status variable */
      char const   *dsc;  /*!< A description of it meaning      */
      char const *units;  /*!< The units that will be displayed */
   };


   /*
    | Describe the status variables
    | Going it this way removes at lot of the 
    | overhead associated with C++ strings
    */
   static const StatusVariableSpec_s Svss[] = 
   {
      [BufferCount] = { "BufferCount", "Buffer Count",                    0 },
      [RxCount]     = { "RxCount",     "Rx Counter",                      0 },
      [RxRate]      = { "RxRate",      "Rx Rate",                       "Hz"},
      [RxBw]        = { "RxBw",        "Rx Bandwidth",                "Mbps"},
      [RxSize]      = { "RxSize",      "Rx Size",                    "Bytes"},
      [RxPend]      = { "RxPend",      "Rx Pend",                         0 },
      [RxErrors]    = { "RxErrors",    "Rx Error Count",                  0 },
      [DropCount]   = { "DropCount",   "Number of frame buffers dropped", 0 },
      [Triggers]    = { "Triggers",    "Trigger Count",                   0 },
      [TriggerRate] = { "TriggerRate", "TriggerRate",                   "Hz"},
      [TxErrors]    = { "TxErrors",    "Transmit Errors",                 0 },
      [TxCount]     = { "TxCount",     "Transmit Count",                  0 },
      [TxSize]      = { "TxSize",      "Transmit Size",              "Bytes"},
      [TxBw]        = { "TxBw",        "Transmit Bandwidth",          "Mbps"},
      [TxRate]      = { "TxRate",      "Transmit Rate",                 "Hz"},
      [TxPend]      = { "TxPend",      "Transmit Pend",                   0 },
      [DisTrgCnt]   = { "DisTrgCnt",   "Discarded Trigger Count",         0 },
      [DropSeqCnt]  = { "DropSeqCnt",  "Drop Sequnece Count",             0 },
      [TrgMsgCnt]   = { "TrgMsgCnt",   "Trigger Message Count",           0 },
   };


   for (unsigned int idx = 0; idx < sizeof (Svss) / sizeof (Svss[0]); idx++)
   {
      string const None ("");

      StatusVariableSpec_s const &svs = Svss[idx];

      Variable *var = new Variable (svs.name, Variable::Status);

      device->addVariable (var);
      var->setDescription (svs.dsc);
      var->setComp        (0, 1, 0, svs.units != NULL ? svs.units : None);

      v[idx] = var;
   }

   return;
}
/* ---------------------------------------------------------------------- */




/* ---------------------------------------------------------------------- *//*!
 *
 * \brief  Transfers the set of status values to the status variables
 *
 * \param[in]  status  The collection of status values to transfer
 *
\* ---------------------------------------------------------------------- */
void DataBuffer::StatusVariables::set (BufferStatus const &status)
{
   static char Format_1f[] =  "%.1f";

   v[BufferCount]->setInt   (status.buffCount);
   v[RxCount    ]->setInt   (status.rxCount);
   v[RxRate     ]->setFloat (status.rxRate, Format_1f);
   v[RxBw       ]->setFloat (status.rxBw,   Format_1f);
   v[RxSize     ]->setInt   (status.rxSize);
   v[RxPend     ]->setInt   (status.rxPend);
   v[RxErrors   ]->setInt   (status.rxErrors);
   v[DropCount  ]->setInt   (status.dropCount);
   v[Triggers   ]->setInt   (status.triggers);
   v[TriggerRate]->setFloat (status.triggerRate, Format_1f);
   v[TxErrors   ]->setInt   (status.txErrors);
   v[TxCount    ]->setInt   (status.txCount);
   v[TxSize     ]->setInt   (status.txSize);
   v[TxPend     ]->setInt   (status.txPend);
   v[TxBw       ]->setFloat (status.txBw,   Format_1f);
   v[TxRate     ]->setFloat (status.txRate, Format_1f);
   v[DisTrgCnt  ]->setInt   (status.disTrgCnt);
   v[DropSeqCnt ]->setInt   (status.dropSeqCnt);
   v[TrgMsgCnt  ]->setInt   (status.trgMsgCnt);
}
/* ---------------------------------------------------------------------- */
/* DataBuffer::StatusVariables                                            */
/* ====================================================================== */




/* ---------------------------------------------------------------------- *//*!
 *
 * \brief  Constructor for all the DataBuffer configuraion variables
 *
 * \param[in] device  The device to add these variables to
 *
\* ---------------------------------------------------------------------- */
DataBuffer::ConfigurationVariables::ConfigurationVariables (Device *device)
{
   static const string sRunMode        ("RunMode");
   static const string sBlowOffDmaData ("BlowOffDmaData");
   static const string sBlowOffTxEth   ("BlowOffTxEth");   
   static const string sPreTrigger     ("PreTrigger");
   static const string sDuration       ("Duration");
   static const string sPeriod         ("Period");
   static const string sDaqHost        ("DaqHost");
   static const string sDaqPort        ("DaqPort");

   static const string sPreTriggerDsc  ("Event: Begins usecs before the trigger");
   static const string sDurationDsc    ("Event: Duration in usecs");
   static const string sPeriodDsc      ("Software trigger period in usecs");


   Variable *var;

   // Variables
   device->addVariable(var = new Variable (sBlowOffDmaData, Variable::Configuration));  
   var->setTrueFalse();
   var->set("False");
   v[BlowOffDmaData] = var;
 

   device->addVariable(var = new Variable (sBlowOffTxEth, Variable::Configuration));  
   var->setTrueFalse();
   var->set("False"); 
   v[BlowOffTxEth] = var;


   // Default event window to 2.5msecs before trigger
   device->addVariable(var = new Variable (sPreTrigger, Variable::Configuration));
   var->setDescription (sPreTriggerDsc);
   var->setBase10      ();
   var->setInt         (2500);
   v[PreTrigger] = var;


   // Default event window duration to 5.0msecs
   device->addVariable(var = new Variable (sDuration, Variable::Configuration));
   var->setDescription (sDurationDsc);
   var->setBase10      ();
   var->setInt         (5000);
   v[Duration] = var;

   // Default software trigger period to 1 sec
   device->addVariable(var = new Variable (sPeriod, Variable::Configuration));
   var->setDescription (sPeriodDsc);
   var->setBase10      ();
   var->setInt         (1000*1000);
   v[Period] = var;

 
   device->addVariable(var = new Variable (sDaqPort, Variable::Configuration));  
   var->setDescription("Port of DAQ Host");
   var->setPerInstance(true);   
   v[DaqPort] = var;
  

   device->addVariable(var = new Variable(sDaqHost, Variable::Configuration));  
   var->setDescription("Address of DAQ Host");
   var->setPerInstance(true);
   v[DaqHost] = var;

 
   device->addVariable(var = new Variable (sRunMode, Variable::Configuration));  
   var->setDescription("Run Mode");

   static char const *RunModeStates[3] = {"Idle", "External", "Software"};
   static const vector<string> states (RunModeStates, 
                                       RunModeStates 
                                     + sizeof ( RunModeStates) 
                                     / sizeof (*RunModeStates));
   var->setEnums(states);
   v[RunMode] = var;

   return;
}
/* ---------------------------------------------------------------------- */

#endif
