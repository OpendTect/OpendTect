---
jupyter:
  jupytext:
    formats: ipynb,md
    text_representation:
      extension: .md
      format_name: markdown
      format_version: '1.3'
      jupytext_version: 1.14.4
  kernelspec:
    display_name: Python 3 (ipykernel)
    language: python
    name: python3
---

# Horizon3D Class - ODBind Python Bindings

```python
odpy_path = '/home/wayne/Work/WMSeismicSolutions/dGB/Development/Build/bin/od7.0/bin/python'
data_root = '/mnt/Data/seismic/ODData'
import os
import sys
import numpy as np
```

```python
sys.path.insert(0,odpy_path)
os.environ["DTECT_DATA"] = data_root
import odbind as odb
```

The Horizon3D class provides access to 3D horizons in an OpendTect project/survey. A Survey object is required for context.

```python
f3demo = odb.Survey(data_root, 'F3_Demo_2020')
```

```python
mfs4 = odb.Horizon3D(f3demo, 'Demo 1 --> MFS4')
mfs4.info()
```

```python
mfs4.attribnames
```

```python tags=[]
import xarray as xr
hor = mfs4.get_xarray()
xr.plot.pcolormesh(hor, x='x', y='y')
```

## Static methods
A number of methods are provided to get information either for all or a selected number of 3D horizons in the user provided survey.

```python
hors = odb.Horizon3D.names(f3demo)
hors
```

```python tags=[]
odb.Horizon3D.infos_dataframe(f3demo, hors[6:8])
```

```python tags=[]
odb.Horizon3D.features(f3demo, hors[6:8])
```
