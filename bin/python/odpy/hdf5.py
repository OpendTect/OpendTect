#
# (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# AUTHOR   : A. Huck
# DATE     : Nov 2018
#
# tools for reading hdf5 files created by OpendTect
#

import collections
import numpy as np
import h5py

dGBInfoDSName = '++info++'

def openFile( filenm, mode ):
  return h5py.File( filenm, mode, libver=('v110', 'latest') )

def getInfoDataSet( h5file_or_grp ):
  try:
    return h5file_or_grp[dGBInfoDSName]
  except KeyError:
    print( "Input Error: HDF5 file probably not created by OpendTect (no ++info++ attribute)" )
    raise

def ensureHasDataset( h5file_or_grp ):
  if dGBInfoDSName in h5file_or_grp:
    return getInfoDataSet( h5file_or_grp )
  return h5file_or_grp.create_dataset(dGBInfoDSName,dtype='S1')

def getAttr( dataset, ky ):
  try:
    attrib = dataset.attrs[ky]
    return np.bytes_(dataset.attrs[ky]).decode().rstrip().split("\x00")[0]
  except KeyError:
    print( "HDF5 key not found: '"+ky+"'. Available keys:\n")
    print( list(dataset.attrs.keys()) )
    raise

def setAttr( dataset, ky, str ):
  dataset.attrs[ky] = np.chararray.encode( str )

def hasAttr( dataset, ky ):
  return ky in dataset.attrs

def getAttrs( dataset, ky ):
  return getAttr( dataset, ky ).split( "`" )

def getText( infos, ky ):
  ret = getAttr( infos, ky )
  if "`" in ret:
    return ret.split( "`" )
  return ret

def setArray( infos, ky, arr ):
  try:
    arrstr = '`'.join( map(str,arr) )
  except TypeError:
    arrstr = str(arr)
  setAttr( infos, ky, arrstr )

def getBoolValue( infos, ky ):
  return getAttr( infos, ky ) == "Yes"

def getIArray_( valsstr ):
  if isinstance(valsstr,list) and len(valsstr) > 1:
    ret = list()
    for valstr in valsstr:
      ret.append( int(valstr) )
    return ret
  try:
    return int(valsstr)
  except ValueError:
    return int(float(valsstr))

def getIntValue( infos, ky ):
  return getIArray_( getText(infos,ky) )

def getIInterval( infos, ky ):
  return getIArray_( getText(infos,ky) )

def getIStepInterval( infos, ky ):
  return getIArray_( getText(infos,ky) )

def getIArray( infos, ky ):
  return getIArray_( getText(infos,ky) )

def getDArray_( valsstr ):
  if isinstance(valsstr,list) and len(valsstr) > 1:
    ret = list()
    for valstr in valsstr:
      ret.append( float(valstr) )
    return ret
  return float(valsstr)
  
def getDValue( infos, ky ):
  return getDArray_( getText(infos,ky) )

def getDInterval( infos, ky ):
  return getDArray_( getText(infos,ky) )

def getDStepInterval( infos, ky ):
  return getDArray_( getText(infos,ky) )

def getDArray( infos, ky ):
  return getDArray_( getText(infos,ky) )

def getAttribInfo( filenm ):
  h5file = openFile( filenm, 'r' )
  infods = getInfoDataSet( h5file )

  blocksversion = getIntValue(infods,"Blocks.Version")
  zdomain  = getText(infods,"ZDomain")
  datasetnm = getText(infods,"Name.Cube")
  compnms  = getText(infods,"Components")
  datachar = getText(infods,"Data storage")
  inlinerg    = getIInterval(infods,"In-line range")
  crosslinerg = getIInterval(infods,"Cross-line range")
  zrg      = getDStepInterval(infods,"Z range")
  inl_step = getIntValue(infods,"Step In-line")
  crl_step = getIntValue(infods,"Step Cross-line")
  blocksdim = getIInterval(infods,"Blocks.Max Dimensions")
  blocksinlrg = getIInterval(infods,"Blocks.Inl ID Range")
  blockscrlrg = getIInterval(infods,"Blocks.Crl ID Range")
  h5file.close()
  if zdomain == "TWT":
    zrg = np.multiply(zrg,1000).astype("int32")
    zrg = [zrg[0],zrg[1],zrg[2]]
  return collections.OrderedDict({
    'name': datasetnm,
    'attributes': compnms,
    'version': blocksversion,
    'zdomain': zdomain,
    'storage': datachar,
    'range': collections.OrderedDict({
      'Inline': [inlinerg[0],inlinerg[1],inl_step],
      'Crossline': [crosslinerg[0],crosslinerg[1],crl_step],
      'Z': zrg
    }),
    'block': {
      'size': blocksdim,
      'range': collections.OrderedDict({
        'Inline': blocksinlrg,
        'Crossline': blockscrlrg
      })
    }
  })

# Survey transformation (X-Y) to (Inl-Crl)
# Given an Inl-Crl pair: ic_pos = [inl,crl]
# the following returns the corresponding X,Y location:
# xy_pos = transform['origin'] + np.dot( transform['rotation'], ic_pos )
def getTransform( info ):
  coordsxbid = getDStepInterval(info,"Coord-X-BinID")
  coordsybid = getDStepInterval(info,"Coord-Y-BinID")
  return collections.OrderedDict({
    'origin':   [coordsxbid[0],coordsybid[0]],
    'rotation': [[coordsxbid[1],coordsxbid[2]],[coordsybid[1],coordsybid[2]]]
  })

def getSurveyInfo( filenm ):
  h5file = openFile( filenm, 'r' )
  infods = getInfoDataSet( h5file )
  surveyname = getText(infods,"Name.Survey")
  inlstart = getIntValue(infods,"First In-line")
  inlstop  = getIntValue(infods,"Last In-line")
  crlstart = getIntValue(infods,"First Cross-line")
  crlstop  = getIntValue(infods,"Last Cross-line")
  xrg = getDInterval(infods,"X range")
  yrg = getDInterval(infods,"Y range")
  transform = getTransform( infods )
  h5file.close()
  return collections.OrderedDict({
    'name': surveyname,
    'range': collections.OrderedDict({
      'Inline': [inlstart,inlstop],
      'Crossline': [crlstart,crlstop],
      'X': xrg,
      'Y': yrg
    }),
    'transform': transform
  })
