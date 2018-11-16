import numpy as np
import h5py
import odpy.common

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
  if zdomain == "TWT":
    zrg = np.multiply(zrg,1000).astype("int32")
    zrg = [zrg[0],zrg[1],zrg[2]]
  h5file.close()
  return {
    'name': datasetnm,
    'attributes': compnms,
    'version': blocksversion,
    'zdomain': zdomain,
    'storage': datachar,
    'range': {
      'Inline': [inlstart,inlstop,inl_step],
      'Crossline': [crlstart,crlstop,crl_step],
      'Z': zrg
    },
    'block': {
      'size': blocksdim,
      'range': {
        'Inline': blocksinlrg,
        'Crossline': blockscrlrg
      }
    }
  }

# Survey transformation (X-Y) to (Inl-Crl)
# Given an Inl-Crl pair: ic_pos = [inl,crl]
# the following returns the corresponding X,Y location:
# xy_pos = transform['origin'] + np.dot( transform['rotation'], ic_pos )
def getTransform( info ):
  coordsxbid = getDStepInterval(info,"Coord-X-BinID")
  coordsybid = getDStepInterval(info,"Coord-Y-BinID")
  return {
    'origin':   [coordsxbid[0],coordsybid[0]],
    'rotation': [[coordsxbid[1],coordsxbid[2]],[coordsybid[1],coordsybid[2]]]
  }

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
  return {
    'name': surveyname,
    'range': {
      'Inline': inlinerg,
      'Crossline': crosslinerg,
      'X': xrg,
      'Y': yrg
    },
    'transform': transform
  }

def getWellInfo( filenm ):
  h5file = h5py.File( filenm, "r" )
  infods = getInfoDataSet( h5file )
  try:
    type = getText(infods,"Type")
  except KeyError:
    print("No type found. Probably wrong type of hdf5 file")
    return {}

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
