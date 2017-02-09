//-----------------------------------------------------------------------------
// File          : CommLink.h
// Author        : Ryan Herbst  <rherbst@slac.stanford.edu>
// Created       : 04/12/2011
// Project       : General Purpose
//-----------------------------------------------------------------------------
// Description :
// Generic communications link
//-----------------------------------------------------------------------------
// This file is part of 'SLAC Generic DAQ Software'.
// It is subject to the license terms in the LICENSE.txt file found in the 
// top-level directory of this distribution and at: 
//    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
// No part of 'SLAC Generic DAQ Software', including this file, 
// may be copied, modified, propagated, or distributed except according to 
// the terms contained in the LICENSE.txt file.
// Proprietary and confidential to SLAC.
//-----------------------------------------------------------------------------
// Modification history :
// 04/12/2011: created
//-----------------------------------------------------------------------------
#ifndef __COMM_LINK_H__
#define __COMM_LINK_H__

#include <string>
#include <sstream>
#include <map>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <CommQueue.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <resolv.h>
#include <stdint.h>

using namespace std;

class Data;
class Register;
class Command;

//! Class to contain generic communications link
class CommLink {

      // Max UDP transfer size
      static const uint32_t MaxUdpSize = 16000;

      // Mutux variable for thread locking
      pthread_mutex_t reqMutex_;

   protected:

      // Debug flag
      bool debug_;

      // Data mask
      uint32_t dataSource_;

      // Data receive queue
      CommQueue dataQueue_;

      // Data file status
      int32_t dataFileFd_;
      string dataFile_;
      uint32_t maxSize_;
      uint32_t fileCount_; 
      uint32_t fileSize_; 
      uint32_t lastWrite_;

      // Data network status
      struct sockaddr_in net_addr_;
      int32_t            dataNetFd_;
      string             dataNetAddress_;
      int32_t            dataNetPort_;

      // Shared memory
      uint32_t smemFd_;
      void *smem_;

      // Data rx callback function
      void (*dataCb_)(void *, uint32_t);

      // Register request/response queue
      Register *regReqEntry_;
      uint32_t  regReqConf_;
      uint32_t  regReqCnt_;
      bool      regReqWrite_;
      uint32_t  regRespCnt_;

      // Command request queue
      Command  *cmdReqEntry_;
      uint32_t  cmdReqConf_;
      uint32_t  cmdReqCnt_;
      uint32_t  cmdRespCnt_;

      // Run Command request queue
      Command  *runReqEntry_;
      uint32_t  runReqConf_;
      uint32_t  runReqCnt_;

      // Data transmit request queue
      uint32_t  *dataReqEntry_;
      uint32_t   dataReqLength_;
      uint32_t   dataReqConf_;
      uint32_t   dataReqAddr_;
      uint32_t   dataReqCnt_;
      uint32_t   dataRespCnt_;

      // Config/Status request queue
      string    xmlReqEntry_;
      uint32_t  xmlType_;
      uint32_t  xmlReqCnt_;
      uint32_t  xmlRespCnt_;

      // Thread pointers
      pthread_t rxThread_;
      pthread_t ioThread_;
      pthread_t dataThread_;

      // Thread Routines
      static void *rxRun ( void *t );
      static void *ioRun ( void *t );
      static void *dataRun ( void *t );

      // Thread condition variables
      pthread_cond_t  ioCondition_;
      pthread_mutex_t ioMutex_;
      pthread_cond_t  dataCondition_;
      pthread_mutex_t dataMutex_;
      pthread_cond_t  mainCondition_;
      pthread_mutex_t mainMutex_;
      pthread_mutex_t fileMutex_;

      // Condition set and wait routines
      void dataThreadWait(uint32_t usec);
      void dataThreadWakeup();
      void ioThreadWait(uint32_t usec);
      void ioThreadWakeup();
      void mainThreadWait(uint32_t usec);
      void mainThreadWakeup();

      // Timer functions
      void initTime(struct timeval *tm);
      uint32_t timePassed(struct timeval *tm, uint32_t usec);

      // Run enable
      bool runEnable_;

      // Config/status/start/stop Store Enable
      bool xmlStoreEn_;

      // Data routine
      bool enDataThread_;
      virtual void dataHandler();

      // Stat counters
      uint32_t   dataFileCount_;
      uint32_t   dataRxCount_;
      uint32_t   regRxCount_;
      uint32_t   timeoutCount_;
      uint32_t   errorCount_;
      uint32_t   unexpCount_;

      // IO handling routines
      virtual void rxHandler();
      virtual void ioHandler();

      // Max RX/Tx size
      uint32_t maxRxTx_;

      // Buffer for pending register transactions
      uint32_t *regBuff_;

      // Timeout disable flag
      bool toDisable_;

      // Buffered write enable
      bool      buffWriteEn_;
      uint32_t  buffWriteSize_;
      uint8_t * buffWriteData_;
      uint32_t  buffWriteTotal_;

      // Internal file re-open
      int32_t checkFileSize();

      // Internal write function
      ssize_t buffWrite(int fd, const void *buf, size_t count);

   public:

      //! Constructor
      CommLink ( );

      //! Deconstructor
      virtual ~CommLink ( );

      //! Open link and start threads
      /*! 
       * Return true on success.
       * Throws string on error.
      */
      virtual void open (bool enDataThread = true);

      //! Stop threads and close link
      virtual void close ();

      //! Open data file
      /*! 
       * Return true on success.
       * Throws string on error.
       * \param file filename to open
       * \param maxSize Maximum data file size. 0 = infinate.
      */
      void openDataFile (string file, uint32_t maxSize);

      //! Close data file
      void closeDataFile ();

      //! Open data network
      /*! 
       * Return true on success.
       * Throws string on error.
       * \param address network address to send data to
       * \param port    network port to send data to
      */
      void openDataNet (string address, int32_t port);

      //! Close data network
      void closeDataNet ();

      //! Set data callback function
      /*!
       * This function is called whenever data is received.
       * The passed function accepts a data pointer and a length
       * value in bytes.
      */
      void setDataCb ( void (*dataCb_)(void *, uint32_t));

      //! Set debug flag
      /*! 
       * \param enable Debug state
      */
      void setDebug( bool enable );

      //! Queue register request
      /*! 
       * Throws string on error.
       * \param linkConfig LinkConfig information
       * \param reg    Register pointer
       * \param write       Write flag
       * \param wait        Wait flag
      */
      void queueRegister ( uint32_t linkConfig, Register *reg, bool write, bool wait );

      //! Queue command request
      /*! 
       * Throws string on error.
       * \param linkConfig LinkConfig information
       * \param cmd     Command pointer
      */
      void queueCommand ( uint32_t linkConfig, Command *cmd );

      //! Queue run command request
      void queueRunCommand ( );

      //! Queue data transmission request
      void queueDataTx ( uint32_t linkConfig, uint32_t address, uint32_t *txBuffer, uint32_t txLength );

      //! Set run command
      /*! 
       * \param linkConfig LinkConfig information
       * \param cmd     Command pointer
      */
      void setRunCommand ( uint32_t linkConfig, Command *cmd );

      //! Get data file count
      uint32_t dataFileCount ();

      //! Get data receive count
      uint32_t   dataRxCount();

      //! Get register rx count
      uint32_t   regRxCount();

      //! Get timeout count
      uint32_t   timeoutCount();

      //! Get error count
      uint32_t   errorCount();

      //! Get unexpcted count
      uint32_t   unexpectedCount();

      //! Clear counters
      void   clearCounters();

      //! Set mask for data reception (deprecated)
      /*! 
       * Set mask for data reception. The mask is implementation specific.
       * \param mask Mask value
      */
      void setDataMask ( uint32_t mask );

      //! Add source for data reception
      /*! 
       * Add source for data reception. The source value is
       * specific to each CommLink sub class.
       * \param source Source value
      */
      virtual void addDataSource ( uint32_t source );

      //! Set max rx/tx size 
      /*! 
       * \param maxRxTx  Maximum receive/transmit size in bytes
      */
      void setMaxRxTx ( uint32_t maxRxTx );

      //! Add configuration to data file
      /*! 
       * \param config Configuration XML data
      */
      void addConfig ( string config );

      //! Add status to data file
      /*! 
       * \param status Status XML data
      */
      void addStatus ( string status );

      //! Add run start to data file
      /*! 
       * \param xml Run start data
      */
      void addRunStart ( string xml );

      //! Add run stop to data file
      /*! 
       * \param xml Run stop data
      */
      void addRunStop ( string xml );

      //! Add run time to data file
      /*! 
       * \param xml Run time data
      */
      void addRunTime ( string xml );

      //! Enable store of config/status/start/stop to data file & callback
      /*! 
       * \param enable Enable of config/status/start/stop
      */
      void setXmlStore ( bool enable );

      //! Enable shared memory for control
      /*! 
       * \param system System name
       * \param id ID to identify your process
      */
      void enableSharedMemory ( string system, uint32_t id );
      
      //! Function for polling the queue when the RX Thread is disabled
      Data* pollDataQueue(uint32_t wait = 0);

      // Enable buffered writes
      void enableWriteBuffer ( uint32_t size );
};

#endif

