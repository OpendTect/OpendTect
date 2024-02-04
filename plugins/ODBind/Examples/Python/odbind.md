---
jupyter:
  jupytext:
    cell_metadata_filter: -all
    formats: md,ipynb
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

# The ODBind Plugin - An Open Source Python Binding to OpendTect Project Data


OpendTect 7.0.3 includes the intial release of the ODBind plugin which provides a Python API to access OpendTect project data. OpendTect already includes the odpy Python module which provides access to some OpendTect project data. The odpy module uses a command line application to interact with the OpendTect project data. Each data or information request incurs the overhead of starting a command line application with arguments defining the data request and the subsequent writing and reading of the results to and from an ascii data stream. This overhead has limited the practical use of odpy to "small" data types like well data. Access to "heavier" data types such as seismic or 3D horizons is not feasible with odpy. The ODBind plugin is a direct binding to the OpendTect C++ code providing Python code direct access to OpendTect project data. The faster data access provided by ODBind makes it feasible to work with more types of OpendTect data from Python than are currently supported by the odpy module.

ODBind uses the Python ctypes module to bind to the OpendTect C/C++ code so the plugin is not restricted to a specific version of the Python interpreter. While the Python API is incomplete and still maturing it already provides streamlined access to a variety of OpendTect project data. 

## ODBind Features
- Provides direct, fast access to OpendTect project data from Python.
- The plugin is not restricted to a specific version of the Python interpreter.
- Memory for large data like seismic and horizons is allocated and managed for the user by the Python interpreter.
- Data can be accessed in more advanced Python data types such as Pandas DataFrames and Xarray DataArrays and DataSets. 


## Some Examples

```python
import numpy as np
from matplotlib import pyplot as plt
import xarray as xr
from odbind.survey import Survey
from odbind.seismic3d import Seismic3D

f3demo = Survey('F3_Demo_2020')
dsmf = Seismic3D(f3demo, '4 Dip steered median filter')

fig, axs = plt.subplots(figsize=(25,5), ncols=3)
for iln, ax in zip(dsmf.iline[200:400:100],axs):
    xriln = dsmf.as_xarray(*iln)
    xr.plot.imshow(xriln['mdf2'], ax=ax, x='xline', y='twt', yincrease=False, cmap='Greys')
```

```python
from odbind.well import Well
Well.names(f3demo)
```

```python
Well.infos_dataframe(f3demo)
```

```python
import folium
wellmap = folium.Map(location=[54.2,5.0], zoom_start = 8, min_lat=-90, max_lat=90, min_lon=-180, max_lon=180, max_bounds=True, maxBoundsViscosity=1)
folium.GeoJson(Well.features(f3demo), popup=folium.GeoJsonPopup(fields=['name'])).add_to(wellmap)
folium.GeoJson(f3demo.feature(), popup=folium.GeoJsonPopup(fields=['name'])).add_to(wellmap)
wellmap
```

## ODBind Status OpendTect 7.0.3

| OpendTect Project Data | ODBind Python Class | Status |
|:-----------------------|:--------------------|:-------|
| Surveys/Projects       | Survey              | <li>Read Info</li><li>Convert between CRS X/Y and inline/crossline</li>|
| Wells                  | Well                | <li>Read Info</li><li>Read markers, well track and log data</li><li>Create and save new logs</li><li>Delete logs</li> |
| 2D Line Geometry       | Geom2D              | <li>Read Info</li><li>Read line geometry</li><li>Create and save new line geometries</li><li>Delete geometries</li> |
| 2D Seismic Horizons    | Horizon2D           | <li>Read Info</li><li>Read horizon X, Y, Z data</li> |
| 3D Seismic Horizons    | Horizon3D           | <li>Read Info</li><li>Read horizon X, Y, Z data</li><li>Create and save new 3D horizons</li><li>Delete 3D horizons</li> |
| 2D Seismic Data        | Seismic2D           | <li>Read Info</li><li>Read 2D seismic data</li>|
| 3D Seismic Data        | Seismic3D           | <li>Read Info</li><li>Read 3D seismic sub volumes, inline/crossline/z slices or individual traces</li><li>Create and save new 3D seismic data</li><li>Delete 3D seismic data</li> |
| 3D Fault Surface       | Fault3D             | <li>Read Info</li><li>Read 3D fault surface sticks</li> |
| Faultstick Set         | FaultStickSet       | <li>Read Info</li><li>Read fault sticks</li> |


## Python API - Basic Information Functions
All ODBind classes have a set of basic class functions to query OpendTect project data:
- **names** returns a Python list with the names of the related OpendTect objects
- **infos** returns a Python dictionary with basic information on the related OpendTect objects
- **infos_dataframe** returns a Pandas Dataframe with the basic information
- **features** returns a GeoJSON Feature Collection including the basic information and GIS feature summary

For the Survey class these methods optional take an argument with the base data folder location and in the case of the infos, infos_dataframe and features methods an optional list of survey names. In the absence of this list, results for all Surveys in the specified data root folder are returned.

For all the data object classes these methods take a Survey instance for context and in the case of the infos, infos_dataframe and features methods an optional list of object names. In the absence of this list, results for all objects in the specified survey are returned.

The data object classes have additional static methods for managing the underlying OpendTect project data:
- **delete** deletes the named objects from a given Survey.


## Python API - Miscellaneous Functions

The ODBind module includes a few functions for accessing the current user's OpendTect settings:

- **get_user_datadir** returns the OpendTect data root folder set in the current user's OpendTect settings
- **get_user_survey** returns the current OpendTect survey set in the current user's OpendTect settings
