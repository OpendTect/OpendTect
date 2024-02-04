---
jupyter:
  jupytext:
    formats: ipynb,md
    text_representation:
      extension: .md
      format_name: markdown
      format_version: '1.3'
      jupytext_version: 1.14.0
  kernelspec:
    display_name: Python 3 (ipykernel)
    language: python
    name: python3
---

# Seismic3D Class - ODBind Python Bindings


**If this notebook is not opened from OpendTect then the following paths should be set for your system and the cell executed.**

```python
#import os
#import sys
#odpy_path = '/home/wayne/Work/WMSeismicSolutions/dGB/Development/Build/bin/odmain/bin/python'
#sys.path.insert(0,odpy_path)
```

```python
import numpy as np
from odbind.survey import Survey
from odbind.seismic3d import Seismic3D
```

## Seismic3D class

The Seismic3D class provides access to 3D seismic volumes in an OpendTect project/survey. A Survey object is required for context.

The user must create a **Seismic3D** object to access a specific volume. There are 2 constructors provided:
-   **Seismic3D( survey:odSurvey, vol_name:str )** which opens an existing volume called "vol_name" if it exists or raises an exception.


```python
f3demo = Survey('F3_Demo_2020')
```

### Seismic3D.info() function
Returns basic information for a 3D seismic volume in a Python dictionary

```python
vol = Seismic3D(f3demo, '4 Dip steered median filter')
vol.info()
```

### Seismic3D.feature function
Return a GeoJSON feature collection with the bounding box of the 3D seismic volume. This can be used in map displays.

```python
feature = vol.feature()
feature
```

```python
import folium
volmap = folium.Map(location=[54.2,5.0], zoom_start = 8)
folium.GeoJson(feature, popup=folium.GeoJsonPopup(fields=['name'])).add_to(volmap)
volmap
```

### Seismic3D.comp_names property
Return a list of the component names in the 3D seismic volume.

```python
vol.comp_names
```

### Seismic3D.bin_count and trace_count properties
- bin_count returns the number of inline, crossline bin locations in the 3D seismic volume area.
- trace_count returns the actual number of traces in the 3D seismic volume.

Generally trace_count<=bin_count.

```python
print(vol.bin_count, vol.trace_count)
```

### Seismic3D.bin and trace_index functions
- bin returns the inline and crossline location for a given trace index number
- trace_index returns the trace_index for a give inline, crossline location

The first trace of the volume has a trace_index of 0 and the last trace a trace_index of trace_count-1.
Both functions will raise an IndexError if the inputs are invalid for the 3D seismic volume.

```python
vol.trace_index(100, 400)
```

```python
vol.bin(100)
```

### Reading 3D Seismic Data
#### By Inline

```python
from matplotlib import pyplot as plt
import xarray as xr

xrinl = vol.iline[400]
xr.plot.imshow(xrinl['mdf2'], x='xline', y='twt', yincrease=False, cmap='Greys')
```

#### By Crossline

```python
xrcrl = vol.xline[800]
xr.plot.imshow(xrcrl['mdf2'], x='iline', y='twt', yincrease=False, cmap='Greys')
```

#### By ZSlice

```python
xrzsl = vol.zslice[vol.z_index(1600)]
xr.plot.pcolormesh(xrzsl['mdf2'], x='x', y='y', cmap='Greys')
```

#### By Trace

```python
xrtrc = vol.trace[-10]
xrtrc['mdf2'].plot(figsize=(15,1))
```

```python
xrtrc = vol.trace[750,1241]
xrtrc['mdf2'].plot(figsize=(15,1))
```

```python
vol.trace[20:40]
```

#### By Subvolume

```python
subvol = vol.volume[200:300,400:600,[800,900,4]]
subvol
```

```python
f3demo.crlrange
```

### Creating 3D Seismic Data
#### By Subvolume

```python
inlrg, crlrg, zrg = vol.ranges
with Seismic3D.create(f3demo, 'create_test', inlrg, crlrg, [100,200,4], ['comp1'], 'CBVS', True, True) as test:
        test.volume[:] = vol.volume[300:400:10, 600:620,[100,200,4]]
        
Seismic3D(f3demo,'create_test').info()
```

#### By Inline

```python
inlrg, crlrg, zrg = vol.ranges
with Seismic3D.create(f3demo, 'create_test', inlrg, crlrg, zrg, ['comp1'], 'CBVS', True, True) as test:
        test.iline[:] = vol.iline[300:310:2]
        
Seismic3D(f3demo,'create_test').info()
```

#### By Trace

```python
inlrg, crlrg, zrg = vol.ranges
with Seismic3D.create(f3demo, 'create_test', inlrg, crlrg, zrg, ['comp1'], 'CBVS', True, True) as test:
        test.trace[:] = vol.trace[200:220,400:500:100]
        
Seismic3D(f3demo,'create_test').info()    
```

### Deleting 3D Seismic Data

```python
Seismic3D.delete(f3demo, ['create_test'])
```

## Static methods
A number of methods are provided to get information either for all or a selected number of wells in the user provided survey.

```python
vols = Seismic3D.names(f3demo)
vols
```

### Seismic3D.infos() functions
Return basic information for the listed 3D volumes (or all volumes if no list provided) in the given survey in either a Pandas DataFrame or simple list+dict format depending on the value of Seismic3D.use_dataframe.

```python
Seismic3D.infos(f3demo)

```

### Seismic3D.features() function

Returns a GeoJSON feature collection for the listed 3D seismic volumes (or all volumes if no list provided) in the given survey. This can be used to create map displays.

```python
features = Seismic3D.features(f3demo,['1 Original Seismics', '3a Steering PCA111 MF225'])
features
```

```python
import folium
volmap = folium.Map(location=[54.2,5.0], zoom_start = 8)
folium.GeoJson(features, popup=folium.GeoJsonPopup(fields=['name'])).add_to(volmap)
volmap
```
