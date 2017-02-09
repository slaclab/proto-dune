#!/usr/bin/env python
##############################################################################
## This file is part of 'SLAC Generic DAQ Software'.
## It is subject to the license terms in the LICENSE.txt file found in the 
## top-level directory of this distribution and at: 
##    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
## No part of 'SLAC Generic DAQ Software', including this file, 
## may be copied, modified, propagated, or distributed except according to 
## the terms contained in the LICENSE.txt file.
##############################################################################

import MySQLdb
import datetime
import time
import threading

__version__ = "1.0"

class DaqMysqlException(Exception):
    pass

class DaqMysql(threading.Thread):
    """Class to handle mysql updates and queries."""

    def __init__(self, dbhost, dbname, dbuser, dbpass, client=False):
        """ Create mysql class.  """
        threading.Thread.__init__(self)

        self._host    = dbhost
        self._port    = 3306
        self._user    = dbuser
        self._pass    = dbpass
        self._dbase   = dbname
        self._tspoll  = "server_ts"  if client else "client_ts"
        self._tsset   = "client_ts"  if client else "server_ts"
        self._serpoll = "server_ser" if client else "client_ser"
        self._serset  = "client_ser" if client else "server_ser"
        self._db      = None
        self._dbLock  = threading.Lock()
        self._cbLock  = threading.Lock()
        self._cbacks  = {}
        self._ctime   = {}
        self._cser    = {}
        self._pollEn  = False;
        self._pollPer = 0

    def __del__(self):
        self.pollDisable()

    def connect (self ) :
        tries = 0

        while tries < 5:
            try:
                if self._db == None:
                    self._db = MySQLdb.connect(host=self._host,user=self._user,passwd=self._pass,db=self._dbase,port=self._port)

                cursor = self._db.cursor(MySQLdb.cursors.DictCursor)
                cursor.execute("SELECT VERSION()")
                self._db.autocommit(True)

                return self._db

            except MySQLdb.Error:
                self._db = None
                time.sleep(.1)
            tries = tries + 1

        raise DaqMysqlException("Failed to connect to Mysql server " + self._host)

    def disconnect (self):
        if self._db != None:
            self._db.close()
            self._db = None

    def pollEnable(self,period):
        self._pollEn  = True
        self._pollPer = period
        self.start()

    def pollDisable(self):
        if self._pollEn :
            self._pollEn = False
            self.join()

    # Register a callback
    def addPollCallback (self, function, table, client):
        self._cbLock.acquire()

        if not self._cbacks.has_key(table):
            self._cbacks[table] = []
            self._ctime[table]  = None
            self._cser[table]   = {}

        self._cbacks[table].append(function)

        self._cbLock.release()

    # Register a command callback
    def addCommandCallback (self, function):
        self.addPollCallback (function, "command",False)

    # Register a configuration callback
    def addConfigurationCallback (self, function, client=False):
        self.addPollCallback (function, "configuration", client)

    # Register a status callback
    def addStatusCallback (self, function):
        self.addPollCallback (function, "status",True)

    # Add a configuration entry
    def addConfigurationEntry (self, varData):
        self._dbLock.acquire()

        self.connect()

        cursor = self._db.cursor(MySQLdb.cursors.DictCursor)

        sql =  "replace into configuration "
        sql += "(configuration.id,configuration.create_ts,configuration.value,configuration.server_ts,"
        sql += "configuration.client_ser,configuration.server_ser,"
        sql += "configuration.client_ts,configuration.type,configuration.enum,configuration.compA,"
        sql += "configuration.compB,configuration.compC,configuration.compUnits,configuration.min,"
        sql += "configuration.max,configuration.perInstance,configuration.hidden) "
        sql += "values ('%s',now(),'',now()," % (varData['name'])
        sql += "'0','0',now(),"
        sql += "'%s','%s','%s','%s'," % (varData['type'],varData['enum'],varData['compA'],varData['compB'])
        sql += "'%s','%s','%s'," % (varData['compC'],varData['compUnits'],varData['min'])
        sql += "'%s','%s','%s')" % (varData['max'],varData['perInstance'],varData['hidden'])

        cursor.execute(sql)
        self._dbLock.release()

    # Add a status entry
    def addStatusEntry (self, varData):
        self._dbLock.acquire()

        self.connect()
        cursor = self._db.cursor(MySQLdb.cursors.DictCursor)

        sql =  "replace into status "
        sql += "(status.id,status.create_ts,status.value,status.server_ts,"
        sql += "status.server_ser,"
        sql += "status.type,status.enum,status.compA,"
        sql += "status.compB,status.compC,status.compUnits,status.min,"
        sql += "status.max,status.perInstance,status.hidden) "

        sql += "values ('%s',now(),'',now()," % (varData['name'])
        sql += "'0',"
        sql += "'%s','%s','%s','%s'," % (varData['type'],varData['enum'],varData['compA'],varData['compB'])
        sql += "'%s','%s','%s'," % (varData['compC'],varData['compUnits'],varData['min'])
        sql += "'%s','%s','%s')" % (varData['max'],varData['perInstance'],varData['hidden'])

        cursor.execute(sql)
        self._dbLock.release()

    # Add an error
    def addError (self, message):
        self._dbLock.acquire()

        self.connect()

        cursor = self._db.cursor(MySQLdb.cursors.DictCursor)
        sql =  "insert into errors (errors.time,errors.message) values (now(),'%s')" % (message)
        cursor.execute(sql)

        self._dbLock.release()

    # Add a command entry
    def addCommandEntry (self, cmdData):
        self._dbLock.acquire()

        self.connect()

        cursor = self._db.cursor(MySQLdb.cursors.DictCursor)
        sql =  "replace into command "
        sql += "(command.id,command.create_ts,command.client_ts,"
        sql += "command.arg,command.hasArg,command.hidden,command.client_ser) "

        sql += "values ('%s',now(),now()," % (cmdData['name'])
        sql += "'','%s','%s','0')" % (cmdData['hasArg'],cmdData['hidden'])

        cursor.execute(sql)

        self._dbLock.release()

    # Clear entries
    def clearEntries (self):
        self._dbLock.acquire()

        self.connect()

        cursor = self._db.cursor(MySQLdb.cursors.DictCursor)

        cursor.execute("update command       set client_ser='0'")
        cursor.execute("update configuration set client_ser='0', server_ser='0'")
        cursor.execute("update status        set server_ser='0'")

        self._dbLock.release()

    # Delete old entries
    def delOldEntries (self):
        self._dbLock.acquire()

        self.connect()

        cursor = self._db.cursor(MySQLdb.cursors.DictCursor)

        cursor.execute("delete from command       where create_ts < (now() - interval 1 minute)")
        cursor.execute("delete from configuration where create_ts < (now() - interval 1 minute)")
        cursor.execute("delete from status        where create_ts < (now() - interval 1 minute)")

        self._dbLock.release()

    # Update configuration entry
    def updateConfiguration (self, name, value):
        self._dbLock.acquire()

        self.connect()

        cursor = self._db.cursor(MySQLdb.cursors.DictCursor)

        sql = "update configuration set value='%s', %s=now()," % (value,self._tsset) 
        sql += "%s = (%s + 1) where id='%s'" % (self._serset,self._serset,name)

        cursor.execute(sql)
        self._dbLock.release()

    # Update status entry from server
    def updateStatus (self, name, value):
        self._dbLock.acquire()

        self.connect()

        cursor = self._db.cursor(MySQLdb.cursors.DictCursor)

        sql = "update status set value='%s', server_ts=now(), server_ser=(server_ser+1) where id='%s'" % (MySQLdb.escape_string(value),name)

        cursor.execute(sql)
        self._dbLock.release()

    # Update command entry from client
    def updateCommand (self, name, arg=None):
        self._dbLock.acquire()

        self.connect()

        cursor = self._db.cursor(MySQLdb.cursors.DictCursor)

        larg = arg if arg else ''

        sql = "update command set arg='%s', client_ts=now(),client_ser=(client_ser+1) where id='%s'" % (arg,name)
                
        cursor.execute(sql)
        self._dbLock.release()

    # Poll a table for updates
    def pollTable(self, table, tstamp, sort=None):

        self._dbLock.acquire()
        self.connect()
        cursor = self._db.cursor(MySQLdb.cursors.DictCursor)

        # Setup query
        query = "select *, now() as tstamp, %s as ser, (now()-%s) as age, UNIX_TIMESTAMP(%s) as utime from %s" % (self._serpoll,self._tspoll,self._tspoll,table)

        # Filter with optional passed time 
        if tstamp:
            query += " where %s > ('%s' - interval 1 second)" % (self._tspoll,str(tstamp))

        # Apply Sort
        if sort:
            query += " order by " + sort

        # Execute query
        rowCount = cursor.execute(query)
        rows = cursor.fetchall()
        self._dbLock.release()

        return rowCount, rows

    # Polling thread
    def run(self):

        # Loop while thread is enabled
        while self._pollEn == True:

            # Pause for a period
            time.sleep(self._pollPer)

            # lock callback record
            self._cbLock.acquire()
           
            # Process each table
            for table in self._cbacks:
                rowCount,rows = self.pollTable(table,self._ctime[table])

                # We have entries
                if rowCount > 0:
                    for row in rows:

                        if self._cser[table].has_key(row['id']):
                            cmpSer = self._cser[table][row['id']]
                        else:
                            cmpSer = 0

                        # Check against each callback
                        if row['ser'] > cmpSer:
                            for cb in self._cbacks[table]:
                                try:
                                    cb(row)
                                except Exception,msg:
                                    print("Callback exception: " + str(msg))
                                except :
                                    print("Unknown callback exception!")

                        self._ctime[table] = row['tstamp']
                        self._cser[table][row['id']] = row['ser']

            # release callback record
            self._cbLock.release()

