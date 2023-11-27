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

# Fault3D Class - ODBind Python Bindings


**If this notebook is not opened from OpendTect then the following paths should be set for your system and the cell executed.**

```python
#import os
#import sys
#odpy_path = '/home/wayne/Work/WMSeismicSolutions/dGB/Development/Build/bin/od7.0/bin/python'
#sys.path.insert(0,odpy_path)
```

```python
import numpy as np
from matplotlib import pyplot as plt
import xarray as xr
from odbind.survey import Survey
from odbind.fault3d import Fault3D
```

## Fault3D class

The Fault3D class provides access to Fault3D objects in an OpendTect project/survey. A Survey object is required for context.

The API includes:
-  the standard set of static functions for getting information for all Fault3D objects in a survey:
    -  names()
    -  infos()
    -  infos_dataframe()
    -  features()
-  a constructor to create an object for access to a specific fault and standard instance functions to access information:
    -  info()
    -  feature()
-  the fault object behaves like a tuple of fault sticks:
    -  len() returns the number of fault sticks in the fault
    -  list like subcripting (including negative indices) can be used to access individual fault sticks


```python
f3demo = Survey('F3_Demo_2020')
Fault3D.names(f3demo)
```

```python
Fault3D.infos_dataframe(f3demo)
```

```python
fault = Fault3D(f3demo, 'Fault A')
fault.info()
```

```python
fault.feature()
```

```python
len(fault)
```

```python
fault[0]
```
