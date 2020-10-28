#
# (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# AUTHOR   : A. Huck
# DATE     : Nov 2018
#
# Pre-loads (part of) an HDF5 seismic dataset created by OpendTect, with sampling as:
# sampling = {
#  'Inline': range(start,stop,step)
#  'Crossline': range(start,stop,step)
#  'Z': range(start,stop,step)
# }
# Mind that in python stop >= start+step, otherwise the range is empty!
# The entire sampling can be retrieve with odhdf5.getAttribInfo( filenm )['range']
#

import numpy as np
import h5py
import odpy.hdf5 as odhdf5
import odpy.ranges as ranges

def getAxes( sampling, dataarr ):
  flatx1 = len(dataarr) < 2
  flatx2 = len(dataarr[0]) < 2
  flatx3 = len(dataarr[0][0]) < 2
  lines = ranges.getLineObj( sampling )
  traces = ranges.getTraceObj( sampling )
  zsamp = ranges.getZObj( sampling )
  x3axis = None
  if flatx1:
    x1axis = traces
    x2axis = zsamp
  elif flatx2:
    x1axis = lines
    x2axis = zsamp
  elif flatx3:
    x1axis = lines 
    x2axis = traces
  else:
    x1axis = lines 
    x2axis = traces
    x3axis = zsamp

  x1label = list(sampling.keys())[list(sampling.values()).index(x1axis)]
  x2label = list(sampling.keys())[list(sampling.values()).index(x2axis)]
  x3label = None
  if x3axis != None:
    x3label = list(sampling.keys())[list(sampling.values()).index(x3axis)]

  return {
    'range': [ranges.arrayFromRange(x1axis),
              ranges.arrayFromRange(x2axis),
              ranges.arrayFromRange(x3axis)],
    'labels': [x1label,x2label,x3label]
  }

def preLoad( filenm, attribute=None, sampling=None, fillval=None ):
  attribinfo = odhdf5.getAttribInfo( filenm )
  samplinginfo = attribinfo['range']
  datatkzs = ranges.getAxesAsRanges( samplinginfo )
  if attribute==None:
    attribinfo = odhdf5.getAttribInfo( filenm )
    compnms = attribinfo['attributes']
    if isinstance( compnms, list ):
      if len(compnms)<1:
        return []
      attribute = compnms[0]
    else:
      attribute = compnms

  if sampling==None:
    sampling = datatkzs
  if fillval==None:
    fillval = np.nan

  blocksinfo = attribinfo['block']
  blocksz = blocksinfo['size']
  dim1blocksz = blocksz[0]
  dim2blocksz = blocksz[1]
  blocksrg = blocksinfo['range']
  nrblockx = blocksrg['Inline'][1]+1
  nrblocky = blocksrg['Crossline'][1]+1

  h5file = odhdf5.openFile( filenm, 'r' )
  datagroup = h5file[attribute]
  if len(datagroup) < 1:
    print( "Empty dataset found" )
    return []

  alllines = ranges.getLineObj( datatkzs )
  alltraces = ranges.getTraceObj( datatkzs )
  allzsamp = ranges.getZObj( datatkzs )
  lines = ranges.getLineObj( sampling )
  traces = ranges.getTraceObj( sampling )
  zsamp = ranges.getZObj( sampling )
  dim3blocksz = len(allzsamp)
  nrz = len(zsamp)
  blkzrg = [ allzsamp[0], allzsamp[-1] ]
  zrg = [ max(blkzrg[0],zsamp.start), min(blkzrg[1],zsamp.stop-zsamp.step) ]
  zrgin = [ allzsamp.index(zrg[0]) % dim3blocksz, allzsamp.index(zrg[1]) % dim3blocksz+1 ]
  zrgout = [ zsamp.index(zrg[0]), zsamp.index(zrg[1])+1 ]

  datagroupset = list( datagroup.items() )
  datasetsamptype = datagroupset[0][1].dtype
  sliceout = np.full((len(lines),len(traces),nrz),fillval,dtype=datasetsamptype)
  for blkidx in range(nrblockx):
    for blkidy in range(nrblocky):
      subcubenm = str(blkidx)+"."+str(blkidy)
      subcube = datagroup.get( subcubenm )
      if subcube == None:
        continue
      dim1shift = 0
      dim2shift = 0
      if len(subcube.attrs['Loc00']) > 0:
        locorig = odhdf5.getIInterval( subcube, "Loc00" )
        dim1shift = locorig[0]
        dim2shift = locorig[1]
      firstx1 = blkidx * dim1blocksz + dim1shift
      lastx1 = firstx1 + subcube.shape[0] - 1
      blklinerg = [ alllines[firstx1], alllines[lastx1] ]
      if lines.stop-lines.step < blklinerg[0] or lines.start > blklinerg[1]:
        continue
      firstx2 = blkidy * dim2blocksz + dim2shift
      lastx2 = firstx2 + subcube.shape[1] - 1
      blktrcrg = [ alltraces[firstx2], alltraces[lastx2] ]
      if traces.stop-traces.step < blktrcrg[0] or traces.start > blktrcrg[1]:
        continue
      linerg = [ max(blklinerg[0],lines.start), min(blklinerg[1],lines.stop-lines.step) ]
      trcrg  = [ max(blktrcrg[0],traces.start), min(blktrcrg[1],traces.stop-traces.step) ]
      linergin = [ alllines.index(linerg[0]) % dim1blocksz-dim1shift,
                   alllines.index(linerg[1]) % dim1blocksz+1-dim1shift ]
      trcrgin = [ alltraces.index(trcrg[0]) % dim2blocksz-dim2shift,
                  alltraces.index(trcrg[1]) % dim2blocksz+1-dim2shift ]
      linergout = [ lines.index(linerg[0]), lines.index(linerg[1])+1 ]
      trcrgout = [traces.index(trcrg[0]), traces.index(trcrg[1])+1 ]
      sliceout[linergout[0]:linergout[1],trcrgout[0]:trcrgout[1],zrgout[0]:zrgout[1]] = subcube[linergin[0]:linergin[1],trcrgin[0]:trcrgin[1],zrgin[0]:zrgin[1]]
  h5file.close()

  axes = getAxes( sampling, sliceout )
  sliceout = np.squeeze(sliceout)

  return {
    'data': sliceout,
    'axes': axes
  }
