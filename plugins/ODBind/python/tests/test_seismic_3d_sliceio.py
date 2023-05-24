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
    for ln, info in vol.iline[400:500:20]:
        inl.append(info['iline'])
        vals.append(ln[0][0,100])
    assert inl == [400, 420, 440, 460, 480, 500]
    assert vals == pytest.approx([-22., -396., 1016., 1139., 485., -401.])

    trc, info = vol.xline[400]
    assert info['comp'] == ['mdf2']
    assert info['iline'] == [100, 750, 1] 
    assert info['xline'] == 400 
    assert info['x'][0::400] == pytest.approx([608334.55, 608055.34])
    assert info['y'][0::400] == pytest.approx([6073626.34, 6083622.20])
    assert info['twt'] == [0., 1848., 4.]
    assert info['dims'] == ['iline', 'twt']
    assert trc[0][0::400,100] == pytest.approx([-1496., -1221.])

    xln = []
    vals = []
    for ln, info in vol.xline[400:500:20]:
        xln.append(info['xline'])
        vals.append(ln[0][0,100])
    assert xln == [400, 420, 440, 460, 480, 500]
    assert vals == pytest.approx([-1496., -3357., 76., -1253., 125., -2809.])

    trc, info = vol.zslice[100]
    assert info['comp'] == ['mdf2']
    assert info['iline'] == [100, 750, 1] 
    assert info['xline'] == [300, 1250, 1] 
    assert info['x'][0,0] == pytest.approx(605835.52)
    assert info['y'][0,0] == pytest.approx(6073556.50)
    assert info['x'][325,475] == pytest.approx(617479.04)
    assert info['y'][325,475] == pytest.approx(6082009.89)
    assert info['twt'] == 400.
    assert info['dims'] == ['iline', 'xline']
    assert trc[0][325,475] == pytest.approx(-1485.)

    z = []
    vals = []
    for sl, info in vol.zslice[100:110:5]:
        z.append(info['twt'])
        vals.append(sl[0][325,475])
    assert z == [400., 420., 440.]
    assert vals == pytest.approx([-1485., 259., 3839.])

    with Seismic3D.create(f3demo,'pytest',[200,210,1],[400,410,1],[200,400,4],['comp1'],'CBVS', True, True) as newvol:
        newvol.iline[:] = vol.iline[200:210]

    newvol = Seismic3D(f3demo,'pytest')
    info =  {
                'name': 'pytest',
                'inl_range': [200, 210, 1],
                'crl_range': [400, 410, 1],
                'z_range': [200., 400., 4.],
                'zunit' : 'ms',
                'comp_count': 1,
                'storage_dtype': 'Float`Signed`4`IEEE`Yes',
                'nrsamp': 51,
                'bin_count': 121,
                'trc_count': 121,
            }
    assert newvol.info() == info
    trc,_ = newvol.trace[205,405]
    assert trc[0][0::20]==pytest.approx(np.array([-78., 530., -1106.], dtype=np.float32))
    Seismic3D.delete(f3demo, ['pytest'])

    with Seismic3D.create(f3demo,'pytest',[200,210,1],[400,410,1],[200,400,4],['comp1'],'CBVS', True, True) as newvol:
        newvol.xline[:] = vol.xline[400:410]

    newvol = Seismic3D(f3demo,'pytest')
    info =  {
                'name': 'pytest',
                'inl_range': [200, 210, 1],
                'crl_range': [400, 410, 1],
                'z_range': [200., 400., 4.],
                'zunit' : 'ms',
                'comp_count': 1,
                'storage_dtype': 'Float`Signed`4`IEEE`Yes',
                'nrsamp': 51,
                'bin_count': 121,
                'trc_count': 121,
            }
    assert newvol.info() == info
    trc,_ = newvol.trace[205,405]
    assert trc[0][0::20]==pytest.approx(np.array([-78., 530., -1106.], dtype=np.float32))
    Seismic3D.delete(f3demo, ['pytest'])
