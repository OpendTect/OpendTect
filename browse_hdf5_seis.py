import sys
import os
import numpy as np
import h5py
import odpy.hdf5 as odhdf5
import odpy.ranges as ranges
import odpy.volpreload as volpreload
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1.inset_locator import inset_axes

if len(sys.argv) < 2:
  print( "No file name specified" )

filenm = sys.argv[1]
print( "\nBrowsing '" + filenm + "'\n\n" )

si = odhdf5.getSurveyInfo( filenm )
attribinfo = odhdf5.getAttribInfo( filenm )
samplinginfo = attribinfo['range']
datatkzs = ranges.getAxesAsRanges( samplinginfo )
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

cont = "Y"
while (cont == "Y") or (cont == "y" ) or (cont == "YES") or (cont == "Yes"):
  slicesel = -1
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
      slicetxt = "Inline"
      slicerg = ranges.getLineObj( datatkzs )
      tkzs = {
        'Crossline': ranges.getTraceObj( datatkzs ),
        'Z': ranges.getZObj( datatkzs )
      }
      x1axis = ranges.getTraceObj( axes )
      x2axis = ranges.getZObj( axes )
      xlabel = "Cross-line"
      ylabel = attribinfo['zdomain']
    elif crosslinedir:
      slicetxt = "Crossline"
      slicerg = ranges.getTraceObj( datatkzs )
      tkzs = {
        'Inline': ranges.getLineObj( datatkzs ),
        'Z': ranges.getZObj( datatkzs )
      }
      x1axis = ranges.getLineObj( axes )
      x2axis = ranges.getZObj( axes )
      xlabel = "In-line"
      ylabel = attribinfo['zdomain']
    elif zdir:
      slicetxt = "Z slice"
      slicerg = ranges.getZObj( datatkzs )
      tkzs = {
        'Inline': ranges.getLineObj( datatkzs ),
        'Crossline': ranges.getTraceObj( datatkzs )
      }
      x1axis = ranges.getLineObj( axes )
      x2axis = ranges.getTraceObj( axes )
      xlabel = "In-line"
      ylabel = "Cross-line"
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
    sliceidx = slicerg.index( slicenb )
  except ValueError:
    print( slicetxt + " " + slicenb + "is not in range: " + repr(slicerg.start) + "-" + repr(slicerg.stop) + " (step" + repr(slicestep) + ")" + "\n")
    continue

  if slicesel == 1:
    step = ranges.getLineObj( datatkzs ).step
    tkzs['Inline'] = range(slicenb,slicenb+step,step)
  elif slicesel == 2:
    step = ranges.getTraceObj( datatkzs ).step
    tkzs['Crossline'] = range(slicenb,slicenb+step,step)
  elif slicesel == 3:
    step = ranges.getZObj( datatkzs ).step
    tkzs['Z'] = range(slicenb,slicenb+step,step)

  sliceout = volpreload.preLoad( filenm, attribnm, tkzs )
  if len(sliceout) == 0:
    exit(1)

  fig,ax = plt.subplots(1,1,sharex="col",sharey="row",facecolor="white")
  fig.subplots_adjust(top=0.95,bottom=0.05,left=0.05,right=0.93,hspace=0.02,wspace=0.15)
  plt.get_current_fig_manager().window.setGeometry(1920,0,1920,960)
  #TODO: Assign amplitude range from data
  im = ax.imshow(np.transpose(sliceout),cmap="seismic_r",aspect="auto",interpolation="quadric",vmin=-8000,vmax=8000,extent=[x1axis[0],x1axis[-1],x2axis[-1],x2axis[0]])
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

h5file = h5py.File( filenm, "r" )
#cubeposidx = list( h5file.keys() ).index("Seismic Cube Positions")
#cubepos = list(h5file.values())[cubeposidx]
cubepos = h5file["Seismic Cube Positions"]
h5file.close()

exit( 0 )
