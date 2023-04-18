import pytest
import os
import platform
import json
import odbind as odb

@pytest.fixture
def data_root():
    dtect_data = None
    if platform.system() == 'Windows':
        dtect_data = os.getenv('DTECT_WINDATA')
    else:
        dtect_data = os.getenv('DTECT_DATA')

    if not dtect_data:
        dtect_data = '/mnt/Data/seismic/ODData'

    return dtect_data

def test_Well_class(data_root):
    f3demo = odb.Survey(data_root, "F3_Demo_2020")
    wells = odb.Well.names(f3demo)
    assert 'F02-1' in wells
    well = odb.Well(f3demo, 'F03-4')
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
    assert well.log_names == [
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
                            ]
    assert well.log_info(['Density']) == [{
                                            'name': 'Density',
                                            'mnemonic': 'RHOB',
                                            'uom': 'g/cc',
                                            'dah_range': [30.15, 1859.550048828125],
                                            'log_range': [1.926, 2.3105]
                                        }]
    assert well.marker_names == [
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
