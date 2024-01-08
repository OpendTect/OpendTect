import pytest
import numpy as np
from odbind.survey import Survey
from odbind.horizon3d import Horizon3D

@pytest.fixture
def survey(request):
    return Survey(request.config.getoption('--survey'))

def make_horizon3d(survey):
    si = survey.info()
    surv_inlrg = survey.inlrange
    surv_crlrg = survey.crlrange
    surv_zrg = survey.zrange

    filn = 10 + surv_inlrg[0]
    fcrl = 20 + surv_crlrg[0]
    niln = 21
    ncrl = 41
    inlrg = [filn, min(filn+(niln-1)*surv_inlrg[2], surv_inlrg[1]), surv_inlrg[2]]
    crlrg = [fcrl, min(fcrl+(ncrl-1)*surv_crlrg[2], surv_crlrg[1]), surv_crlrg[2]]
    ilines = [*range(inlrg[0],inlrg[1]+inlrg[2],inlrg[2])]
    xlines = [*range(crlrg[0],crlrg[1]+crlrg[2],crlrg[2])]
    data_info = {
                'comp': ['z', 'attrib1'],
                'iline': inlrg,
                'xline': crlrg,
    }
    data = []
    for idx in range(len(data_info['comp'])):
        surf = np.ones((niln,ncrl),dtype=np.float32)
        surf[:,:] = np.mgrid[inlrg[0]:inlrg[1]+inlrg[2], crlrg[0]:crlrg[1]+crlrg[2]][0] + np.mgrid[inlrg[0]:inlrg[1]+inlrg[2], crlrg[0]:crlrg[1]+crlrg[2]][1]
        data.append(surf*(idx+1))

    return (data, data_info)

def test_Horizon3D_class(survey):
    assert survey.has3d == True
    si = survey.info()

#
# Create some test data
    data, data_info = make_horizon3d(survey)
#
# Save the test data to storage
    inlrg = data_info['iline']
    crlrg = data_info['xline']
    with Horizon3D.create(survey, 'pytest', inlrg, crlrg, True) as test:
        test.putdata((data, data_info,))
#
# Verify the saved data
    assert 'pytest' in Horizon3D.names(survey)
    info = {
                'name': 'pytest',
                'inl_range': inlrg,
                'crl_range': crlrg,
                'z_range': [np.min(data[0]), np.max(data[0])],
                'zunit': si['zunit'],
                'attrib_count': len(data_info['comp'])-1
            }
    test = Horizon3D(survey, 'pytest')
    assert test.info() == info
    assert test.attribnames == data_info['comp'][1:]
    assert test.ranges == (data_info['iline'], data_info['xline'],)
#
# Read and verify horizon z and data
    Horizon3D.use_xarray = False
    test_data, test_info = test.getdata(data_info['comp'][1:])
    for key in data_info:
        assert data_info[key] == pytest.approx(test_info[key])
    np.testing.assert_allclose(data, test_data, atol=1e-3)
    Horizon3D.use_xarray = True
#
# Convert to/from Xarray Dataset
    xrdata = test.to_xarray(test_data, test_info)
    test_data, test_info = test.from_xarray(xrdata)
    for key in data_info:
        assert data_info[key] == test_info[key]
    np.testing.assert_allclose(data, test_data, atol=1e-3)
#
# Cleanup the test data from storage
    Horizon3D.delete(survey,['pytest'])
    assert 'pytest' not in Horizon3D.names(survey)
