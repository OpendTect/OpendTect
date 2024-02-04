import pytest
import json
import numpy as np
from odbind.survey import Survey
from odbind.seismic2d import Seismic2D
from odbind.geom2d import Geom2D

@pytest.fixture
def survey(request):
    return Survey(request.config.getoption('--survey'))

def make_data(survey):
    si = survey.info()
    surv_inlrg = survey.inlrange
    surv_crlrg = survey.crlrange
    surv_zrg = survey.zrange
    zistime = si['zdomain']=='twt'

    ntrcs = 1000
    nz = 101
    fz = 10 * surv_zrg[2] + surv_zrg[0]
    zrg = [fz, min(fz+(nz-1)*surv_zrg[2], surv_zrg[1]), surv_zrg[2]]
    xst, yst = survey.coords(surv_inlrg[0], surv_crlrg[0])
    xend, yend = survey.coords(surv_inlrg[1], surv_crlrg[1])
    data_info= {
                'comp': ['comp1','comp2'],
                'line': 'pytest2d',
                'trc': np.linspace(1, ntrcs, ntrcs, True, dtype=np.int32),
                'ref': np.linspace(1*2, ntrcs*2, ntrcs, True, dtype=np.float32),
                'x': np.linspace(xst, xend, ntrcs, True, dtype=np.float64),
                'y': np.linspace(yst, yend, ntrcs, True, dtype=np.float64),
                si['zdomain']: zrg,
                'dims': ['trc', si['zdomain']]
    }
    data = []
    for idx in range(len(data_info['comp'])):
        trcs = np.ones((ntrcs,nz),dtype=np.float32) * np.linspace(zrg[0], zrg[1], nz, True, dtype=np.float32)
        trcs[:,1] = data_info['trc']
        trcs[:,0] = idx
        data.append(trcs)

    return (data, data_info)


def test_Seismic2D_class(survey):
    assert survey.has2d == True
    si = survey.info()
    zistime = si['zdomain']=='twt'
#
# Create some test data
    data, data_info = make_data(survey)
#
# Save the test data to storage
    linenm = data_info['line']
    with Seismic2D.create(survey, 'pytest', data_info['comp'], 'CBVS', zistime, True) as test:
        test.putdata(linenm, (data, data_info,), True, True) 

#
# Verify the saved data
    Seismic2D.use_dataframe = False
    Seismic2D.use_xarray = False
    assert 'pytest' in Seismic2D.names(survey)
    info = {
                'name': 'pytest',
                'line_count': 1,
                'zunit': si['zunit'],
                'comp_count': len(data_info['comp']),
                'storage_dtype': 'Float`Signed`4`IEEE`Yes'
    }
    test = Seismic2D(survey, 'pytest')
    assert test.info() == info
    assert test.zistime == zistime
    assert test.comp_names == data_info['comp']
    assert test.line_names == [linenm]
    line_info = [{  'name': linenm, 
                    'trc_range': [data_info['trc'][0], data_info['trc'][-1], data_info['trc'][1]-data_info['trc'][0]],
                    'z_range': data_info[si['zdomain']]
    }]
    assert test.line_info(test.line_names) == line_info
    test_data, test_info = test.getdata(linenm)
    for key in data_info:
        assert data_info[key] == pytest.approx(test_info[key])
    np.testing.assert_equal(data, test_data)
    Seismic2D.use_dataframe = True
    Seismic2D.use_xarray = True
#
# Convert to/from Xarray Dataset
    xrdata = test.to_xarray(test_data, test_info)
    test_data, test_info = test.from_xarray(xrdata)
    for key in data_info:
        assert data_info[key] == pytest.approx(test_info[key])
    np.testing.assert_equal(data, test_data)
#
# Save and restore as Xarray Dataset
    with Seismic2D.create(survey, 'pytest', data_info['comp'], 'CBVS', zistime, True) as test:
        test.putdata(linenm, xrdata, True, True) 

    test = Seismic2D(survey, 'pytest')
    xrdata = test.getdata(linenm)
    test_data, test_info = test.from_xarray(xrdata)
    for key in data_info:
        assert data_info[key] == pytest.approx(test_info[key])
    np.testing.assert_equal(data, test_data)
#
# Cleanup the test data from storage
    test.delete_lines([data_info['line']])
    assert data_info['line'] not in test.line_names
    Seismic2D.delete(survey, ['pytest'])
    assert 'pytest' not in Seismic2D.names(survey)    
    Geom2D.delete(survey,[data_info['line']])
    assert data_info['line'] not in Geom2D.names(survey)



