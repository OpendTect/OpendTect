---
jupyter:
  jupytext:
    formats: ipynb,md
    text_representation:
      extension: .md
      format_name: markdown
      format_version: '1.3'
      jupytext_version: 1.14.5
  kernelspec:
    display_name: Python 3 (ipykernel)
    language: python
    name: python3
---

# Horizon2D Class - ODBind Python Bindings


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
from odbind.horizon2d import Horizon2D
```

## Horizon2D class

The Horizon2D class provides access to 2D horizons in an OpendTect project/survey. A Survey object is required for context.

The user must create an **Horizon2D** object to access a specific 2D horizon. There are 2 constructors provided:
-   **Horizon2D( survey:odSurvey, horizon_name:str )** which opens an existing horizon called "horizon_name" if it exists or raises an exception.
-   **Horizon2D.create( survey:odSurvey, horizon_name:str, creategeom:bool, overwrite:bool )** which creates a new 2D horizon called "horizon_name". By default "overwrite" is set to false so if "horizon_name" already exists the statement will fail but if true any existing 2D horizon of the same name will be replaced.

```python
f3demo = Survey('F3_Demo_2020')
```

### Horizon2D.info() function
Returns basic information for a 2D horizon in a Python dictionary

```python
gbase = Horizon2D(f3demo, 'SSIS-Grid-Base')
gbase.info()
```

### Horizon2D.attribnames property
Return a list of the attributes attached to this horizon.

```python
gbase.attribnames
```

### Horizon2D.lineids() function
Returns a list of the integer lineid's for lines with data for this horizon

```python
gbase.lineids()
```

### Horizon2D.linenames() function
Return a list of line names with data for this horizon

```python
gbase.linenames()
```

### Horizon2D.linename(lineid) function
Return the line name for the given lineid

```python
gbase.linename(3)
```

### Horizon2D.getz(lineid), Horizon2D.getxy(lineid) and Horizon2D.get_xarray(lineid) functions

-  getz(): return a Numpy 1D array with the horizon Z values for the given lineid
-  getxy(): return a tuple of Numpy 1D arrays with the X coordinates, Y coordinates and trace numbers of the horizon z values for the given lineid
-  get_xarray(): return an XArray DataArray with the horizon X, Y, trace numbers and Z values for the given lineid

```python
import xarray as xr
hor = gbase.get_xarray(12)
hor.plot(yincrease=False)
```

### Horizon2D.feature() function
Returns a GeoJSON feature collection with the outline of the 2D horizon. This can be used in map displays.

```python
gbase.feature()
```

```python
import folium
survmap = folium.Map(location=[52.3,8.0], tiles="Stamen Terrain", zoom_start = 6, min_lat=-90, max_lat=90, min_lon=-180, max_lon=180, max_bounds=True, maxBoundsViscosity=1)
folium.GeoJson(gbase.feature(), popup=folium.GeoJsonPopup(fields=['name'])).add_to(survmap)
survmap
```

## Static methods
A number of methods are provided to get information either for all or a selected number of 2D horizons in the user provided survey.

```python
hors = Horizon2D.names(f3demo)
hors
```

### Horizon2D.infos() and Horizon2D.infos_dataframe() functions
These return a dictionary and a Pandas DataFrame respectively with basic information for the listed horizons (or all horizons if no list provided) in the given survey.

```python
Horizon2D.infos_dataframe(f3demo)
```

### Horizon2D.features() function
Returns a GeoJSON feature collection for the listed 2D horizons (or all 2D horizons if no list provided) in the given survey. This can be used to create map displays.

```python
Horizon2D.features(f3demo, hors[3:5])
```
