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

# Survey Class - ODBind Python Bindings


If this notebook is not opened from OpendTect then the following paths should be set for your system and the cell executed.

```python
#import os
#import sys
#odpy_path = '/home/wayne/Work/WMSeismicSolutions/dGB/Development/Build/bin/od7.0/bin/python'
#sys.path.insert(0,odpy_path)
```

```python
import odbind as odb
from odbind.survey import Survey
```

```python
odb.get_user_datadir()
```

```python
odb.get_user_survey()
```

```python
help(odb.get_user_datadir)
```

## Survey class
This class encapsulates an OpendTect project/survey. Creating a Survey object requires the project/survey name. Optionally a base data folder location can be included in the function parameters. If a base data folder is not provided the location stored in the users OpendTect settings is used.

Note that **odb.Survey('F3_Demo_2020')** and **odb.Survey('F3_Demo_2020', odb.get_user_datadir())** are equivalent.

The other data specific classes provided by the bindings require a Survey object for context. Various methods/properties are available to get information about the survey.

```python
f3demo = Survey('F3_Demo_2020')
penobscot = Survey('Penobscot')
```

### Survey.info() function
Returns basic information for a survey in a Python dictionary

```python
print(f'F3Demo: {f3demo.info()} \nPenobscot: {penobscot.info()}')
```

### Survey.bin(), Survey.bincoords() and Survey.coords() functions
Convert between inlines/crosslines and x/y coordinates:
-  bin(): convert x/y coordinates to nearest(integer) inline crossline
-  bincoords() : convert x/y coordinates to decimal inline crossline
-  coords(): convert integer inline/crossline to x/y coordinates

```python
f3demo.bincoords(610693.97, 6078694.00)
```

```python
f3demo.coords(300, 500)
```

### Survey.feature() function
Returns a GeoJSON feature collection with the outline of the survey. This can be used in map displays.

```python
f3demo.feature()
```

```python
import folium
survmap = folium.Map(location=[52.3,8.0], zoom_start = 6, min_lat=-90, max_lat=90, min_lon=-180, max_lon=180, max_bounds=True, maxBoundsViscosity=1)
folium.GeoJson(f3demo.feature(), popup=folium.GeoJsonPopup(fields=['name','type'])).add_to(survmap)
survmap
```

## Static methods
A number of methods are provided to get information either for all or a selected number of surveys.

### Survey.names() function
Returns a python list with the names of all OpendTect surveys. Optionally  a base data folder location can be included in the function parameters. If a base data folder is not provided the location stored in the users OpendTect settings is used.

Note that **odb.Survey.names()** and **odb.Survey.names(odb.get_user_datadir())** are equivalent. 

```python
Survey.names(odb.get_user_datadir())
```

### Survey.infos() and Survey.infos_dataframe() functions
These return a dictionary and a Pandas DataFrame respectively with basic information for the listed surveys (or all surveys if no list provided). Optionally a base data folder location can be included in the function parameters. If a base data folder is not provided the location stored in the users OpendTect settings is used.

Note that **odb.Survey.infos()** and **odb.Survey.infos([], odb.get_user_datadir())** are equivalent. 

```python
Survey.infos(['F3_Demo_2020', 'Penobscot'])
```

```python
Survey.infos_dataframe()
```

### Survey.features() function

Returns a GeoJSON feature collection for the listed surveys (or all surveys if no list provided). Optionally a base data folder location can be included in the function parameters. If a base data folder is not provided the location stored in the users OpendTect settings is used.

Note that **odb.Survey.features()** and **odb.Survey.features([], odb.get_user_datadir())** are equivalent.

```python
features = Survey.features(['F3_Demo_2020', 'Penobscot'])
features
```

```python
features_map = folium.Map(location=[52.3,8.0], zoom_start = 3, min_lat=-90, max_lat=90, min_lon=-180, max_lon=180, max_bounds=True, maxBoundsViscosity=1)
folium.GeoJson(features, popup=folium.GeoJsonPopup(fields=['name','type'])).add_to(features_map)
features_map
```

## Object Interface

```python
f3demo.get_object_names('Well')
```

```python
f3demo.has_object('F02-1', 'Geometry')
```

```python
f3demo.has_object('SSIS-Grid-Dip1', 'Geometry')
```

```python
f3demo.has_object('SSIS-Grid-Dip1')
```

```python
f3demo.get_object_info('SSIS-Grid-Dip1', 'Geometry')
```

```python
f3demo.get_object_info('4 Dip steered median filter')
```

```python
f3demo.get_object_info_byid('100100.3')
```

```python
f3demo.get_object_infos('Well')
```

```python
f3demo.create_object('test_object', 'Seismic Data', 'CBVS', True)
f3demo.get_object_info('test_object')
```

```python
f3demo.remove_object('test_object','Seismic Data')
f3demo.has_object('test_object')
```
