#
# (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# AUTHOR   : Arnaud
# DATE     : November 2018
#
# tools for reading hdf5 files created by OpendTect
#

import collections
import numpy as np
import h5py

def getInfoDataSet( h5file_or_grp ):
  return h5file_or_grp["++info++"]

def getAttr( dataset, ky ):
  return np.bytes_(dataset.attrs[ky]).decode().rstrip().split("\x00")[0]

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
  return int(getAttr(infos,ky))

def getDValue( infos, ky ):
  return float(getAttr(infos,ky))

def getIInterval( infos, ky ):
  ret = getText(infos,ky)
  return [int(ret[0]), int(ret[1])]

def getDInterval( infos, ky ):
  ret = getText(infos,ky)
  return [float(ret[0]), float(ret[1])]

def getIStepInterval( infos, ky ):
  ret = getText(infos,ky)
  return [int(ret[0]), int(ret[1]), int(ret[2])]

def getDStepInterval( infos, ky ):
  ret = getText(infos,ky)
  return [float(ret[0]), float(ret[1]), float(ret[2])]

def getAttribInfo( filenm ):
  h5file = h5py.File( filenm, "r" )
  infods = getInfoDataSet( h5file )

  blocksversion = getIntValue(infods,"Blocks.Version")
  zdomain  = getText(infods,"ZDomain")
  datasetnm = getText(infods,"Name.Cube")
  compnms  = getText(infods,"Components")
  datachar = getText(infods,"Data storage")
  inlstart = getIntValue(infods,"First In-line")
  inlstop  = getIntValue(infods,"Last In-line")
  inl_step = getIntValue(infods,"Step In-line")
  crlstart = getIntValue(infods,"First Cross-line")
  crlstop  = getIntValue(infods,"Last Cross-line")
  crl_step = getIntValue(infods,"Step Cross-line")
  zrg      = getDStepInterval(infods,"Z range")
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
      'Inline': [inlstart,inlstop,inl_step],
      'Crossline': [crlstart,crlstop,crl_step],
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
  h5file = h5py.File( filenm, "r" )
  infods = getInfoDataSet( h5file )
  surveyname = getText(infods,"Name.Survey")
  inlinerg    = getIInterval(infods,"In-line range")
  crosslinerg = getIInterval(infods,"Cross-line range")
  xrg = getDInterval(infods,"X range")
  yrg = getDInterval(infods,"Y range")
  transform = getTransform( infods )
  h5file.close()
  return collections.OrderedDict({
    'name': surveyname,
    'range': collections.OrderedDict({
      'Inline': inlinerg,
      'Crossline': crosslinerg,
      'X': xrg,
      'Y': yrg
    }),
    'transform': transform
  })
