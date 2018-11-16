import numpy
import h5py
import odpy.common

def getInfoDataSet( h5file_or_grp ):
  return h5file_or_grp["++info++"]

def getAttr( dataset, ky ):
  return numpy.bytes_(dataset.attrs[ky]).decode().rstrip().split("\x00")[0]

def getAttrs( dataset, ky ):
  return getAttr( dataset, ky ).split( "`" )

def getText( infos, ky ):
  ret = getAttr( infos, ky )
  if "`" in ret:
    return ret.split( "`" )
  return ret

def getBoolValue( infos, ky ):
  return getAttr( infos, ky ) == "Yes"

def getIntValue( infos, ky ):
  return int(getAttr( infos, ky ))

def getDValue( infos, ky ):
  return float(getAttr( infos, ky ))

def getWellInfo( filenm ):
  h5file = h5py.File( filenm, "r" )
  infods = getInfoDataSet( h5file )
  type = getText(infods,"Type")

  ex_sz = getIntValue(infods,"Examples.Size") 
  idx = 0
  examples = list()
  while idx < ex_sz:
    example_sz = getIntValue(infods,"Examples."+str(idx)+".Size")
    example_id = list()
    idy = 0
    while idy < example_sz:
      example_id.append(getText(infods,"Examples."+str(idx)+".ID."+str(idy)))
      idy += 1
    example = {
      "name": getText(infods,"Examples."+str(idx)+".Log"),
      "id": example_id,
      "survey": getText(infods,"Examples."+str(idx)+".Survey")
    }
    examples.append( example )
    idx += 1

  inp_sz = getIntValue(infods,"Input.Size")
  idx = 0
  input = list()
  while idx < inp_sz:
    inp = {
      "name": getText(infods,"Input."+str(idx)+".Logs"),
      "survey": getText(infods,"Input."+str(idx)+".Survey")
    }
    input.append( inp )
    idx += 1

  zstep = getDValue(infods,"Z step") 
  stepout = getIntValue(infods,"Stepout") 
  marker = (getText(infods,"Top marker"), getText(infods,"Bottom marker"))
  isinterpol = getBoolValue(infods,"Edge extrapolation")
  h5file.close()
  return {
    'examples': examples,
    'input': input,
    'zstep': zstep,
    'stepout': stepout,
    'marker': marker,
    'interpolated': isinterpol
  }
