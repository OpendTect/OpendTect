import pytest
import json
import numpy as np
from odbind.survey import Survey
from odbind.seismic3d import Seismic3D

def test_Seismic3D_class():
    f3demo = Survey("F3_Demo_2020")
    vols = Seismic3D.names(f3demo)
    vol = Seismic3D(f3demo, '4 Dip steered median filter')

    trc, info = vol.iline[400]
    assert info['comp'] == ['mdf2']
    assert info['iline'] == 400 
    assert info['xline'] == [300, 1250, 1] 
    assert info['x'][0::400] == pytest.approx([605626.12, 615622.22,625618.32])
    assert info['y'][0::400] == pytest.approx([6081053.39, 6081332.76, 6081612.14])
    assert info['twt'] == [0., 1848., 4.]
    assert info['dims'] == ['xline', 'twt']
    assert trc[0][0::400,100] == pytest.approx([-22., 1090., 601.])

    inl = []
    vals = []
    for iln, info in vol.iline[400:500:20]:
        inl.append(info['iline'])
        vals.append(iln[0][0,100])
    assert inl == [400, 420, 440, 460, 480, 500]
    assert vals == pytest.approx([-22., -396., 1016., 1139., 485., -401.])

