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

# Horizon2D Class - ODBind Python Bindings

```python
odpy_path = '/home/wayne/Work/WMSeismicSolutions/dGB/Development/Build/bin/od7.0/bin/python'
data_root = '/mnt/Data/seismic/ODData'
import os
import sys
```

```python
sys.path.insert(0,odpy_path)
os.environ["DTECT_DATA"] = data_root
import odbind as odb
```

The Horizon2D class provides access to 2D horizons in an OpendTect project/survey. A Survey object is required for context.

```python
f3demo = odb.Survey(data_root, 'F3_Demo_2020')
```

```python
gtop = odb.Horizon2D(f3demo, 'SSIS-Grid-Top')
gtop.info()
```

```python
gtop.lineids()
```

```python
gtop.attribnames
```

## Static methods
A number of methods are provided to get information either for all or a selected number of 2D horizons in the user provided survey.

```python
hors = odb.Horizon2D.names(f3demo)
hors
```

```python tags=[]
odb.Horizon2D.infos(f3demo, hors[0:4])
```
