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

```python
import os
import sys
```

If this notebook is not opened from OpendTect then the following paths should be set for your system and the cell executed.

```python
odpy_path = '/home/wayne/Work/WMSeismicSolutions/dGB/Development/Build/bin/od7.0/bin/python'
data_root = '/mnt/Data/seismic/ODData'
sys.path.insert(0,odpy_path)
os.environ["DTECT_DATA"] = data_root
```

```python
import odbind as odb
```

## Survey class
This class encapsulates an OpendTect project/survey. Creating a Survey object requires both the base data folder location and the project/survey name. The other data specific classes provided by the bindings require a Survey object for context.

Various methods/properties are available to get information about the survey.

```python
f3demo = odb.Survey(data_root, 'F3_Demo_2020')
penobscot = odb.Survey(data_root, 'Penobscot')
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
survmap = folium.Map(location=[52.3,8.0], tiles="Stamen Terrain", zoom_start = 6, min_lat=-90, max_lat=90, min_lon=-180, max_lon=180, max_bounds=True, maxBoundsViscosity=1)
folium.GeoJson(f3demo.feature(), popup=folium.GeoJsonPopup(fields=['name','type'])).add_to(survmap)
survmap
```

## Static methods
A number of methods are provided to get information either for all or a selected number of surveys in a user provided data root.

### Survey.names() function
Returns a python list with the names of all OpendTect surveys in the user supplied base data folder.

```python
odb.Survey.names(data_root)
```

### Survey.infos() and Survey.infos_dataframe() functions
These return a dictionary and a Pandas DataFrame respectively with basic information for the listed surveys (or all surveys if no list provided) in the user supplied base data folder.

```python tags=[]
odb.Survey.infos(data_root, ['F3_Demo_2020', 'Penobscot'])
```

```python
odb.Survey.infos_dataframe(data_root)
```

### Survey.features() function

Returns a GeoJSON feature collection for the listed surveys (or all surveys if no list provided) in the user supplied base data folder. This can be used to create map displays.

```python
features = odb.Survey.features(data_root, ['F3_Demo_2020', 'Penobscot'])
features
```

```python
features_map = folium.Map(location=[52.3,8.0], tiles="Stamen Terrain", zoom_start = 3, min_lat=-90, max_lat=90, min_lon=-180, max_lon=180, max_bounds=True, maxBoundsViscosity=1)
folium.GeoJson(features, popup=folium.GeoJsonPopup(fields=['name','type'])).add_to(features_map)
features_map
```
