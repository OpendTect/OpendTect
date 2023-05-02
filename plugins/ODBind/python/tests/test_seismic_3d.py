import pytest
import json
import numpy as np
from odbind.survey import Survey
from odbind.seismic3d import Seismic3D

def test_Seismic3D_class():
    f3demo = Survey("F3_Demo_2020")
    vols = Seismic3D.names(f3demo)
    assert '4 Dip steered median filter' in vols
    vol = Seismic3D(f3demo, '4 Dip steered median filter')
    info =  {
                'name': '4 Dip steered median filter',
                'inl_range': [100, 750, 1],
                'crl_range': [300, 1250, 1],
                'z_range': [0, 1848., 4.],
                'comp_count': 1,
                'storage_dtype': 'Integer`Signed`2`IEEE`Yes',
                'nrsamp': 463,
                'bin_count': 619101,
                'trc_count': 600515,
            }
    assert vol.info() == info
    feature =   {
                    'type': 'FeatureCollection',
                    'features': [{
                                    'type': 'Feature',
                                    'properties': info,
                                    'geometry': {
                                                    'type': 'Polygon',
                                                    'coordinates': [[ ['4.644803', '54.796120'],
                                                                      ['4.643676', '54.942126'],
                                                                      ['5.014355', '54.942512'],
                                                                      ['5.014145', '54.796514'],
                                                                      ['4.644803', '54.796120']
                                                                  ]]
                                                }
                                }]
                }
    assert json.loads(vol.feature()) == feature

    assert vol.trace_index(100,400) == 100
    assert vol.bin(100) == (100,400)
    assert vol.z_index(400.)==100
    assert vol.z_value(100)==pytest.approx(400.)

    trc = vol[100]
    assert trc['mdf2'][0::100]==pytest.approx(np.array([0., -1496., 2160., -3589., 682.], dtype=np.float32))
    assert trc['comp'] == ['mdf2']
    assert trc['inl'] == 100 
    assert trc['crl'] == 400 
    assert trc['x'] == pytest.approx([608334.55])
    assert trc['y'] == pytest.approx([6073626.34])
    assert trc['z'][0::100] == pytest.approx([0., 400., 800., 1200., 1600])
    assert trc['dims'] == ['z']

    trc = vol[100, 400]
    assert trc['mdf2'][0::100]==pytest.approx(np.array([0., -1496., 2160., -3589., 682.], dtype=np.float32))
    assert trc['comp'] == ['mdf2']
    assert trc['inl'] == 100 
    assert trc['crl'] == 400 
    assert trc['x'] == pytest.approx([608334.55])
    assert trc['y'] == pytest.approx([6073626.34])
    assert trc['z'][0::100] == pytest.approx([0., 400., 800., 1200., 1600])
    assert trc['dims'] == ['z']

    trc = vol[400,500:502,200:202]
    assert trc['mdf2'] == pytest.approx(np.array([[-3988., 1677., 5693.],[-3314., 2325., 5492.],[-2817., 2936., 5533.]], dtype=np.float32))
    assert trc['comp'] == ['mdf2']
    assert trc['inl'] == 400 
    assert trc['crl'] == [500, 501, 502] 
    assert trc['x'] == pytest.approx([610624.17, 610649.16, 610674.15])
    assert trc['y'] == pytest.approx([6081193.08, 6081193.78, 6081194.47])
    assert trc['z'] == pytest.approx([800., 804., 808.])
    assert trc['dims'] == ['crl', 'z']
    
    trc = vol[400:402,500,200:202]
    assert trc['mdf2'] == pytest.approx(np.array([[-3988., 1677., 5693.],[-3178., 2019., 5516.],[-2859., 2379., 5150.]], dtype=np.float32))
    assert trc['comp'] == ['mdf2']
    assert trc['inl'] == [400, 401, 402] 
    assert trc['crl'] == 500 
    assert trc['x'] == pytest.approx([610624.17, 610623.47,610622.77])
    assert trc['y'] == pytest.approx([6081193.08, 6081218.07, 6081243.06])
    assert trc['z'] == pytest.approx([800., 804., 808.])
    assert trc['dims'] == ['inl', 'z']

    trc = vol[400:402:1,500:502:1,200:202:2]
    assert trc['mdf2'] == pytest.approx(np.array([[[-3988., 5693.],[-3314., 5492.],[-2817., 5533.]],[[-3178., 5516.],[-2667., 5085.],[-2327.,5086.]],
                                                    [[-2859., 5150.],[-2324., 4862.],[-1888.,4642]]], dtype=np.float32))
    assert trc['comp'] == ['mdf2']
    assert trc['inl'] == [400, 401, 402] 
    assert trc['crl'] == [500, 501, 502] 
    assert trc['x'] == pytest.approx(np.array([[610624.17,610649.16,610674.15],[610623.47,610648.46,610673.45],[610622.77,610647.76, 610672.75]], dtype=np.float32))
    assert trc['y'] == pytest.approx(np.array([[6081193.08,6081193.78,6081194.47],[6081218.0,6081218.77,6081219.46],[6081243.06,6081243.76, 6081244.45]], dtype=np.float32))
    assert trc['z'] == pytest.approx([800., 808.])
    assert trc['dims'] == ['inl', 'crl', 'z']
 
    trc = vol[400:402:2,500:502:2,200:202:2]
    assert trc['mdf2'] == pytest.approx(np.array([[[-3988., 5693.],[-2817., 5533.]],[[-2859., 5150.],[-1888.,4642]]], dtype=np.float32))
    assert trc['comp'] == ['mdf2']
    assert trc['inl'] == [400, 402] 
    assert trc['crl'] == [500, 502] 
    assert trc['x'] == pytest.approx(np.array([[610624.17, 610674.15],[610622.77, 610672.75]], dtype=np.float32))
    assert trc['y'] == pytest.approx(np.array([[6081193.08,  6081194.47],[6081243.06, 6081244.45]],dtype=np.float32))
    assert trc['z'] == pytest.approx([800., 808.])
    assert trc['dims'] == ['inl', 'crl', 'z']

    trc = vol[400:402:1,500:502:1,200]
    assert trc['mdf2'] == pytest.approx(np.array([[-3988.,-3314.,-2817.],[-3178.,-2667.,-2327.],[-2859.,-2324.,-1888.]], dtype=np.float32))
    assert trc['comp'] == ['mdf2']
    assert trc['inl'] == [400, 401, 402] 
    assert trc['crl'] == [500, 501, 502] 
    assert trc['x'] == pytest.approx(np.array([[610624.17,610649.16,610674.15],[610623.47,610648.46,610673.45],[610622.77,610647.76, 610672.75]], dtype=np.float32))
    assert trc['y'] == pytest.approx(np.array([[6081193.08,6081193.78,6081194.47],[6081218.0,6081218.77,6081219.46],[6081243.06,6081243.76, 6081244.45]], dtype=np.float32))
    assert trc['z'] == pytest.approx(800.)
    assert trc['dims'] == ['inl', 'crl']

    trc = vol[400:402:2,500:502:2,200]
    assert trc['mdf2'] == pytest.approx(np.array([[-3988.,-2817.],[-2859.,-1888.]], dtype=np.float32))
    assert trc['comp'] == ['mdf2']
    assert trc['inl'] == [400, 402] 
    assert trc['crl'] == [500, 502] 
    assert trc['x'] == pytest.approx(np.array([[610624.17,610674.15],[610622.77, 610672.75]], dtype=np.float32))
    assert trc['y'] == pytest.approx(np.array([[6081193.08,6081194.47],[6081243.06, 6081244.45]], dtype=np.float32))
    assert trc['z'] == pytest.approx(800.)
    assert trc['dims'] == ['inl', 'crl']

