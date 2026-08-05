import pytest
import odbind.pytest_helper as pytest_helper
import json
import numpy as np
from odbind.survey import Survey
from odbind.geom2d import Geom2D

data = {
        'line': 'pytest',
        'trc': np.array([1,2,3],dtype=np.int32),
        'ref': np.array([100.0,200.0,300.0],dtype=np.float32),
        'x': np.array([0.0,100.0,200.0],dtype=np.float64),
        'y': np.array([0.0,0.0,0.0],dtype=np.float64)
        }

def test_Survey_class(survey, request):
    assert request.config.getoption('--survey') in Survey.names()

    try:
        Survey("bogus")
        assert False
    except Exception:
        assert True

    info = survey.info()
    assert any(info == pytest_helper.approx(si, rel=0.001) for si in Survey.infos())
    assert survey.survey_type == info['type']

    inl0, crl0 = survey.inlrange[0], survey.crlrange[0]
    x, y = survey.coords(inl0, crl0)
    assert survey.bin(x, y) == (inl0, crl0)
    assert survey.bincoords(x, y) == pytest.approx((float(inl0), float(crl0)), rel=0.1)
    assert survey.coords(*survey.bin(x, y)) == pytest.approx((x, y), rel=0.01)

    feature = json.loads(survey.feature())
    assert feature['type'] == 'FeatureCollection'
    assert len(feature['features']) == 1
    assert feature['features'][0]['properties'] == pytest_helper.approx(info, rel=0.001)

    assert isinstance(survey.has2d, bool) and isinstance(survey.has3d, bool)
    assert survey.has2d or survey.has3d

    zrg = survey.zrange
    assert len(zrg) == 3 and zrg[0] <= zrg[1] and zrg[2] > 0

def test_Object_Interface(survey):
    assert survey.has2d == True
    with Geom2D.create(survey,'pytest', True) as test:
        test.putdata(data)

    assert 'pytest' in survey.get_object_names('Geometry')
    assert survey.has_object('pytest','Geometry') == True
    assert survey.has_object('notpresent','Geometry') == False
    result = survey.get_object_info('pytest','Geometry')
    assert result['TranslatorGroup'] == 'Geometry'
    assert result == survey.get_object_info_byid(result['ID'])
    assert result in survey.get_object_infos('Geometry')
    survey.create_object('pytest_create', 'Seismic Data', 'CBVS', True)
    assert survey.has_object('pytest_create', 'Seismic Data') == True
    result = survey.get_object_info('pytest_create', 'Seismic Data')
    assert result == {
                        'ID': result['ID'],
                        'Name': 'pytest_create',
                        'Format': 'CBVS',
                        'TranslatorGroup': 'Seismic Data',
                        'File name': result['File name']
                    }
    survey.remove_object('pytest_create', 'Seismic Data')
    assert survey.has_object('pytest_create', 'Seismic Data') == False

    Geom2D.delete(survey,['pytest'])
