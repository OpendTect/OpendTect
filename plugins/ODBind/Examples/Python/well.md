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

# Well Class - ODBind Python Bindings


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
from odbind.well import Well
```

## Well class

The Well class provides access to wells in an OpendTect project/survey. A Survey object is required for context.

The user must create an **Well** object to access a specific well. There are 2 constructors provided:
-   **Well( survey:odSurvey, well_name:str )** which opens an existing well called "well_name" if it exists or raises an exception.


```python
f3demo = Survey('F3_Demo_2020')
```

### Well.info() function
Returns basic information for a well in a Python dictionary

```python
well = Well(f3demo, 'F03-4')
well.info()
```

### Well.feature function
Return a GeoJSON feature collection with the surface position of the well. This can be used in map displays.

```python
feature = well.feature()
feature
```

```python
import folium
wellmap = folium.Map(location=[52.3,8.0], zoom_start = 6)
folium.GeoJson(feature, popup=folium.GeoJsonPopup(fields=['name'])).add_to(wellmap)
wellmap
```

### Well.lognames property
Return a list of the well log names attached to this well.

```python
well.log_names
```

### Well.log_info and Well.log_info_dataframe functions
These return a dictionary and a Pandas DataFrame respectively with basic information for the listed logs (or all logs if no list provided) in this well.

```python
well.log_info(['Density'])
```

### Well.marker_names property
Return a list of the marker names attached to this well

```python
well.marker_names
```

```python
well.marker_info_dataframe()
```

### Well.track and Well.track_dataframe functions
These return a dictionary and a Pandas DataFrame respectively with the well track for the well.

```python
well.track_dataframe()
```

### Well.logs and Well.logs_dataframe functions
These return a dictionary and a Pandas DataFrame respectively with the well log data. The user can control the logs output, the zstep and how the logs are sampled. Sampling can be either by upscaling/averaging  or linear interpolation.

```python
data, uom = well.logs(['Density','Sonic'], zstep=500.1, upscale=True)
uom
```

```python
df, uoms = well.logs_dataframe(['Density','Sonic','Gamma Ray'], zstep=10, upscale=False)
df
```

### Well.put_log function
Saves a log to the well.

```python
well.put_log('Double Density', data['dah'], data['Density']*2, 'g/cc', 'RHOB', True)
well.log_info(['Double Density'])
```

```python
print( f'Density range: [{data["Density"].min()}, {data["Density"].max()}]')
```

## Static methods
A number of methods are provided to get information either for all or a selected number of wells in the user provided survey.

```python
wells = Well.names(f3demo)
wells
```

### Well.infos() and Well.infos_dataframe() functions
These return a dictionary and a Pandas DataFrame respectively with basic information for the listed wells (or all wells if no list provided) in the given survey.

```python
Well.infos_dataframe(f3demo)
```

### Well.features() function

Returns a GeoJSON feature collection for the listed wells (or all wells if no list provided) in the given survey. This can be used to create map displays.

```python
features = Well.features(f3demo)
features
```

```python
import folium
wellmap = folium.Map(location=[52.3,8.0], tiles="Stamen Terrain", zoom_start = 6, min_lat=-90, max_lat=90, min_lon=-180, max_lon=180, max_bounds=True, maxBoundsViscosity=1)
folium.GeoJson(features, popup=folium.GeoJsonPopup(fields=['name'])).add_to(wellmap)
wellmap
```
