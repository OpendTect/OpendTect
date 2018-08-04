import os
import platform
import sys
import numpy as np
import h5py
import matplotlib.pyplot as plt

from mpl_toolkits.axes_grid1.inset_locator import inset_axes

def getDataSetInfos( fd ):
  return fd['++info++']

if platform.python_version() < '3':
  print("Unsuitable python version: Must be >=3")
  exit( 1 )

if len(sys.argv) < 2:
  print("Incorrect arguments")
  print("Syntax: python(3) "+os.path.basename(__file__)+" filename.h5")
  exit( 1 )

#filenm = "/auto/d55/surveys/F3_Demo_d55/Seismics/4_Dip_steered_median_filter_blocks.hdf5"
filenm = sys.argv[1]
h5file = h5py.File(filenm,'r')
print("Browsing '%s'\n\n" % (filenm))

infos = getDataSetInfos( h5file )

#TODO: make a dictonary of the loaded info objects
blocksversion = int(np.bytes_(infos.attrs['Blocks.Version']).decode().rstrip().split('\x00')[0])

surveynm   = np.bytes_(infos.attrs['Name.Survey']).decode().rstrip().split('\x00')[0]
datasetnm  = np.bytes_(infos.attrs['Name.Cube']).decode().rstrip().split('\x00')[0]
xrgstr	   = np.bytes_(infos.attrs['X range']).decode().rstrip().replace('\x00','`').split('`')
yrgstr	   = np.bytes_(infos.attrs['Y range']).decode().rstrip().replace('\x00','`').split('`')
datacharnm = np.bytes_(infos.attrs['Data storage']).decode().rstrip().split('\x00')[0]
zdomain    = np.bytes_(infos.attrs['ZDomain']).decode().rstrip().split('\x00')[0]
compnms    = np.bytes_(infos.attrs['Components']).decode().rstrip().split('`')
coordsxbid = np.bytes_(infos.attrs['Coord-X-BinID']).decode().rstrip().replace('\x00','`').split('`')
coordsybid = np.bytes_(infos.attrs['Coord-Y-BinID']).decode().rstrip().replace('\x00','`').split('`')
inlines	   = np.bytes_(infos.attrs['In-line range']).decode().rstrip().replace('\x00','`').split('`')
crosslines = np.bytes_(infos.attrs['Cross-line range']).decode().rstrip().replace('\x00','`').split('`')
zrangestr  = np.bytes_(infos.attrs['Z range']).decode().rstrip().replace('\x00','`').split('`')
inlstart   = int(np.bytes_(infos.attrs['First In-line']).decode().rstrip().split('\x00')[0])
crlstart   = int(np.bytes_(infos.attrs['First Cross-line']).decode().rstrip().split('\x00')[0])
inl_step   = int(np.bytes_(infos.attrs['Step In-line']).decode().rstrip().split('\x00')[0])
crl_step   = int(np.bytes_(infos.attrs['Step Cross-line']).decode().rstrip().split('\x00')[0])
inlstop    = int(np.bytes_(infos.attrs['Last In-line']).decode().rstrip().split('\x00')[0])
crlstop    = int(np.bytes_(infos.attrs['Last Cross-line']).decode().rstrip().split('\x00')[0])
blocksdim  = np.bytes_(infos.attrs['Blocks.Max Dimensions']).decode().rstrip().replace('\x00','`').split('`')
blocksinlrg  = np.bytes_(infos.attrs['Blocks.Inl ID Range']).decode().rstrip().replace('\x00','`').split('`')
blockscrlrg  = np.bytes_(infos.attrs['Blocks.Crl ID Range']).decode().rstrip().replace('\x00','`').split('`')

# Survey transformation (X-Y) to (Inl-Crl)
# Given an Inl-Crl pair: ic_pos = np.array( [inl,crl] ).astype(np.float)
# the following returns the corresponding X,Y location:
# xy_pos = survtrans_orig + np.dot( survtrans_rotmat, ic_pos )
#
survtrans_origt  = np.array( [coordsxbid[0],coordsybid[0]] ).astype(np.float)
survtrans_rotmat = np.array( [[coordsxbid[1],coordsxbid[2]],
                              [coordsybid[1],coordsybid[2]]] ).astype(np.float)

# Positions as ranges or start/stop/step array:
surv_inlrg = range(int(inlines[0]),int(inlines[1])+inl_step,inl_step)
surv_crlrg = range(int(crosslines[0]),int(crosslines[1])+crl_step,crl_step)
surv_xrange = np.array( [xrgstr[0],xrgstr[1]] ).astype(np.float)
surv_yrange = np.array( [yrgstr[0],yrgstr[1]] ).astype(np.float)
inlrg = range(inlstart,inlstop+inl_step,inl_step)
crlrg = range(crlstart,crlstop+crl_step,crl_step)
zrg   = np.array( [zrangestr[0],zrangestr[1],zrangestr[2]] ).astype(np.float)
if zdomain == 'TWT':
  zrg = np.multiply(zrg,1000).astype('int32')
zstep = zrg[2]
zrg = range(zrg[0],zrg[1]+zstep,zstep)

# Positions as iterable arrays:
surv_inlines    = np.linspace( surv_inlrg.start, surv_inlrg.stop, len(surv_inlrg), dtype=np.int32 )
surv_crosslines = np.linspace( surv_crlrg.start, surv_crlrg.stop, len(surv_crlrg), dtype=np.int32 )
inlines    = np.linspace( inlrg.start, inlrg.stop, len(inlrg), dtype=np.int32 )
crosslines = np.linspace( crlrg.start, crlrg.stop, len(crlrg), dtype=np.int32 )
zsamp      = np.linspace( zrg.start, zrg.stop, len(zrg), dtype=np.int32 )

blocks = { "Inline Dim": int(blocksdim[0]), "Crossline Dim": int(blocksdim[1]),
           "First.Inl ID": int(blocksinlrg[0]), "First.Crl ID": int(blockscrlrg[0]),
           "Last.Inl ID": int(blocksinlrg[1]), "Last.Crl ID": int(blockscrlrg[1]) }

print("Data is stored as %s\n" % (datacharnm))
# TODO: add gap information: The cube has varying cross-line ranges. / The cube is 100% rectangular.

if len(compnms) > 1:
  for idx, compnm in enumerate(compnms):
    print("Component %s: '%s'" % (idx+1,compnm))
  cidx = input('Enter component index')-1
else:
  cidx = 0

print('')
print('In-line range:',repr(inlrg.start),'-',repr(inlrg.stop-inlrg.step),'( step',repr(inl_step),')')
print('Cross-line range:',repr(crlrg.start),'-',repr(crlrg.stop-crlrg.step),'( step',repr(crl_step),')')
print('')
print('Z range:',repr(zrg.start),'-',repr(zrg.stop-zrg.step),'( step',repr(zrg.step),')')
print('Number of samples:',repr(len(zsamp)) )
print('')

cont = "Y"
while (cont == "Y") or (cont == "y" ) or (cont == "YES") or (cont == "Yes"):
  slicesel = -1
  #TODO: Add volume sub-sel (3D pre-loading)
  while (slicesel != 1) and (slicesel != 2) and (slicesel != 3):
    slicesel = input("Slice type: [1-In-line] [2-Cross-line] [3-Z slice]  ")
    try:
      slicesel = int(slicesel)
    except ValueError:
      cont='No'
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
      dim1blocksz = blocks.get('Inline Dim')
      dim2blocksz = blocks.get('Crossline Dim')
      dim2rg = range( blocks.get('First.Crl ID'), blocks.get('Last.Crl ID')+1 )
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
      dim1blocksz = blocks.get('Crossline Dim')
      dim2blocksz = blocks.get('Inline Dim')
      dim2rg = range( blocks.get('First.Inl ID'), blocks.get('Last.Inl ID')+1 )
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
      dim1blocksz = blocks.get('Inline Dim')
      dim2blocksz = blocks.get('Crossline Dim')
      dim1rg = range( blocks.get('First.Inl ID'), blocks.get('Last.Inl ID')+1 )
      dim2rg = range( blocks.get('First.Crl ID'), blocks.get('Last.Crl ID')+1 )
    elif slicesel == 0:
      cont='No'
      break
    else:
      continue
  if cont == 'No':
    continue

  slicestep = slicerg.step
  slicenb = input(slicetxt+" number: ")
  sliceidx = 0
  try:
    slicenb = int(slicenb)
  except ValueError:
    print('Please enter an integer','\n')
    continue
  if slicenb % slicestep != 0:
    print(slicetxt,slicenb,'is not a multiple of the step:',repr(slicerg.start),'-',repr(slicerg.stop),'( step',repr(slicestep),')','\n')
    continue
  try:
    sliceidx = slicerg.index( slicenb)
  except ValueError:
    print(slicetxt,slicenb,'is not in range:',repr(slicerg.start),'-',repr(slicerg.stop),'( step',repr(slicestep),')','\n')
    continue

  datagroup = h5file[compnms[cidx]]
  if len(datagroup) < 1:
    print("Empty dataset found")
    exit( 1 )

  datagroupset = list( datagroup.items() )
  datasetsamptype = datagroupset[0][1].dtype
  sliceout = np.zeros((len(x1axis),len(x2axis)),dtype=datasetsamptype)
  if inlinedir or crosslinedir:
    dim1str = int(sliceidx/dim1blocksz)
    for dim2str in dim2rg:
      if inlinedir:
        subcubenm = str(dim1str)+'.'+str(dim2str)
      elif crosslinedir:
        subcubenm = str(dim2str)+'.'+str(dim1str)
      subcube = datagroup.get( subcubenm )
      if subcube == None:
        continue
      dim1shift = 0
      dim2shift = 0
      if len(subcube.attrs.get('Loc00')) > 0:
        locorigstr = np.bytes_(subcube.attrs.get('Loc00')).decode().rstrip().replace('\x00','`').split('`')
        dim1shift = int(locorigstr[firstidx])
        dim2shift = int(locorigstr[secondidx])
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
        print( "Target slice: "+ str(slicenb)+'; Target 1st idx: '+str(sliceidx) )
        print( "dim2str: "+ str(dim2str)+'; dim2blocksz: '+str(dim2blocksz) )
        print( "idy start: "+ str(firstx1)+' idy stop: '+str(lastx1) )
        print( "SubCube: "+ subcubenm, "Dim1idx: "+str(dim1idx) )
        raise
  elif zdir:
    for dim1str in dim1rg:
      for dim2str in dim2rg:
        subcubenm = str(dim1str)+'.'+str(dim2str)
        subcube = datagroup.get( subcubenm )
        if subcube == None:
          continue
        dim1shift = 0
        dim2shift = 0
        if len(subcube.attrs.get('Loc00')) > 0:
          locorigstr = np.bytes_(subcube.attrs.get('Loc00')).decode().rstrip().replace('\x00','`').split('`')
          dim1shift = int(locorigstr[firstidx])
          dim2shift = int(locorigstr[secondidx])
        firstx1 = dim1str * dim1blocksz + dim1shift
        firstx2 = dim2str * dim2blocksz + dim2shift
        lastx1 = firstx1 + subcube.shape[firstidx]
        lastx2 = firstx2 + subcube.shape[secondidx]
        sliceout[firstx1:lastx1,firstx2:lastx2] = subcube[:,:,sliceidx]

  fig,ax = plt.subplots(1,1,sharex='col',sharey='row',facecolor='white')
  fig.subplots_adjust(top=0.95,bottom=0.05,left=0.05,right=0.93,hspace=0.02,wspace=0.15)
  plt.get_current_fig_manager().window.setGeometry(1920,0,1920,960)
  #TODO: Assign amplitude range from data
  im = ax.imshow(np.transpose(sliceout),cmap="bwr_r",aspect='auto',interpolation='sinc',vmin=-4500,vmax=4500,extent=[x1axis[0],x1axis[-1],x2axis[-1],x2axis[0]])
  ax.set_title("HDF5 Slice")
  ax.set_xlabel(xlabel)
  ax.set_ylabel(ylabel)
  axins = inset_axes(ax,width="1%",height="100%",loc=3,bbox_to_anchor=(1.02,0.,1,1),bbox_transform=ax.transAxes,borderpad=0)
  cbar = ax.figure.colorbar(im,cax=axins)
  print( "Close the plot to continue" )
  plt.show()

  cont = input("Continue [Y/N] (Yes)? ")
  if not cont:
    cont = 'Yes'
  print('')

#cubeposidx = list( h5file.keys() ).index('Seismic Cube Positions')
#cubepos = list(h5file.values())[cubeposidx]
cubepos = h5file['Seismic Cube Positions']

h5file.close()

exit( 0 )
