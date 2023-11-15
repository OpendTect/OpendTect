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

# Geom2D Class - ODBind Python Bindings


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
from odbind.geom2d import Geom2D
```

## Geom2D class

The Geom2D class provides access to 2D seismic line geometry in an OpendTect project/survey. A Survey object is required for context.

The user must create a **Geom2D** object to access a specific 2D seismic line geometry. There are 2 constructors provided:
-   **Geom2D( survey:odSurvey, line_name:str )** which opens an existing 2D line name if it exists or raises an exception.


```python
f3demo = Survey('F3_Demo_2020')
```

### Geom2D.info() function
Returns basic information for a 2D seismic line in a Python dictionary

```python
line = Geom2D(f3demo, 'SSIS-Grid-Dip1')
line.info()
```

### Geom2D.feature function
Return a GeoJSON feature collection with the 2D seismic line location. This can be used in map displays.

```python
feature = line.feature()
feature
```

```python
import folium
datamap = folium.Map(location=[52.3,8.0], tiles="openstreetmap", zoom_start = 6, min_lat=-90, max_lat=90, min_lon=-180, max_lon=180, max_bounds=True, maxBoundsViscosity=1)
folium.GeoJson(feature, popup=folium.GeoJsonPopup(fields=['name'])).add_to(datamap)
datamap
```

### Geom2D.getdata Function

```python
pos = line.getdata()
print(f"{pos['line']} \nTrcs: [{pos['trc'][0]},{pos['trc'][-1]}]")
```

## Static methods
A number of methods are provided to get information either for all or a selected number of 2D seismic line geometries in the user provided survey.

```python
datasets = Geom2D.names(f3demo)
datasets
```

### Geom2D.infos() and Geom2D.infos_dataframe() functions
These return a dictionary and a Pandas DataFrame respectively with basic information for the listed 2D seismic line geometries (or all lines if no list provided) in the given survey.

```python
Geom2D.infos_dataframe(f3demo)
```

### Geom2D.features() function

Returns a GeoJSON feature collection for the listed 2D seismic datasets (or all datasets if no list provided) in the given survey. This can be used to create map displays.

```python
features = Geom2D.features(f3demo,['SSIS-Grid-Strike1', 'SSIS-Grid-Strike2'])
features
```

```python
import folium
datamap = folium.Map(location=[52.3,8.0], tiles="openstreetmap", zoom_start = 6, min_lat=-90, max_lat=90, min_lon=-180, max_lon=180, max_bounds=True, maxBoundsViscosity=1)
folium.GeoJson(features, popup=folium.GeoJsonPopup(fields=['name'])).add_to(datamap)
datamap
```
