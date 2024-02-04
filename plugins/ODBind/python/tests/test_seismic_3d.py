import pytest
import json
import numpy as np
import random
import xarray as xr
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
#
# Create some test data
    data, data_info = make_data_volume(survey)
#
# Save the test data to storage
    inlrg = data_info['iline']
    crlrg = data_info['xline']
    zrg = data_info[si['zdomain']]
    zistime = si['zdomain']=='twt'
    with Seismic3D.create(survey, 'pytest', inlrg, crlrg, zrg, data_info['comp'], 'CBVS', zistime, True) as test:
        test.putdata((data, data_info,))
#
# Verify the saved data
    niln = int((inlrg[1]-inlrg[0])/inlrg[2]) + 1
    ncrl = int((crlrg[1]-crlrg[0])/crlrg[2]) + 1
    nz = int((zrg[1]-zrg[0])/zrg[2]) + 1
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
#
# Read as a subvolume
    Seismic3D.use_dataframe = False
    Seismic3D.use_xarray = False
    test_data, test_info = test.getdata(rgs.inlrg, rgs.crlrg, rgs.zrg)
    for key in data_info:
        assert data_info[key] == pytest.approx(test_info[key])
    np.testing.assert_equal(data, test_data)
    Seismic3D.use_dataframe = True
    Seismic3D.use_xarray = True
#
# Convert to/from Xarray Dataset
    xrdata = test.to_xarray(test_data, test_info)
    test_data, test_info = test.from_xarray(xrdata)
    for key in data_info:
        assert data_info[key] == pytest.approx(test_info[key])
    np.testing.assert_equal(data, test_data)
#
# Reading inlines
    inl = random.randint(inlrg[0], inlrg[1])
    xrinl = test.iline[inl]
    xr.testing.assert_allclose(xrinl, xrdata.sel(iline=inl))

    inl0 = random.randint(inlrg[0], inlrg[1])
    inl1 = min(inl0+5*inlrg[2], inlrg[1])
    inl = inl0
    for xrtrc in test.iline[inl0:inl1]:
        xr.testing.assert_allclose(xrtrc, xrdata.sel(iline=inl))
        inl += inlrg[2]
#
# Reading crosslines
    crl = random.randint(crlrg[0], crlrg[1])
    xrcrl = test.xline[crl]
    xr.testing.assert_allclose(xrcrl, xrdata.sel(xline=crl))

    crl0 = random.randint(crlrg[0], crlrg[1])
    crl1 = min(crl0+5*crlrg[2], crlrg[1])
    crl = crl0
    for xrtrc in test.xline[crl0:crl1]:
        xr.testing.assert_allclose(xrtrc, xrdata.sel(xline=crl))
        crl += crlrg[2]
#
# Reading zslices
    zidx = random.randint(test.z_index(zrg[0]), test.z_index(zrg[1]))
    zval = test.z_value(zidx)
    xrzsl = test.zslice[zidx]
    if test.zistime:
        xr.testing.assert_allclose(xrzsl, xrdata.sel(twt=zval, method='nearest'))
    else:
        xr.testing.assert_allclose(xrzsl, xrdata.sel(depth=zval, method='nearest'))

    zsl0 = zidx
    zsl1 = min(zsl0+3, test.z_index(zrg[1]))
    zsl = zsl0
    for xrzsl in test.zslice[zsl0:zsl1]:
        zval = test.z_value(zsl)
        if test.zistime:
            xr.testing.assert_allclose(xrzsl, xrdata.sel(twt=zval, method='nearest'))
        else:
            xr.testing.assert_allclose(xrzsl, xrdata.sel(depth=zval, method='nearest'))
        zsl += 1
#
# Reading single traces
    idx = random.randint(0, niln*ncrl-1)
    xrtrc = test.trace[idx]
    inl, crl = test.bin(idx)
    xr.testing.assert_allclose(xrtrc, xrdata.sel(iline=inl, xline=crl))

    xrtrc = test.trace[inl,crl]
    xr.testing.assert_allclose(xrtrc, xrdata.sel(iline=inl, xline=crl))

    stop = min(idx+10,niln*ncrl-1)
    for itrc, xrtrc in enumerate(test.trace[idx:stop], start=idx):
        inl, crl = test.bin(itrc)
        xr.testing.assert_allclose(xrtrc, xrdata.sel(iline=inl, xline=crl))

    inl0 = random.randint(inlrg[0], inlrg[1])
    crl0 = random.randint(crlrg[0], crlrg[1])
    inl1 = min(inl0+5*inlrg[2], inlrg[1])
    crl1 = min(crl0+5*crlrg[2], crlrg[1])
    inl = inl0
    crl = crl0
    for xrtrc in test.trace[inl0:inl1, crl0:crl1]:
        if crl>crl1:
            crl = crl0
            inl += 1
        xr.testing.assert_allclose(xrtrc, xrdata.sel(iline=inl, xline=crl))
        crl += 1
#
# Write by trace
    idx = random.randint(0, niln*ncrl-1)
    stop = min(idx+10,niln*ncrl-1)
    with Seismic3D.create(survey, 'pytest_create', inlrg, crlrg, zrg, data_info['comp'], 'CBVS', zistime, True) as trctest:
        trctest.trace[:] = test.trace[idx:stop]
    trctest = Seismic3D(survey, 'pytest_create')
    xrtest = trctest.volume[:,:,:]
    xr.testing.assert_allclose(xrdata.sel(iline=xrtest.iline, xline=xrtest.xline), xrtest)

    inl0 = random.randint(inlrg[0], inlrg[1])
    inl1 = min(inl0+5*inlrg[2], inlrg[1])
    crl0 = random.randint(crlrg[0], crlrg[1])
    crl1 = min(crl0+5*crlrg[2], crlrg[1])
    with Seismic3D.create(survey, 'pytest_create', inlrg, crlrg, zrg, data_info['comp'], 'CBVS', zistime, True) as trctest:
        trctest.trace[:] = test.trace[inl0:inl1, crl0:crl1]
    trctest = Seismic3D(survey, 'pytest_create')
    xrtest = trctest.volume[:,:,:]
    xr.testing.assert_allclose(xrdata.sel(iline=xrtest.iline, xline=xrtest.xline), xrtest)
#
# Write by inlines
    inl0 = random.randint(inlrg[0], inlrg[1])
    inl1 = min(inl0+5*inlrg[2], inlrg[1])
    with Seismic3D.create(survey, 'pytest_create', inlrg, crlrg, zrg, data_info['comp'], 'CBVS', zistime, True) as trctest:
        trctest.iline[:] = test.iline[inl0:inl1]
    trctest = Seismic3D(survey, 'pytest_create')
    xrtest = trctest.volume[:,:,:]
    xr.testing.assert_allclose(xrdata.sel(iline=xrtest.iline, xline=xrtest.xline), xrtest)
#
# Cleanup the test data from storage
    Seismic3D.delete(survey, ['pytest', 'pytest_create'])
    assert 'pytest' not in Seismic3D.names(survey)
    assert 'pytest_create' not in Seismic3D.names(survey)

