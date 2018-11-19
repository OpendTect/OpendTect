import sys
import os
import numpy
import h5py
import odpy.hdf5 as odhdf5
import odpy.ranges as ranges
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1.inset_locator import inset_axes

if len(sys.argv) < 2:
  print( "No file name specified" )

filenm = sys.argv[1]
print( "\nBrowsing '" + filenm + "'\n\n" )

si = odhdf5.getSurveyInfo( filenm )
attribinfo = odhdf5.getAttribInfo( filenm )
samplinginfo = attribinfo['range']
tkzs = ranges.getAxesAsRanges( samplinginfo )
axes = ranges.getAxesAsArrays( samplinginfo )
blocksinfo = attribinfo['block']
blocksrg = blocksinfo['range']

print( "Data is stored as " + attribinfo['storage'] + "\n" )
# TODO: add gap information: The cube has varying cross-line ranges. / The cube is 100% rectangular.

usrinp = "1"
compnms = attribinfo['attributes']
if isinstance( compnms, list ):
  nrcomps = len( compnms )
  if nrcomps > 1:
    for idx, compnm in enumerate(compnms):
      print( "Component " + str(idx+1) + ": '" + compnm + "'" )
      idx = idx+1
    usrinp = input( "Enter component index: " )
  cidx = int( usrinp ) - 1
  if cidx < 0:
    cidx = 0
  if cidx >= nrcomps:
    cidx = nrcomps - 1
  attribnm = compnms[cidx]
else:
  attribnm = compnms

ranges.printSampling( samplinginfo )
print( "Number of Z samples: " + repr(len(axes['zsamp'])) )
print( "" )

h5file = h5py.File( filenm, "r" )
cont = "Y"
while (cont == "Y") or (cont == "y" ) or (cont == "YES") or (cont == "Yes"):
  slicesel = -1
  #TODO: Add volume sub-sel (3D pre-loading)
  while (slicesel != 1) and (slicesel != 2) and (slicesel != 3):
    slicesel = input("Slice type: [1-In-line] [2-Cross-line] [3-Z slice]  ")
    try:
      slicesel = int(slicesel)
    except ValueError:
      cont="No"
      break
    inlinedir = slicesel == 1
    crosslinedir = slicesel == 2
    zdir = slicesel == 3
    if inlinedir:
      firstidx = 0
      secondidx = 1
      slicetxt = "Inline"
      slicerg = ranges.getLineObj( tkzs )
      slicevals = ranges.getLineObj( axes )
      x1axis = ranges.getTraceObj( axes )
      x2axis = ranges.getZObj( axes )
      xlabel = "Cross-line"
      ylabel = attribinfo['zdomain']
      dim1blocksz = blocksinfo['size'][0]
      dim2blocksz = blocksinfo['size'][1]
      dim2rg = range( ranges.getTraceObj(blocksrg)[0], ranges.getTraceObj(blocksrg)[1]+1 )
    elif crosslinedir:
      firstidx = 1
      secondidx = 0
      slicetxt = "Crossline"
      slicerg = ranges.getTraceObj( tkzs )
      slicevals = ranges.getTraceObj( axes )
      x1axis = ranges.getLineObj( axes )
      x2axis = ranges.getZObj( axes )
      xlabel = "In-line"
      ylabel = attribinfo['zdomain']
      dim1blocksz = blocksinfo['size'][1]
      dim2blocksz = blocksinfo['size'][0]
      dim2rg = range( ranges.getLineObj(blocksrg)[0], ranges.getLineObj(blocksrg)[1]+1 )
    elif zdir:
      firstidx = 0
      secondidx = 1
      slicetxt = "Z slice"
      slicerg = ranges.getZObj( tkzs )
      slicevals = ranges.getZObj( axes )
      x1axis = ranges.getLineObj( axes )
      x2axis = ranges.getTraceObj( axes )
      xlabel = "In-line"
      ylabel = "Cross-line"
      dim1blocksz = blocksinfo['size'][0]
      dim2blocksz = blocksinfo['size'][1]
      dim1rg = range( ranges.getLineObj(blocksrg)[0], ranges.getLineObj(blocksrg)[1]+1 )
      dim2rg = range( ranges.getTraceObj(blocksrg)[0], ranges.getTraceObj(blocksrg)[1]+1 )
    elif slicesel == 0:
      cont="No"
      break
    else:
      continue
  if cont == "No":
    continue

  slicestep = slicerg.step
  slicenb = input( slicetxt + " number: " )
  sliceidx = 0
  try:
    slicenb = int(slicenb)
  except ValueError:
    print( "Please enter an integer\n" )
    continue
  if slicenb % slicestep != 0:
    print( slicetxt + " " + slicenb + " is not a multiple of the step: " + repr(slicerg.start) + "-" + repr(slicerg.stop) + " (step " + repr(slicestep) + ")" + "\n")
    continue
  try:
    sliceidx = slicerg.index( slicenb)
  except ValueError:
    print( slicetxt + " " + slicenb + "is not in range: " + repr(slicerg.start) + "-" + repr(slicerg.stop) + " (step" + repr(slicestep) + ")" + "\n")
    continue

  datagroup = h5file[ attribnm ]
  if len(datagroup) < 1:
    print( "Empty dataset found" )
    exit( 1 )

  datagroupset = list( datagroup.items() )
  datasetsamptype = datagroupset[0][1].dtype
  sliceout = numpy.zeros((len(x1axis),len(x2axis)),dtype=datasetsamptype)
  if inlinedir or crosslinedir:
    dim1str = int(sliceidx/dim1blocksz)
    for dim2str in dim2rg:
      if inlinedir:
        subcubenm = str(dim1str) + "." + str(dim2str)
      elif crosslinedir:
        subcubenm = str(dim2str) + "." + str(dim1str)
      subcube = datagroup.get( subcubenm )
      if subcube == None:
        continue
      dim1shift = 0
      dim2shift = 0
      if len(subcube.attrs['Loc00']) > 0:
        locorig = odhdf5.getIInterval( subcube, "Loc00" )
        dim1shift = locorig[firstidx]
        dim2shift = locorig[secondidx]
      dim1idx = sliceidx % dim1blocksz + dim1shift
      if dim1idx >= subcube.shape[firstidx]:
        continue
      firstx1 = dim2str * dim2blocksz + dim2shift
      lastx1 = firstx1 + subcube.shape[secondidx]
      try:
        if inlinedir:
          sliceout[firstx1:lastx1,:] = subcube[dim1idx]
        elif crosslinedir:
          sliceout[firstx1:lastx1,:] = subcube[:,dim1idx,:]
      except ValueError:
        print( "Target slice: " + str(slicenb) + "; Target 1st idx: " + str(sliceidx) )
        print( "dim2str: " + str(dim2str) + "; dim2blocksz: " + str(dim2blocksz) )
        print( "idy start: " + str(firstx1) + " idy stop: " + str(lastx1) )
        print( "SubCube: "+ subcubenm +  " dim1idx: " + str(dim1idx) )
        raise
  elif zdir:
    for dim1str in dim1rg:
      for dim2str in dim2rg:
        subcubenm = str(dim1str)+"."+str(dim2str)
        subcube = datagroup.get( subcubenm )
        if subcube == None:
          continue
        dim1shift = 0
        dim2shift = 0
        if len(subcube.attrs['Loc00']) > 0:
          locorig = odhdf5.getIInterval( subcube, "Loc00" )
          dim1shift = locorig[firstidx]
          dim2shift = locorig[secondidx]
        firstx1 = dim1str * dim1blocksz + dim1shift
        firstx2 = dim2str * dim2blocksz + dim2shift
        lastx1 = firstx1 + subcube.shape[firstidx]
        lastx2 = firstx2 + subcube.shape[secondidx]
        sliceout[firstx1:lastx1,firstx2:lastx2] = subcube[:,:,sliceidx]

  fig,ax = plt.subplots(1,1,sharex="col",sharey="row",facecolor="white")
  fig.subplots_adjust(top=0.95,bottom=0.05,left=0.05,right=0.93,hspace=0.02,wspace=0.15)
  plt.get_current_fig_manager().window.setGeometry(1920,0,1920,960)
  #TODO: Assign amplitude range from data
  im = ax.imshow(numpy.transpose(sliceout),cmap="bwr_r",aspect="auto",interpolation="sinc",vmin=-4500,vmax=4500,extent=[x1axis[0],x1axis[-1],x2axis[-1],x2axis[0]])
  ax.set_title("HDF5 Slice")
  ax.set_xlabel(xlabel)
  ax.set_ylabel(ylabel)
  axins = inset_axes(ax,width="1%",height="100%",loc=3,bbox_to_anchor=(1.02,0.,1,1),bbox_transform=ax.transAxes,borderpad=0)
  cbar = ax.figure.colorbar(im,cax=axins)
  print( "Close the plot to continue" )
  plt.show()

  cont = input( "Continue [Y/N] (Yes)? " )
  if not cont:
    cont = "Yes"
  print( "" )

#cubeposidx = list( h5file.keys() ).index("Seismic Cube Positions")
#cubepos = list(h5file.values())[cubeposidx]
cubepos = h5file["Seismic Cube Positions"]

h5file.close()

exit( 0 )
