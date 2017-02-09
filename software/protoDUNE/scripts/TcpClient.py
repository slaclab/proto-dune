##############################################################################
## This file is part of 'Example Project Software'.
## It is subject to the license terms in the LICENSE.txt file found in the 
## top-level directory of this distribution and at: 
##    https://confluence.slac.stanford.edu/display/ppareg/LICENSE.html. 
## No part of 'Example Project Software', including this file, 
## may be copied, modified, propagated, or distributed except according to 
## the terms contained in the LICENSE.txt file.
##############################################################################

import socket
import select
import sys
from time import sleep
from random import randint

S_IP = "192.168.2.1"
S_PORT = 8099

C_IP = "192.168.2.117"
C_PORT = randint(49152,65535)

try:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM) # Create a socket object
    s.bind((C_IP, C_PORT))                      # Bind to the port
    s.connect((S_IP, S_PORT))                                            # Start connection
except socket.error as msg:
    print 'Bind failed. Error Code : ' + str(msg[0]) + ' Message ' + msg[1]
    sys.exit()

txData = ""
for i in range(1,512*1024):
    txData+=chr(i%256)

while True:    
    s.send(txData)    
# print s.recv(1024)
s.close                     # Close the socket when done
