"""
Copyright (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
  * AUTHOR : A. Huck
  * DATE   : Nov 2018

Module Summary
###############

Tools for ranges operations

KEY methods
-------------

* get_range()
  
  * gets range for sample object (start, stop, step)

* arrayFromRange()
  
  * gets range definitions as array

"""

import collections
import math
import odpy.common
import numpy as np

def get_range( samp ):
  """ Creates range object from input

  Parameters:
    * samp (list, array, tuple): array object with three elements [start, stop, step]

  Returns:
    * range: (start, stop, step) object

  Notes:
    * stop position is increased by addition with the step to allow for 
      complete iteration over the range


  """

  return range( samp[0], samp[1]+samp[2], samp[2] )

def arrayFromRange( rg ):
  """ Creates array from range object

  Parameters:
    * rg (obj): range object e.g. range(0, 10, 2)

  Returns:
    * numpy array: of elements defined by range

  """

  if rg == None:
    return None
  return np.linspace( rg.start, rg.stop, len(rg), dtype=np.int32 )

def get_range_steps( samp ):
  """ Gets range steps

  Parameters:
    * samp (list, array, tuple): array object with three elements [start, stop, step]

  Returns:
    * array: stop and start of input
  """

  return arrayFromRange( get_range(samp) )

def getLineObj( obj ):
  """ Gets inline range from object (2D seismic line or 3D cube)

  Parameters:
    * obj (dict): with crossline, inline, Z keys
    
  Returns:
    * array: inline range[start, stop, step]

  Example:

  >>> import odpy.ranges as ranges
  >>> tkzs = {
              'Inline': [200,400,1],
              'Crossline': [500,900,2],
              'Z': [0,2000,4]
            }
            
  >>> ranges.getLineObj(tkzs)
      [200, 400, 1]

  """

  try:
    ret = obj['Inline']
  except (AttributeError, KeyError):
    try:
      ret = obj['lines']
    except (AttributeError, KeyError):
      ret = None
  return ret

def getTraceObj( obj ):
  """ Gets trace range from object (2D seismic trace or 3D cube)

  Parameters:
    * obj (dict): with crossline, inline, Z keys
    
  Returns:
    * array: crossline or trace range[start, stop, step]

  """

  try:
    ret = obj['Crossline']
  except (AttributeError, KeyError):
    try:
      ret = obj['traces']
    except (AttributeError, KeyError):
      ret = None
  return ret

def getZObj( obj ):
  """ Gets depth range from object (2D seismic trace or 3D cube)

  Parameters:
    * obj (dict): with crossline, inline, Z keys
    
  Returns:
    * array: depth or Z range[start, stop, step]

  """

  try:
    ret = obj['Z']
  except (AttributeError, KeyError):
    try:
      ret = obj['zsamp']
    except (AttributeError, KeyError):
      ret = None
  return ret

def getAxesAsRanges( tkzs ):
  """ Gets ranges of seismic dataset axes

  Parameters:
    * tkzs (dict): with crossline, inline, Z keys

  Returns:
    * dict: axes ranges as range object

  Example:

  >>> import odpy.ranges as ranges
  >>>  tkzs = {
               'Inline': [200,400,1],
                'Crossline': [500,900,2],
               'Z': [0,2000,4]
              } 

  >>> ranges.getAxesAsRanges(tkzs)
      OrderedDict([('lines', range(200, 401)),
             ('traces', range(500, 902, 2)),
             ('zsamp', range(0, 2004, 4))])

  """ 

  return collections.OrderedDict ({
    'lines': get_range( getLineObj(tkzs) ),
    'traces': get_range( getTraceObj(tkzs) ),
    'zsamp': get_range( getZObj(tkzs) )
   })

def getAxesAsArrays( tkzs ):
  """ Gets aarray values of seismic dataset axes ranges

  Parameters:
    * tkzs (dict): with crossline, inline, Z keys

  Returns:
    * dict: axes ranges as arrays

  """ 

  return collections.OrderedDict ({
    'lines': get_range_steps( getLineObj(tkzs) ),
    'traces': get_range_steps( getTraceObj(tkzs) ),
    'zsamp': get_range_steps( getZObj(tkzs) )
  })

def getIntervalStr( rg, label ):
  """ Converts and stores range along string as string

  Parameters:
    * rg (list, array, tuple): array object with three elements [start, stop, step]
    * label (str): label to be concatenated with range info

  Returns:
    * str: concatenated label and range info

  """

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
    
    Parameters:
      * value : (float) Input value.
      * round : (boolean, optional). If true returned number will be rounded otherwise returns the ceiling. The default is True.
   
    Returns:
      * float: The nice number
   
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
  
  Parameters:
      * min (float): range minimum
      * max (float): range maximum
      * maxsteps (int, optional): maximum number of major tick steps desired in output range. Default is 10.

  Returns:
    * tuple(niceMin: float, niceMax: float)
       * niceMin: the nicely rounded minimum
       * niceMax: the nicely rounded maximum

  """

  range = abs(max-min)
  if math.isclose(min, max, rel_tol=1e-5):
      range = 0.2 * abs(min)
  range = niceNumber(range, False)
  tickspacing = niceNumber(range/(maxsteps), True)
  niceMin = math.floor(min/tickspacing)*tickspacing
  niceMax = math.ceil(max/tickspacing)*tickspacing
  return (niceMin, niceMax)
