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

### Horizon3D.ilines and Horizon3D.xlines properties
Return a list of the inline and crossline numbers covered by the horizon.

```python
mfs4.ilines[0:10]
```

### Horizon3D.getz(), Horizon3D.getxy() and Horizon3D.get_xarray() functions

-  getz(): return a Numpy 2D array with the horizon Z values
-  getxy(): return a tuple of Numpy 2D arrays with the X and Y coordinates of the horizon z values
-  get_xarray(): return an XArray DataArray with the horizon X, Y and Z values

```python tags=[]
import xarray as xr
hor = mfs4.get_xarray()
xr.plot.pcolormesh(hor, x='x', y='y', cmap='plasma_r', robust=True)
```

### Horizon3D.putz(), Horizon3D.putz_bycoord and Horizon3D.put_xarray functions

-  putz(): takes a Numpy 2D array of Z values, an inline number list/array and a crossline number list/array and saves it to an horizon
-  putz_bycoord(): takes a Numpy 2D array of z values, an array of X locations and an array of Y locations and saves it to an horizon
-  put_xarray(): takes horizon Z values and locations from a XArray DataFrame and saves it to an horizon

In all case values outside the creation limits of the horizon will be ignored. 

```python
newhor = Horizon3D.create(f3demo, 'newhor', [300,400,1], [500,700,1], True)
```

```python
newhor_z = newhor.get_xarray()
newhor_z[:] = 900.0
newhor.put_xarray(newhor_z)
```

```python
newhor_read = Horizon3D(f3demo,'newhor')
newhor_read.info()
```

### Horizon3D.feature() function
Returns a GeoJSON feature collection with the outline of the 3D horizon. This can be used in map displays.

```python
mfs4.feature()
```

```python
import folium
survmap = folium.Map(location=[52.3,8.0], tiles="Stamen Terrain", zoom_start = 6, min_lat=-90, max_lat=90, min_lon=-180, max_lon=180, max_bounds=True, maxBoundsViscosity=1)
folium.GeoJson(mfs4.feature(), popup=folium.GeoJsonPopup(fields=['name'])).add_to(survmap)
survmap
```

## Static methods
A number of methods are provided to get information either for all or a selected number of 3D horizons in the user provided survey.

```python
hors = Horizon3D.names(f3demo)
hors
```

### Horizon3D.infos() and Horizon3D.infos_dataframe() functions
These return a dictionary and a Pandas DataFrame respectively with basic information for the listed horizons (or all horizons if no list provided) in the given survey.

```python tags=[]
Horizon3D.infos_dataframe(f3demo)
```

### Horizon3D.features() function

Returns a GeoJSON feature collection for the listed horizons (or all horizons if no list provided) in the given survey. This can be used to create map displays.

```python tags=[]
Horizon3D.features(f3demo, hors[6:8])
```
