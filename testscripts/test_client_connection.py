#!/usr/bin/env python3
#
# (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# AUTHOR   : A. Huck
# DATE     : Jan 2023
#
# Echo client program
#

import argparse
import socket
import struct

parser = argparse.ArgumentParser(
          description='Client application for connection to TCP server')
parser.add_argument( '-v', '--version',
            action='version',version='%(prog)s 1.0')
netgrp = parser.add_argument_group( 'Network' )
netgrp.add_argument( '--address',
            dest='addr', metavar='ADDRESS', action='store',
            type=str, default='localhost',
            help='Address to connect to' )
netgrp.add_argument( '--port',
            dest='port', action='store',
            type=int, default=37504,
            help='Port to connect to')
parser.add_argument( '--msg', dest='msg', action='store',
                    type=str, default='Hello, World',
                    help='Message to send')

args = vars(parser.parse_args())

HOST = args['addr']    # The remote host
PORT = args['port']    # The same port as used by the server
msg = args['msg']
encoding = 'utf-8'


print( 'Will try to connect to host', HOST, 'using port:', PORT)
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    print( '[OK] Connection established' )
    msg = msg.encode( encoding )
    sz = len(msg)
    od_hdr = struct.pack('=i',4+sz) +  struct.pack('=i',1) + struct.pack('=h',-1)
    hdrlen = len(od_hdr)
    s.sendall( od_hdr + struct.pack('=i',sz) + msg )
    data = s.recv(1024)

print( f"[OK] Received back '{data[hdrlen+4:].decode( encoding )}'" )
