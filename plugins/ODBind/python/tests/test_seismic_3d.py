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
                'z_range': [0, 1.848, 0.004],
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
    
    
