#
# (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# AUTHOR   : A. Huck
# DATE     : Nov 2018
#
# ranges tools
#

import collections
import math
import odpy.common
import numpy as np

def get_range( samp ):
  return range( samp[0], samp[1]+samp[2], samp[2] )

def arrayFromRange( rg ):
  if rg == None:
    return None
  return np.linspace( rg.start, rg.stop, len(rg), dtype=np.int32 )

def get_range_steps( samp ):
  return arrayFromRange( get_range(samp) )

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
   return collections.OrderedDict ({
    'lines': get_range( getLineObj(tkzs) ),
    'traces': get_range( getTraceObj(tkzs) ),
    'zsamp': get_range( getZObj(tkzs) )
   })

def getAxesAsArrays( tkzs ):
  return collections.OrderedDict ({
    'lines': get_range_steps( getLineObj(tkzs) ),
    'traces': get_range_steps( getTraceObj(tkzs) ),
    'zsamp': get_range_steps( getZObj(tkzs) )
  })

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

def niceNumber(value, round=True):
  """Return a nicely rounded number.
    
    Parameters
    ----------
    value : float
      Input value.
    round : boolean, optional
      If true returned number will be rounded otherwise returns the ceiling. The default is True.
   
    Returns
    -------
    float
      The nice number.
   
    """
  if value > 0:
      signfact = 1
  else:
      signfact = -1
  value = np.abs(value)
  nicefrac = 0
  try:
      exponent = math.floor(math.log10(value))
  except ValueError:
      return value*signfact
  fraction = value/math.pow(10,exponent)
  if round:
    if fraction<1.5:
      nicefrac = 1
    elif fraction<3:
      nicefrac = 2
    elif fraction<7:
      nicefrac = 5
    else:
      nicefrac = 10
  else:
    if fraction<=1:
      nicefrac = 1
    elif fraction<=2:
      nicefrac = 2
    elif fraction<=5:
      nicefrac = 5
    else:
      nicefrac = 10
  return nicefrac * math.pow(10, exponent) * signfact
                    
def niceRange(min, max, maxsteps=10):
  """ Return a nicely rounded range - good for axes.
  
   Parameters
   ----------
   min : float
        Range minimum.
    max : float
        Range maximum.
    maxsteps : int, optional
        Maximum number of major tick steps desired in output range. The default is 10.

    Returns
    -------
    niceMin : float
       The nicely rounded minimum.
    niceMax : float
        The nicely rounded maximum.
    """
  range = abs(max-min)
  if math.isclose(min, max, rel_tol=1e-5):
      range = 0.2 * abs(min)
  range = niceNumber(range, False)
  tickspacing = niceNumber(range/(maxsteps), True)
  niceMin = math.floor(min/tickspacing)*tickspacing
  niceMax = math.ceil(max/tickspacing)*tickspacing
  return (niceMin, niceMax)
