#!/usr/bin/env python3
#
# (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# AUTHOR   : A. Huck
# DATE     : Jan 2023
#
# Echo server program
#

import argparse
import socket

parser = argparse.ArgumentParser(
          description='Server application with listening socket')
parser.add_argument( '-v', '--version',
            action='version',version='%(prog)s 1.0')
netgrp = parser.add_argument_group( 'Network' )
netgrp.add_argument( '--port',
                    dest='port', action='store',
                    type=int, default=37504,
                    help='Port to listen on')
netgrp.add_argument( '--tcp6', dest='onlytcp6', action='store_true',
                    default=False,
                    help='listen only on IPv6 interfaces' )
netgrp.add_argument( '--tcp4', dest='onlytcp4', action='store_true',
                    default=False,
                    help='listen only on IPv4 interfaces' )
netgrp.add_argument( '--local', dest='localserv', action='store_true',
                     default=False,
                     help="listen on localhost only" )

args = vars(parser.parse_args())

onlytcp6 = args['onlytcp6']
onlytcp4 = args['onlytcp4']
uselocal = args['localserv']
if uselocal and not onlytcp6 and not onlytcp4:
    onlytcp6 = True
   
HOST = ''            # Symbolic name meaning all available interfaces
family = socket.AF_INET6
if uselocal:
    HOST = 'localhost'
    if onlytcp4:
        family = socket.AF_INET
    print( 'Listening on localhost only' )
elif onlytcp6:
    HOST = '::'
elif onlytcp4:
    HOST = '0.0.0.0'
    family = socket.AF_INET
    
PORT = args['port']  # Arbitrary non-privileged port
if onlytcp6:
    print( 'Listening on only IPv6 interfaces' )
elif onlytcp4:
    print( 'Listening on only IPv4 interfaces' )
else:
    print( 'Listening on both IPv4 and IPv6 interfaces' )
    
print( 'Listening server using port:', PORT )
    
with socket.socket(family=family, type=socket.SOCK_STREAM) as s:
    if onlytcp6 and not uselocal:
      s.setsockopt(socket.IPPROTO_IPV6,socket.IPV6_V6ONLY,1)
    s.bind((HOST, PORT))
    s.listen(50)
    while True:
        conn, addr = s.accept()
        with conn:
            print('Connected by', addr)
            while True:
                data = conn.recv(1024)
                if not data: break
                conn.sendall(data)