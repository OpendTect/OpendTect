import sys
import os
import numpy
import h5py
import odpy.hdf5 as odhdf5
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1.inset_locator import inset_axes

if len(sys.argv) < 2:
  filenm = "/auto/d55/surveys/F3_Demo_d55/Seismics/4_Dip_steered_median_filter_blocks.hdf5"
else:
  filenm = sys.argv[1]

h5file = h5py.File( filenm, "r" )
print( "\nBrowsing '" + filenm + "'\n\n" )

infods = odhdf5.getInfoDataSet( h5file )
def gtInfAttr( ky ):
  return odhdf5.getAttr( infods, ky )
def gtInfAttrs( ky ):
  return odhdf5.getAttrs( infods, ky )

# directly gettable info
blocksversion = int( gtInfAttr("Blocks.Version") )
surveynm   = gtInfAttr( "Name.Survey" );
datasetnm  = gtInfAttr( "Name.Cube" )
zdomain    = gtInfAttr( "ZDomain" )
inlstart   = int( gtInfAttr("First In-line") )
crlstart   = int( gtInfAttr("First Cross-line") )
inl_step   = int( gtInfAttr("Step In-line") )
crl_step   = int( gtInfAttr("Step Cross-line") )
inlstop    = int( gtInfAttr("Last In-line") )
crlstop    = int( gtInfAttr("Last Cross-line") )
compnms    = gtInfAttrs( "Components" )

# just get, use/parse later
datacharattr = gtInfAttr( "Data storage" )
xrgattr	   = gtInfAttrs( "X range" )
yrgattr	   = gtInfAttrs( "Y range" )
zrgattr    = gtInfAttrs( "Z range" )
coordsxbidattr = gtInfAttrs( "Coord-X-BinID" )
coordsybidattr = gtInfAttrs( "Coord-Y-BinID" )
inlinesattr	   = gtInfAttrs( "In-line range" )
crosslinesattr = gtInfAttrs( "Cross-line range" )
blocksdimattr  = gtInfAttrs( "Blocks.Max Dimensions" )
blocksinlrgattr  = gtInfAttrs( "Blocks.Inl ID Range" )
blockscrlrgattr  = gtInfAttrs( "Blocks.Crl ID Range" )

# Survey transformation (X-Y) to (Inl-Crl)
# Given an Inl-Crl pair: ic_pos = numpy.array( [inl,crl] ).astype(numpy.float)
# the following returns the corresponding X,Y location:
# xy_pos = survtrans_orig + numpy.dot( survtrans_rotmat, ic_pos )
#
survtrans_origt  = numpy.array( [coordsxbidattr[0],coordsybidattr[0]] ).astype(numpy.float)
survtrans_rotmat = numpy.array( [[coordsxbidattr[1],coordsxbidattr[2]],
                              [coordsybidattr[1],coordsybidattr[2]]] ).astype(numpy.float)

# Positions as ranges or start/stop/step array:
surv_inlrg = range(int(inlinesattr[0]),int(inlinesattr[1])+inl_step,inl_step)
surv_crlrg = range(int(crosslinesattr[0]),int(crosslinesattr[1])+crl_step,crl_step)
surv_xrange = numpy.array( [xrgattr[0],xrgattr[1]] ).astype(numpy.float)
surv_yrange = numpy.array( [yrgattr[0],yrgattr[1]] ).astype(numpy.float)
inlrg = range(inlstart,inlstop+inl_step,inl_step)
crlrg = range(crlstart,crlstop+crl_step,crl_step)
zrg   = numpy.array( [zrgattr[0],zrgattr[1],zrgattr[2]] ).astype(numpy.float)
if zdomain == "TWT":
  zrg = numpy.multiply(zrg,1000).astype("int32")
zstep = zrg[2]
zrg = range(zrg[0],zrg[1]+zstep,zstep)

# Positions as iterable arrays:
surv_inlines    = numpy.linspace( surv_inlrg.start, surv_inlrg.stop, len(surv_inlrg), dtype=numpy.int32 )
surv_crosslines = numpy.linspace( surv_crlrg.start, surv_crlrg.stop, len(surv_crlrg), dtype=numpy.int32 )
inlines    = numpy.linspace( inlrg.start, inlrg.stop, len(inlrg), dtype=numpy.int32 )
crosslines = numpy.linspace( crlrg.start, crlrg.stop, len(crlrg), dtype=numpy.int32 )
zsamp      = numpy.linspace( zrg.start, zrg.stop, len(zrg), dtype=numpy.int32 )

blocks = { "Inline Dim": int(blocksdimattr[0]), "Crossline Dim": int(blocksdimattr[1]),
           "First.Inl ID": int(blocksinlrgattr[0]), "First.Crl ID": int(blockscrlrgattr[0]),
           "Last.Inl ID": int(blocksinlrgattr[1]), "Last.Crl ID": int(blockscrlrgattr[1]) }

print( "Data is stored as " + datacharattr + "\n" )
# TODO: add gap information: The cube has varying cross-line ranges. / The cube is 100% rectangular.

usrinp = "1"
nrcomps = len( compnms )
if nrcomps > 1:
  for idx, compnm in enumerate(compnms):
    print( "Component " + str(idx+1) + ": '" + compnm + "'" )
  usrinp = input( "Enter component index: " )
cidx = int( usrinp ) - 1
if cidx < 0:
  cidx = 0
if cidx >= nrcomps:
  cidx = nrcomps - 1

print( "" )
print( "In-line range: " + repr(inlrg.start) + "-" + repr(inlrg.stop-inlrg.step) + " (step " + repr(inl_step) + ")" )
print( "Cross-line range: " + repr(crlrg.start) + "-" + repr(crlrg.stop-crlrg.step) + " (step " + repr(crl_step) + ")" )
print( "" )
print( "Z range: " + repr(zrg.start) + "-" + repr(zrg.stop-zrg.step) + " (step " + repr(zrg.step) + ")" )
print( "Number of samples: " + repr(len(zsamp)) )
print( "" )

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
      slicevals = inlines
      slicerg = inlrg
      x1axis = crosslines
      x2axis = zsamp
      xlabel = "Cross-line"
      ylabel = zdomain
      dim1blocksz = blocks.get("Inline Dim")
      dim2blocksz = blocks.get("Crossline Dim")
      dim2rg = range( blocks.get("First.Crl ID"), blocks.get("Last.Crl ID")+1 )
    elif crosslinedir:
      firstidx = 1
      secondidx = 0
      slicetxt = "Crossline"
      slicevals = crosslines
      slicerg = crlrg
      x1axis = inlines
      x2axis = zsamp
      xlabel = "In-line"
      ylabel = zdomain
      dim1blocksz = blocks.get("Crossline Dim")
      dim2blocksz = blocks.get("Inline Dim")
      dim2rg = range( blocks.get("First.Inl ID"), blocks.get("Last.Inl ID")+1 )
    elif zdir:
      firstidx = 0
      secondidx = 1
      slicetxt = "Z slice"
      slicevals = zsamp
      slicerg = zrg
      x1axis = inlines
      x2axis = crosslines
      xlabel = "In-line"
      ylabel = "Cross-line"
      dim1blocksz = blocks.get("Inline Dim")
      dim2blocksz = blocks.get("Crossline Dim")
      dim1rg = range( blocks.get("First.Inl ID"), blocks.get("Last.Inl ID")+1 )
      dim2rg = range( blocks.get("First.Crl ID"), blocks.get("Last.Crl ID")+1 )
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

  datagroup = h5file[ compnms[cidx] ]
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
      if len(subcube.attrs.get("Loc00")) > 0:
        locorig = hdf5.getAttrs( subcube, "Loc00" );
        dim1shift = int( locorig[firstidx] )
        dim2shift = int( locorig[secondidx] )
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
        if len(subcube.attrs.get("Loc00")) > 0:
          locorig = hdf5.getAttrs( subcubem, "Loc00" );
          dim1shift = int( locorig[firstidx] )
          dim2shift = int( locorig[secondidx] )
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
