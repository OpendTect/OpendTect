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

# Horizon3D Class - ODBind Python Bindings


**If this notebook is not opened from OpendTect then the following paths should be set for your system and the cell executed.**

```python
#import os
#import sys
#odpy_path = '/home/wayne/Work/WMSeismicSolutions/dGB/Development/Build/bin/od7.0/bin/python'
#sys.path.insert(0,odpy_path)
```

```python
import numpy as np
from odbind.survey import Survey
from odbind.horizon3d import Horizon3D
```

## Horizon3D class

The Horizon3D class provides access to 3D horizons in an OpendTect project/survey. A Survey object is required for context.

The user must create an **Horizon3D** object to access a specific 3D horizon. There are 2 constructors provided:
-   **Horizon3D( survey:odSurvey, horizon_name:str )** which opens an existing horizon called "horizon_name" if it exists or raises an exception.
-   **Horizon3D.create( survey:odSurvey, horizon_name:str, iline_range:range, xline_range:range, overwrite:bool )** which creates a new 3D horizon called "horizon_name" with the specified extents. By default "overwrite" is set to false so if "horizon_name" already exists the statement will fail but if true any existing 3D horizon of the same name will be replaced.

```python
f3demo = Survey('F3_Demo_2020')
```

### Horizon3D.info() function
Returns basic information for a 3D horizon in a Python dictionary

```python
mfs4 = Horizon3D(f3demo, 'Demo 1 --> MFS4')
mfs4.info()
```

### Horizon3D.attribnames property
Return a list of the attributes attached to this horizon.

```python
mfs4.attribnames
```

### Horizon.ranges properties
Returns a named tuple with the inline and crossline range of the horizon.

```python
mfs4.ranges
```

### Reading 3D Horizon Data

The Horizon3D.getdata() function returns the horizon z values and optionally the horizon data attributes listed by named in the only parameter as either an Xarray.Dataset or simple list+dict format.

```python tags=[]
import xarray as xr
Horizon3D.use_xarray = True
hor = mfs4.getdata(['SD_24Hz[-8,24ms]','SD_64Hz[-8,24ms]'])
hor
```

```python
xr.plot.pcolormesh(hor['SD_64Hz[-8,24ms]'], x='x', y='y', cmap='plasma_r', robust=True)
```

### Writing 3D Horizon Data
The Horizon3D.create method is used to create a new output horizon or overwrite an existing horizon.
The Horizon3D.putdata() function is used to write the horizon data. The input data can be either an Xarray.Dataset or a simple tuple of list+dict as produced by Horizon3D.getdata. 

Values at locations outside the creation limits of the horizon are ignored. 

```python
with Horizon3D.create(f3demo, 'newhor', [300,400,1], [500,700,1], True) as newhor:
    newhor.putdata(hor)
```

```python
newhor_read = Horizon3D(f3demo,'newhor')
newhor_read.info()
```

### Deleting 3D Horizon Data

```python
newhor_read.delete_attribs(['SD_64Hz[-8,24ms]'])
newhor_read.info()                       
```

### Deleting 3D Horizons

```python
Horizon3D.delete(f3demo,['newhor'])
```

### Horizon3D.feature() function
Returns a GeoJSON feature collection with the outline of the 3D horizon. This can be used in map displays.

```python
mfs4.feature()
```

```python
import folium
survmap = folium.Map(location=[54.2,5.0], zoom_start = 8)
folium.GeoJson(mfs4.feature(), popup=folium.GeoJsonPopup(fields=['name'])).add_to(survmap)
survmap
```

## Static methods
A number of methods are provided to get information either for all or a selected number of 3D horizons in the user provided survey.

```python
hors = Horizon3D.names(f3demo)
hors
```

### Horizon3D.infos() functions
Return basic information for the listed horizons (or all horizons if no list provided) in the given survey in either a Pandas DataFrame or simple list+dict format depending on the value of Horizon3D.use_dataframe.

```python tags=[]
Horizon3D.infos(f3demo)
```

### Horizon3D.features() function

Returns a GeoJSON feature collection for the listed horizons (or all horizons if no list provided) in the given survey. This can be used to create map displays.

```python tags=[]
Horizon3D.features(f3demo, hors[6:8])
```
