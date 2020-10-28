#
# (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# AUTHOR   : Bert
# DATE     : Jan 2019
#
# Provides a test server for binary data transfer via stdin and stdout
#
# The client has to put:
# - a 4-byte int holding the number of values
# - a block of floats
# Then the script will return:
# - the number of returned vals (always 2)
# - a block of floats (the avg and std of the values passed)
# This stops when the input nrvals < 0
#

import sys
import struct
import numpy
import time
import platform

inpstrm = sys.stdin.buffer
outstrm = sys.stdout.buffer

if platform.system() == 'Windows':
  def dbg_pr( what, val ):
    pass
else:
  dbg_strm = open( "/tmp/dbg.txt", "w" )
  def dbg_pr( what, val ):
    dbg_strm.write( what + ": " + val + "\n" )
    dbg_strm.flush()

dbg_pr( "Started", "script" )

def put_to_output( what ):
  return outstrm.write( what )

def get_from_input( nr ):
  return inpstrm.read( nr )

def mk_int_bytes( ival ):
  return ival.to_bytes( 4, byteorder=sys.byteorder, signed=True )

def get_int_from_bytes( data_read ):
  return int.from_bytes( data_read, byteorder=sys.byteorder, signed=True )


outvals = numpy.zeros( 2, dtype=numpy.float32 )
time.sleep( 2 ) # simulate startup time
dbg_pr( "Started", "loop" )

while True:

  dbg_pr( "Try get", "4 bytes for nrvals" )
  try:
    data_read = get_from_input( 4 )
  except:
    dbg_pr( "Failed", "getting 4 bytes nrvals" )
    break

  nrvals = get_int_from_bytes( data_read )
  dbg_pr( "Result nrvals", str(nrvals) )
  if nrvals == 0:
    time.sleep( 0.01 )
    dbg_pr( "Got 0 vals, waited", "0.01 seconds" )
    continue
  if nrvals < 0:
    dbg_pr( "Stopped", "loop" )
    break

  dbg_pr( "Try get", str(nrvals) + " values" )
  try:
    inpdata = get_from_input( 4*nrvals )
  except:
    dbg_pr( "Exit", "except" )
    exit( 1 )
  dbg_pr( "Got", str(nrvals) + " values" )

  vals = struct.unpack( 'f'*nrvals, inpdata )
  dbg_pr( "Got values", str(vals) )
  time.sleep( 2 ) # simulate calculation time
  outvals[0] = numpy.mean(vals)
  outvals[1] = numpy.std(vals)
  dbg_pr( "Calculated values", str(outvals) )

  nrbyteswritten = put_to_output( mk_int_bytes(len(outvals)) )
  dbg_pr( "Put", "number of bytes, nrbyteswritten=" + str(nrbyteswritten) )
  nrbyteswritten = put_to_output( outvals.tobytes() )
  dbg_pr( "Put", "outvals, nrbyteswritten=" + str(nrbyteswritten) )

dbg_pr( "Exit", "normal" )
exit( 0 )
