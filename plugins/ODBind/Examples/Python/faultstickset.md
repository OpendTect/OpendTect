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

# FaultStickSet Class - ODBind Python Bindings


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
from odbind.faultstickset import FaultStickSet
```

## FaultStickSet class

The FaultStickSet class provides access to FaultStickSet objects in an OpendTect project/survey. A Survey object is required for context.

The API includes:
-  the standard set of static functions for getting information for all FaultStickSet objects in a survey:
    -  names()
    -  infos()
    -  infos_dataframe()
    -  features()
-  a constructor to create an object for access to a specific FaultStickSet and standard instance functions to access information:
    -  info()
    -  feature()
-  the FaultStickSet object behaves like a tuple of fault sticks:
    -  len() returns the number of fault sticks in the fault
    -  list like subcripting (including negative indices) can be used to access individual fault sticks


```python
f3demo = Survey('F3_Demo_2020')
FaultStickSet.names(f3demo)
```

```python
FaultStickSet.infos_dataframe(f3demo)
```

```python
faultstickset = FaultStickSet(f3demo, 'SSIS-Grid-Faultsticks')
faultstickset.info()
```

```python
faultstickset.feature()
```

```python
len(faultstickset)
```

```python
faultstickset[0]
```
