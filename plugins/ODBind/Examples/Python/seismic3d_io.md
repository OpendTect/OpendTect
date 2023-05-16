# Seismic3D Class IO Examples- ODBind Python Bindings


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
from odbind.seismic3d import Seismic3D
```

```python
f3demo = Survey('F3_Demo_2020')
vol = Seismic3D(f3demo, '4 Dip steered median filter')
```

## Inline Access Mode

```python
fig, axs = plt.subplots(figsize=(25,5), ncols=3)
for iln, ax in zip(vol.iline[200:220:10],axs):
    xriln = vol.as_xarray(*iln)
    xr.plot.imshow(xriln['mdf2'], ax=ax, x='xline', y='twt', yincrease=False, cmap='Greys')
```
