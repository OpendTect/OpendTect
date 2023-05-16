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
                'zunit' : 'ms',
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

    assert vol.comp_names == ['mdf2']
    assert vol.bin_count == 619101
    assert vol.trace_count == 600515
    assert vol.trace_index(100,400) == 100
    assert vol.bin(100) == (100,400)
    assert vol.z_index(400.)==100
    assert vol.z_value(100)==pytest.approx(400.)
    rgs = vol.ranges
    assert rgs.inlrg == [100, 750, 1] and rgs.crlrg == [300, 1250, 1] and rgs.zrg == [0, 1848, 4]
    assert vol.zistime == True

    newvol = Seismic3D.create(f3demo,'pytest',[200,300,1],[400,500,1],[200,400,4],['comp1'],'CBVS', True, True)
    info =  {
                'name': 'pytest',
                'inl_range': [200, 300, 1],
                'crl_range': [400, 500, 1],
                'z_range': [200., 400., 4.],
                'zunit' : 'ms',
                'comp_count': 1,
                'storage_dtype': 'Float`Signed`4`IEEE`Yes',
                'nrsamp': 51,
                'bin_count': 10201,
                'trc_count': 10201,
            }
    assert newvol.info() == info

    try:
        newvol2 = Seismic3D.create(f3demo,'pytest',[200,300,1],[400,500,1],[200,400,4],['comp1'],'CBVS', True, False)
        assert False
    except TypeError:
        assert True
        
    Seismic3D.delete(f3demo, ['pytest'])
    assert 'pytest' not in Seismic3D.names(f3demo)

