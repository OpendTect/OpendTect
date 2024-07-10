import pytest
import json
import numpy as np
from odbind.survey import Survey
from odbind.geom2d import Geom2D

@pytest.fixture
def survey(request):
    return Survey(request.config.getoption('--survey'))


def test_Geom2D_class(survey):
    assert survey.has2d == True

    data = {
                'line': 'pytest',
                'trc': np.array([1,2,3],dtype=np.int32),
                'ref': np.array([100.0,200.0,300.0],dtype=np.float32),
                'x': np.array([0.0,100.0,200.0],dtype=np.float64),
                'y': np.array([0.0,0.0,0.0],dtype=np.float64)
            }
    with Geom2D.create(survey,'pytest', True) as test:
        test.putdata(data)

    assert 'pytest' in Geom2D.names(survey)
    info = {
                'name': 'pytest',
                'trc_range': [1,3],
                'sp_range': [100.0, 300.0],
                'line_length': 200.0,
                'average_trace_distance': 100.0
            }
    geom = Geom2D(survey,'pytest')
    assert geom.info() == info

    testdata = geom.getdata()
    np.testing.assert_equal(testdata, data)

    Geom2D.delete(survey,['pytest'])
    assert 'pytest' not in Geom2D.names(survey)
