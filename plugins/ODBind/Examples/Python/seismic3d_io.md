# Seismic3D Class IO Examples- ODBind Python Bindings


**If this notebook is not opened from OpendTect then the following paths should be set for your system and the cell executed.**

```python
#import os
#import sys
#odpy_path = '/home/wayne/Work/WMSeismicSolutions/dGB/Development/Build/bin/odmain/bin/python'
#sys.path.insert(0,odpy_path)
```

```python
import numpy as np
from matplotlib import pyplot as plt
import xarray as xr
from odbind.survey import Survey
from odbind.seismic3d import Seismic3D
```

```python
f3demo = Survey('F3_Demo_2020')
vol = Seismic3D(f3demo, '4 Dip steered median filter')
```

**Seismic3D** objects have special properties to access the associated 3D seismic volume:
- iline for access to inline slices
- xline for access to crossline slices
- zslice for access to Z slices
- trace for access to individual traces
- volume for access to sub-volumes

Use the indexing operator [] with these properties to select the data to access, eg iline[300], volume[200:300,300:500,100:200]. In the case of Inline and Crossline slices
the numbers used in the index used are the actual inline and crossline numbers. In the case of Z slices the numbers used in the index are the sample numbers. These indices
mostly behave like Python indexing except:
-  negative indices do not have special meaaning (access from end of data)
-  the range implied by a slice includes both endpoints (compared with Python which excludes the slice.stop index)

All components of the seismic data are returned as a tuple of:
-  a list of numpy arrays, one for each seismic component
-  a Python dict with information about the data

The information dict has the following keys and data:
-  'comp': list[str] of the seismic component names
-  'iline': int (for inline slice) or list[int] with the inline start, stop and step
-  'xline': int (for crossline slice) or list[int] with the crossline start, stop and step
-  'x': double | np.ndarray(double) with the x coordinates of the traces  
-  'y': double | np.ndarray(double) with the y coordinates of the traces
-  'twt' | 'depth': float (for z slice) or list[float] with the Z start, stop and step
-  'dims': ['iline|xline', 'twt|depth'] for inline or crossline slices or ['iline', 'zline'] for z slices

The **Seismic3D.as_xarray** method can be used to convert the output from the indexing operator to an Xarray DataSet with coordinates of:
-  the component (comp)
-  inline number (iline)
-  crossline number (xline)
-  Easting in the survey CRS (cdp_x)
-  Northing in the survey CRS (cdp_y)
-  TWT/Depth (twt or depth)



## Slice Access Mode
### Inline Slices
Indiviudal inlines can be accessed by vol.iline[400] or a series of inline slices can be accessed by vol.iline[200:220:10]:

```python
inln = vol.as_xarray(*vol.iline[400])
xr.plot.imshow(inln['mdf2'], x='xline', y='twt', yincrease=False, cmap='Greys')
```

```python
fig, axs = plt.subplots(figsize=(25,5), ncols=3)
for iln, ax in zip(vol.iline[200:220:10],axs):
    xriln = vol.as_xarray(*iln)
    xr.plot.imshow(xriln['mdf2'], ax=ax, x='xline', y='twt', yincrease=False, cmap='Greys')
```

### Crossline Access Mode
Indiviudal crosslines can be accessed by vol.xline[400] or a series of crossline slices can be accessed by vol.xline[200:220:10]:

```python
xrxln = vol.as_xarray(*vol.xline[400])
xr.plot.imshow(xrxln['mdf2'], x='iline', y='twt', yincrease=False, cmap='Greys')
```

### Z Slice Access Mode
Indiviudal Z slices can be accessed by vol.zslice[200] or a series of crossline slices can be accessed by vol.xline[200:220:10]:

```python
zsl = vol.as_xarray(*vol.zslice[200])
xr.plot.pcolormesh(zsl['mdf2'], x='cdp_x', y='cdp_y', cmap='Greys')
```

### Volume Access Mode
Rectangular sub-volumes can be accessed by vol.volume[inline_slice, crossline_slice, Z_range_list] or vol.volume[inline_slice, crossline_slice, zsample_slice]. The difference being
the Z_range_list specifies the volume extent and sampling in the actual Z units, eg [100.,200, 20.] , while the zsample_slice uses integer sample numbers. For data starting at
Z=0 and sampled at a step of 4 the corresponding zsample_slice would be 25:50:5.

Examples:
-  vol[:,:,:], vol[:,:] or vol[:]  is the entire volume
-  vol[300:350] is all traces for inlines 300 to 350 
-  vol[300:350:2]  is all traces from every second inline from 300 to 350
-  vol[:,300:350:2] is all traces from every second crossline from 300 to 350 
-  vol[:,:,100:200] is 101 samples (400-800ms for a time volume starting at 0 witha z step of 4ms) from all traces in the volume 
-  vol[300:350,500:600,[400,800,4]] is samples between 400 and 800ms for all traces from inlines 300-350 and crosslines 500-600

```python
subvol = vol.as_xarray(*vol.volume[200:300,400:600,[800,900,4]])
xr.plot.pcolormesh(subvol['mdf2'][:,:,0], x='cdp_x', y='cdp_y',cmap='Greys')
```
