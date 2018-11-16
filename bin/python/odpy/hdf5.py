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
