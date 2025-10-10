import pytest
import odbind.pytest_helper as pytest_helper
import json
import numpy as np
from odbind.survey import Survey
from odbind.geom2d import Geom2D

@pytest.fixture
def survey(request):
    return Survey(request.config.getoption('--survey'))

data = {
        'line': 'pytest',
        'trc': np.array([1,2,3],dtype=np.int32),
        'ref': np.array([100.0,200.0,300.0],dtype=np.float32),
        'x': np.array([0.0,100.0,200.0],dtype=np.float64),
        'y': np.array([0.0,0.0,0.0],dtype=np.float64)
        }

def test_Survey_class():
    assert 'F3_Demo_2020' in Survey.names()

    try:
        Survey("bogus")
        assert False
    except Exception:
        assert True

    f3demo = Survey("F3_Demo_2020")
    info =  {
                'name': "F3 Demo 2020",
                'type': "2D3D",
                'crs': "EPSG:23031",
                'zdomain': 'twt',
                'xyunit': 'm',
                'zunit': 'ms',
                'srd': 0,
                'inl_step': 25.0,
                'crl_step': 25.0
            }
    assert info == pytest.approx(f3demo.info(), rel=0.001)
    assert f3demo.bin(610693.97, 6078694.00) == (300, 500)
    assert f3demo.bincoords(610693.97, 6078694.00) == pytest.approx((300.0, 500.0), rel=0.1)
    assert f3demo.coords(300, 500) == pytest.approx((610693.97, 6078694.00), rel=0.01)
    feature =   {
                    'type': 'FeatureCollection',
                    'features': [{
                                    'type': 'Feature',
                                    'properties': info,
                                    'geometry': {'type': 'Polygon',
                                    'coordinates': [[['4.644803', '54.796119'], ['4.643676', '54.942125'],
                                                    ['5.014355', '54.942511'], ['5.014145', '54.796513'],
                                                    ['4.644803', '54.796119']]]}
                                }]
                }
    assert feature == pytest_helper.approx(json.loads(f3demo.feature()),rel=0.001)
    assert f3demo.has2d == True
    assert f3demo.has3d == True
    assert f3demo.zrange == pytest.approx([0, 1848, 4])

def test_Object_Interface(survey):
    assert survey.has2d == True
    with Geom2D.create(survey,'pytest', True) as test:
        test.putdata(data)

    assert 'pytest' in survey.get_object_names('Geometry')
    assert survey.has_object('pytest','Geometry') == True
    assert survey.has_object('notpresent','Geometry') == False
    result = survey.get_object_info('pytest','Geometry')
    assert result['TranslatorGroup'] == 'Geometry'
    assert result == survey.get_object_info('pytest')
    assert result == survey.get_object_info_byid(result['ID'])
    assert result in survey.get_object_infos('Geometry')
    survey.create_object('pytest_create', 'Seismic Data', 'CBVS', True)
    assert survey.has_object('pytest_create', 'Seismic Data') == True
    result = survey.get_object_info('pytest_create')
    assert result == {
                        'ID': result['ID'],
                        'Name': 'pytest_create',
                        'Format': 'CBVS',
                        'TranslatorGroup': 'Seismic Data',
                        'File name': result['File name']
                    }
    survey.remove_object('pytest_create', 'Seismic Data')
    assert survey.has_object('pytest_create') == False

    Geom2D.delete(survey,['pytest'])
