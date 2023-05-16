import pytest
import json
import numpy as np
from odbind.survey import Survey
from odbind.seismic3d import Seismic3D

def test_Seismic3D_class():
    f3demo = Survey("F3_Demo_2020")
    vols = Seismic3D.names(f3demo)
    vol = Seismic3D(f3demo, '4 Dip steered median filter')

    trc, info = vol.volume[400:402,500:500,200:202]
    assert trc[0] == pytest.approx(np.array([[-3988., 1677., 5693.],[-3178., 2019., 5516.],[-2859., 2379., 5150.]], dtype=np.float32))
    assert info['comp'] == ['mdf2']
    assert info['iline'] == [400, 402, 1] 
    assert info['xline'] == 500 
    assert info['x'] == pytest.approx([610624.17, 610623.47,610622.77])
    assert info['y'] == pytest.approx([6081193.08, 6081218.07, 6081243.06])
    assert info['twt'] == pytest.approx([800., 808., 4.])
    assert info['dims'] == ['iline', 'twt']

    trc, info = vol.volume[400:402:1,500:502:1,200:202:2]
    assert trc[0] == pytest.approx(np.array([[[-3988., 5693.],[-3314., 5492.],[-2817., 5533.]],[[-3178., 5516.],[-2667., 5085.],[-2327.,5086.]],
                                             [[-2859., 5150.],[-2324., 4862.],[-1888.,4642]]], dtype=np.float32))
    assert info['comp'] == ['mdf2']
    assert info['iline'] == [400, 402, 1]
    assert info['xline'] == [500, 502, 1]
    assert info['x'] == pytest.approx(np.array([[610624.17,610649.16,610674.15],[610623.47,610648.46,610673.45],[610622.77,610647.76, 610672.75]], dtype=np.float32))
    assert info['y'] == pytest.approx(np.array([[6081193.08,6081193.78,6081194.47],[6081218.0,6081218.77,6081219.46],[6081243.06,6081243.76, 6081244.45]], dtype=np.float32))
    assert info['twt'] == pytest.approx([800., 808., 8.])
    assert info['dims'] == ['iline', 'xline', 'twt']
 
    trc, info = vol.volume[400:402:1,500:502:1,[800.,808.,8.]]
    assert trc[0] == pytest.approx(np.array([[[-3988., 5693.],[-3314., 5492.],[-2817., 5533.]],[[-3178., 5516.],[-2667., 5085.],[-2327.,5086.]],
                                             [[-2859., 5150.],[-2324., 4862.],[-1888.,4642]]], dtype=np.float32))
    assert info['comp'] == ['mdf2']
    assert info['iline'] == [400, 402, 1]
    assert info['xline'] == [500, 502, 1]
    assert info['x'] == pytest.approx(np.array([[610624.17,610649.16,610674.15],[610623.47,610648.46,610673.45],[610622.77,610647.76, 610672.75]], dtype=np.float32))
    assert info['y'] == pytest.approx(np.array([[6081193.08,6081193.78,6081194.47],[6081218.0,6081218.77,6081219.46],[6081243.06,6081243.76, 6081244.45]], dtype=np.float32))
    assert info['twt'] == pytest.approx([800., 808., 8.])
    assert info['dims'] == ['iline', 'xline', 'twt']
 
    trc, info = vol.volume[400:402:2,500:502:2,200:202:2]
    assert trc[0] == pytest.approx(np.array([[[-3988., 5693.],[-2817., 5533.]],[[-2859., 5150.],[-1888.,4642]]], dtype=np.float32))
    assert info['comp'] == ['mdf2']
    assert info['iline'] == [400, 402, 2] 
    assert info['xline'] == [500, 502, 2]
    assert info['x'] == pytest.approx(np.array([[610624.17, 610674.15],[610622.77, 610672.75]], dtype=np.float32))
    assert info['y'] == pytest.approx(np.array([[6081193.08,  6081194.47],[6081243.06, 6081244.45]],dtype=np.float32))
    assert info['twt'] == pytest.approx([800., 808., 8.])
    assert info['dims'] == ['iline', 'xline', 'twt']

    trc, info = vol.volume[400:402:1,500:502:1,200:200]
    assert trc[0] == pytest.approx(np.array([[-3988.,-3314.,-2817.],[-3178.,-2667.,-2327.],[-2859.,-2324.,-1888.]], dtype=np.float32))
    assert info['comp'] == ['mdf2']
    assert info['iline'] == [400, 402, 1] 
    assert info['xline'] == [500, 502, 1] 
    assert info['x'] == pytest.approx(np.array([[610624.17,610649.16,610674.15],[610623.47,610648.46,610673.45],[610622.77,610647.76, 610672.75]], dtype=np.float32))
    assert info['y'] == pytest.approx(np.array([[6081193.08,6081193.78,6081194.47],[6081218.0,6081218.77,6081219.46],[6081243.06,6081243.76, 6081244.45]], dtype=np.float32))
    assert info['twt'] == pytest.approx(800.)
    assert info['dims'] == ['iline', 'xline']

    trc, info = vol.volume[400:402:2,500:502:2,200:200]
    assert trc[0] == pytest.approx(np.array([[-3988.,-2817.],[-2859.,-1888.]], dtype=np.float32))
    assert info['comp'] == ['mdf2']
    assert info['iline'] == [400, 402, 2] 
    assert info['xline'] == [500, 502, 2] 
    assert info['x'] == pytest.approx(np.array([[610624.17,610674.15],[610622.77, 610672.75]], dtype=np.float32))
    assert info['y'] == pytest.approx(np.array([[6081193.08,6081194.47],[6081243.06, 6081244.45]], dtype=np.float32))
    assert info['twt'] == pytest.approx(800.)
    assert info['dims'] == ['iline', 'xline']

    with Seismic3D.create(f3demo,'pytest',[200,210,1],[400,410,1],[200,400,4],['comp1'],'CBVS', True, True) as newvol:
        newvol.volume = vol.volume[190:210,400:410,:]

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
        newvol.volume = vol.volume[190:210,400:410,[248,348,4]]

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
    np.testing.assert_equal(trc[0][0::20],np.array([np.nan, 530., np.nan], dtype=np.float32))
    Seismic3D.delete(f3demo, ['pytest'])
