import pytest
import json
import numpy as np
from odbind.survey import Survey
from odbind.well import Well

def test_Well_class():
    f3demo = Survey("F3_Demo_2020")
    wells = Well.names(f3demo)
    assert 'F02-1' in wells
    well = Well(f3demo, 'F03-4')
    info =  {
                'name': 'F03-4',
                'uwid': '',
                'operator': '',
                'field': '',
                'county': '',
                'state': '',
                'province': '',
                'country': '',
                'welltype': 'Unknown',
                'x': 623255.98,
                'y': 6082586.87,
                'kb': 34.1,
                'td': 2048,
                'replacement_velocity': 2000,
                'ground_elevation': 1e+30
            }
    assert well.info() == info
    feature =   {
                    'type': 'FeatureCollection',
                    'features': [{
                                    'type': 'Feature',
                                    'properties': info,
                                    'geometry': {
                                                    'type': 'Point',
                                                    'coordinates': ['4.919459','54.873253']
                                                }
                                }]
                }
    assert json.loads(well.feature()) == feature
    assert all(item in well.log_names for item in [
                                'Density',
                                'Sonic',
                                'Gamma Ray',
                                'Porosity',
                                'P-Impedance',
                                'P-Impedance_rel',
                                'Vp',
                                'Vp_BLI',
                                'Vs_BLI',
                                'Density_BLI',
                                'Litholog (10=sand 15=silt 20=silty shale 30=shale)'
                            ])
    assert well.log_info(['Density']) == [{
                                            'name': 'Density',
                                            'mnemonic': 'RHOB',
                                            'uom': 'g/cc',
                                            'dah_range': [30.15, 1859.550048828125],
                                            'log_range': [1.926, 2.3105]
                                        }]
    assert well.marker_names == [
                                    'Seasurface',
                                    'MFS11',
                                    'FS11',
                                    'MFS10',
                                    'MFS9',
                                    'MFS8',
                                    'FS8',
                                    'FS7',
                                    'Truncation',
                                    'Top Foresets',
                                    'FS6',
                                    'MFS4',
                                    'FS4',
                                    'FS3',
                                    'FS2',
                                    'MFS2',
                                    'FS1',
                                    'MMU'
                                ]
    assert well.marker_info(['FS6']) == [{'name': 'FS6', 'color': '#ffffff', 'dah': 856.0800170898438}]
    assert well.track()['tvdss'][9] == pytest.approx(965.8301, rel=0.01)
    assert well.track()['x'][9] == pytest.approx(623255.70, rel=0.01)
    assert well.track()['y'][9] == pytest.approx(6082596.34, rel=0.01)
    logs, uom = well.logs(['Density','Sonic'], zstep=500.1, upscale=False)
    assert np.allclose(logs['dah'], np.array([0., 500.1, 1000.2, 1500.3]), equal_nan=True)
    assert np.allclose(logs['Density'], np.array([np.nan, 2.1078, 2.2417, 2.003]), equal_nan=True)
    assert np.allclose(logs['Sonic'], np.array([np.nan, np.nan, 126.61453, 167.94113]), equal_nan=True)
    assert uom == { 'dah':'m', 'Density': 'g/cc', 'Sonic': 'us/ft'}
    logs, uom = well.logs(['Density','Sonic'], zstep=500.1, upscale=True)
    assert np.allclose(logs['dah'], np.array([0., 500.1, 1000.2, 1500.3]), equal_nan=True)
    assert np.allclose(logs['Density'], np.array([2.1116648, 2.1382186, 2.1459596, 2.1021001]), equal_nan=True)
    assert np.allclose(logs['Sonic'], np.array([np.nan, 140.28456, 143.23149, 150.78096]), equal_nan=True)
    
    if 'pytest' in well.log_names:
        well.delete_logs(['pytest'])
    dep = np.array([0.0, 500.0, 1000.0, 1500.0], dtype=np.float32)
    logval = np.array([0.0, np.nan, 2.0, 3.0], dtype=np.float32)
    well.put_log('pytest', dep, logval, 'g/cc', None)
    logs, uom = well.logs(['pytest'], zstep=500.0, upscale=False)
    assert np.allclose(logs['dah'], dep, equal_nan=True)
    assert np.allclose(logs['pytest'], logval, equal_nan=True)

    logval = np.array([0.0, 1.0, 2.0, 3.0], dtype=np.float32)
    well.put_log('pytest', dep, logval, 'g/cc', None, True)
    logs, uom = well.logs(['pytest'], zstep=500.0, upscale=False)
    assert np.allclose(logs['dah'], dep, equal_nan=True)
    assert np.allclose(logs['pytest'], logval, equal_nan=True)

    well.delete_logs(['pytest'])
    assert 'pytest' not in well.log_names
    
    
