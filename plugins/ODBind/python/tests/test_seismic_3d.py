import pytest
import json
import numpy as np
from odbind.survey import Survey
from odbind.seismic3d import Seismic3D

@pytest.fixture
def survey(request):
    return Survey(request.config.getoption('--survey'))

def make_data_volume(survey):
    si = survey.info()
    surv_inlrg = survey.inlrange
    surv_crlrg = survey.crlrange
    surv_zrg = survey.zrange
    zistime = si['zdomain']=='twt'

    filn = 10 + surv_inlrg[0]
    fcrl = 20 + surv_crlrg[0]
    fz = 10 * surv_zrg[2] + surv_zrg[0]
    niln = 21
    ncrl = 41
    nz = 101
    inlrg = [filn, min(filn+(niln-1)*surv_inlrg[2], surv_inlrg[1]), surv_inlrg[2]]
    crlrg = [fcrl, min(fcrl+(ncrl-1)*surv_crlrg[2], surv_crlrg[1]), surv_crlrg[2]]
    zrg = [fz, min(fz+(nz-1)*surv_zrg[2], surv_zrg[1]), surv_zrg[2]]
    ilines = [*range(inlrg[0],inlrg[1]+inlrg[2],inlrg[2])]
    xlines = [*range(crlrg[0],crlrg[1]+crlrg[2],crlrg[2])]
    data_info = {
                'comp': ['comp1', 'comp2'],
                'iline': inlrg,
                'xline': crlrg,
                si['zdomain'] : zrg,
                'dims': ['iline', 'xline', si['zdomain']]
    }
    data = []
    for idx in range(len(data_info['comp'])):
        trcs = np.ones((niln,ncrl,nz),dtype=np.float32) * np.linspace(zrg[0], zrg[1], nz, True, dtype=np.float32)
        trcs[:,:,1] = np.mgrid[inlrg[0]:inlrg[1]+inlrg[2], crlrg[0]:crlrg[1]+crlrg[2]][0]
        trcs[:,:,2] = np.mgrid[inlrg[0]:inlrg[1]+inlrg[2], crlrg[0]:crlrg[1]+crlrg[2]][1]
        trcs[:,:,0] = idx
        data.append(trcs)

    return (data, data_info)



def test_Seismic3D_class(survey):
    assert survey.has3d == True
    si = survey.info()

    data, data_info = make_data_volume(survey)
    inlrg = data_info['iline']
    crlrg = data_info['xline']
    zrg = data_info[si['zdomain']]
    zistime = si['zdomain']=='twt'
    niln = int((inlrg[1]-inlrg[0])/inlrg[2]) + 1
    ncrl = int((crlrg[1]-crlrg[0])/crlrg[2]) + 1
    nz = int((zrg[1]-zrg[0])/zrg[2]) + 1

    with Seismic3D.create(survey, 'pytest', inlrg, crlrg, zrg, data_info['comp'], 'CBVS', zistime, True) as test:
        test.volume = (data, data_info)

    assert 'pytest' in Seismic3D.names(survey)

    info = {
                'name': 'pytest',
                'inl_range': inlrg,
                'crl_range': crlrg,
                'z_range': zrg,
                'zunit': si['zunit'],
                'comp_count':len(data_info['comp']),
                'storage_dtype': 'Float`Signed`4`IEEE`Yes',
                'nrsamp': nz,
                'bin_count': niln*ncrl,
                'trc_count': niln*ncrl
            }
    test = Seismic3D(survey, 'pytest')
    assert test.info() == info
    assert test.zistime == zistime
    assert test.comp_names == data_info['comp']
    assert test.z_index(zrg[1])==nz-1
    assert test.z_value(nz-1)==pytest.approx(zrg[1])
    assert test.bin(niln*ncrl-1) == (inlrg[1],crlrg[1])
    assert test.trace_index(inlrg[1], crlrg[1]) == niln*ncrl-1
    rgs = test.ranges
    assert rgs.inlrg == inlrg and rgs.crlrg == crlrg and rgs.zrg == zrg

    test_data, test_info = test.volume[:, :, :]
    for key in data_info:
        assert data_info[key] == pytest.approx(test_info[key])

    np.testing.assert_equal(data, test_data)

    Seismic3D.delete(survey, ['pytest'])
    assert 'pytest' not in Seismic3D.names(survey)

