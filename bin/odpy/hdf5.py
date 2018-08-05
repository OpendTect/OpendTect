import odpy.common
import h5py
import numpy

def getInfoDataSet( h5file_or_grp ):
  return h5file_or_grp["++info++"]

def getAttr( dataset, ky ):
  return numpy.bytes_(dataset.attrs[ky]).decode().rstrip().split("\x00")[0]

def getAttrs( dataset, ky ):
  return getAttr( dataset, ky ).split( "`" )
