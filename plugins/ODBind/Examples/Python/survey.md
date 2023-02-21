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

# Survey Class - ODBind Python Bindings

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

This class encapsulates an OpendTect project/survey. Creating a Survey object requires both the base data folder location and the project/survey name. Various methods/properties are available to get information about the survey.

The other data specific classes provided by the bindings require a Survey object for context. 

```python
f3demo = odb.Survey(data_root, 'F3_Demo_2020')
penobscot = odb.Survey(data_root, 'Penobscot')
```

```python
print(f'F3Demo: {f3demo.info()} \nPenobscot: {penobscot.info()}')
```

```python
f3demo.bincoords(610693.97, 6078694.00)
```

```python
f3demo.coords(300, 500)
```

```python
f3demo.feature()
```

## Static methods
A number of methods are provided to get information either for all or a selected number of surveys in a user provided data root.

```python
odb.Survey.names(data_root)
```

```python tags=[]
odb.Survey.infos(data_root, ['F3_Demo_2020', 'Penobscot'])
```

```python
odb.Survey.features(data_root, ['F3_Demo_2020'])
```
