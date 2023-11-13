import pytest
import numpy as np
from odbind.survey import Survey
from odbind.horizon3d import Horizon3D

@pytest.fixture
def survey(request):
    return Survey(request.config.getoption('--survey'))

def test_Horizon3D_class(survey):
    assert survey.has3d == True

    inlrg = survey.inlrange
    crlrg = survey.crlrange
    ilines = range(inlrg[0], inlrg[1]+inlrg[2], inlrg[2])
    xlines = range(crlrg[0], crlrg[1]+crlrg[2], crlrg[2])
    ninl = len(ilines)
    ncrl = len(xlines)
    data = np.ones((ninl, ncrl),dtype=np.float32)
    data[0,0] = 100.0

    with Horizon3D.create(survey, 'pytest', inlrg, crlrg, True) as test:
        test.putz(data, ilines, xlines)

    assert 'pytest' in Horizon3D.names(survey)
    hor = Horizon3D(survey, 'pytest')

    info = {
                'name': 'pytest',
                'inl_range': inlrg,
                'crl_range': crlrg,
                'z_range': [1.0, 100.0],
                'attrib_count': 0
            }
    assert hor.info() == info

    testdata = hor.getz()
    np.testing.assert_equal(testdata, data)

    Horizon3D.delete(survey,['pytest'])
    assert 'pytest' not in Horizon3D.names(survey)
