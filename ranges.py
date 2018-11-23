#
# (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# AUTHOR   : Arnaud
# DATE     : November 2018
#
# ranges tools
#

import odpy.common
import numpy as np

def get_range( samp ):
  return range( samp[0], samp[1]+samp[2], samp[2] )

def get_range_steps( samp ):
  rg = get_range( samp )
  return np.linspace( rg.start, rg.stop, len(rg), dtype=np.int32 )

def getLineObj( obj ):
  try:
    ret = obj['Inline']
  except (AttributeError, KeyError):
    try:
      ret = obj['lines']
    except (AttributeError, KeyError):
      ret = None
  return ret

def getTraceObj( obj ):
  try:
    ret = obj['Crossline']
  except (AttributeError, KeyError):
    try:
      ret = obj['traces']
    except (AttributeError, KeyError):
      ret = None
  return ret

def getZObj( obj ):
  try:
    ret = obj['Z']
  except (AttributeError, KeyError):
    try:
      ret = obj['zsamp']
    except (AttributeError, KeyError):
      ret = None
  return ret

def getAxesAsRanges( tkzs ):
   return {
    'lines': get_range( getLineObj(tkzs) ),
    'traces': get_range( getTraceObj(tkzs) ),
    'zsamp': get_range( getZObj(tkzs) )
   }

def getAxesAsArrays( tkzs ):
  return {
    'lines': get_range_steps( getLineObj(tkzs) ),
    'traces': get_range_steps( getTraceObj(tkzs) ),
    'zsamp': get_range_steps( getZObj(tkzs) )
  }

def getIntervalStr( rg, label ):
  if len(rg) < 3:
    return
  str = label + " range: "+ repr(rg[0]) + "-" + repr(rg[1])
  str = str + " (step " + repr(rg[2]) + ")"
  return str

def printSampling( tkzs ):
  print( getIntervalStr(getLineObj(tkzs),"Inline") )
  print( getIntervalStr(getTraceObj(tkzs),"Crossline") )
  print( getIntervalStr(getZObj(tkzs),"Z") )
