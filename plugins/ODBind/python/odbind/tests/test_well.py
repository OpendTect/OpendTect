import pytest
import json
import numpy as np
import odbind.pytest_helper as pytest_helper
from odbind.well import Well

def test_Well_class(survey):
    wells = Well.names(survey)
    assert 'F02-1' in wells
    well = Well(survey, 'F03-4')
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
                'ground_elevation': 1e+30,
                'd2tmodel_count': 1
            }
    wellinfo = well.info()
    assert info == pytest_helper.approx(wellinfo, rel=0.001)
    track = well.track()
    assert wellinfo['x'] == pytest.approx(track['x'][0], rel=0.001)
    assert wellinfo['y'] == pytest.approx(track['y'][0], rel=0.001)
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
    assert feature == pytest_helper.approx(json.loads(well.feature()), rel=0.001)
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
    assert pytest_helper.approx(well.log_info(['Density']), rel=0.001) == [{
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
    assert pytest_helper.approx(well.marker_info(['FS6']), rel=0.001) == [{'name': 'FS6', 'color': '#ffaa00', 'dah': 856.0800170898438}]
    assert well.track()['tvdss'][9] == pytest.approx(965.8301, rel=0.01)
    assert well.track()['x'][9] == pytest.approx(623255.70, rel=0.01)
    assert well.track()['y'][9] == pytest.approx(6082596.34, rel=0.01)
    logs, uom = well.logs(['Density','Sonic'], zstep=500.1, upscale=False)
    assert np.allclose(logs['dah'], np.array([0., 500.1, 1000.2, 1500.3]), equal_nan=True)
    assert np.allclose(logs['Density'], np.array([np.nan, 2.1078, 2.2417, 2.003]), equal_nan=True)
    assert np.allclose(logs['Sonic'], np.array([np.nan, np.nan, 126.61453, 167.94113]), equal_nan=True)
    assert uom == {'dah': 'm', 'Density':'g/cc', 'Sonic':'us/ft'}
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

def test_Well_create(survey):
    name = 'pytest_well_create'
    if name in Well.names(survey):
        Well.delete(survey, [name])

    x, y = survey.coords(survey.inlrange[0], survey.crlrange[0])
    kbelev, td = 25.0, 1000.0

    try:
        with Well.create(survey, name, x, y, kbelev, td) as well:
            assert name in Well.names(survey)
            info = well.info()
            assert info['name'] == name
            assert info['x'] == pytest.approx(x)
            assert info['y'] == pytest.approx(y)
            assert info['kb'] == pytest.approx(kbelev)
            assert info['td'] == pytest.approx(td)
            track = well.track()
            assert track['dah'][0] == pytest.approx(0.0)
            assert track['dah'][-1] == pytest.approx(td)
            assert track['tvdss'][0] == pytest.approx(-kbelev)
            assert info['d2tmodel_count'] == 1

            dep = np.array([0.0, 500.0], dtype=np.float32)
            well.put_log('pytest', dep, np.array([1.0, 2.0], dtype=np.float32))
            well.put_markers(['pytest'], np.array([500.0], dtype=np.float32))
            assert 'pytest' in well.log_names
            assert 'pytest' in well.marker_names

        with pytest.raises(TypeError):
            Well.create(survey, name, x, y, kbelev, td)

        with Well.create(survey, name, x+10.0, y+10.0, 50.0, 2000.0, True) as well:
            info = well.info()
            assert info['x'] == pytest.approx(x+10.0)
            assert info['kb'] == pytest.approx(50.0)
            assert info['td'] == pytest.approx(2000.0)
            assert well.track()['dah'][-1] == pytest.approx(2000.0)
    finally:
        if name in Well.names(survey):
            Well.delete(survey, [name])

    assert name not in Well.names(survey)

def test_Well_log(survey):
    well = Well(survey, 'F03-4')
    lognm = 'pytest'
    if lognm in well.log_names:
        well.delete_logs([lognm])
    dep = np.array([10.0, 25.5, 50.25, 100.125], dtype=np.float32)
    logval = np.array([1.5, np.nan, 2.5, 3.5], dtype=np.float32)
    well.put_log(lognm, dep, logval, 'g/cc', None)

    data, uom = well.log(lognm)
    assert list(data.keys()) == ['dah', lognm]
    assert uom == {'dah': 'm', lognm: 'g/cc'}
    assert data['dah'].size == dep.size
    np.testing.assert_allclose(data['dah'], dep, rtol=1e-6)
    np.testing.assert_allclose(data[lognm], logval, rtol=1e-6, equal_nan=True)
    assert np.isnan(data[lognm][1])

    df, _ = well.log_dataframe(lognm)
    np.testing.assert_allclose(df['dah'].to_numpy(), dep, rtol=1e-6)
    np.testing.assert_allclose(df[lognm].to_numpy(), logval, rtol=1e-6, equal_nan=True)

    with pytest.raises(ValueError):
        well.log('No such log')
    data, _ = well.log(lognm)
    assert data[lognm].size == dep.size

    well.delete_logs([lognm])

def test_Well_put_markers(survey):
    well = Well(survey, 'F03-4')
    markernms = ['pytest A', 'pytest B']
    well.delete_markers(markernms)
    nmarkers = len(well.marker_names)
    dah = np.array([1200.5, 300.25], dtype=np.float32)
    well.put_markers(markernms, dah, ['#ff0000', '#00FF00'])
    del well

    # reloaded: stored in depth order between the existing markers
    well = Well(survey, 'F03-4')
    assert len(well.marker_names) == nmarkers+2
    assert well.marker_names.index('pytest B') < well.marker_names.index('pytest A')
    assert pytest_helper.approx(well.marker_info(markernms), rel=0.001) == [
                                    {'name': 'pytest A', 'color': '#ff0000', 'dah': 1200.5},
                                    {'name': 'pytest B', 'color': '#00ff00', 'dah': 300.25}
                                ]

    # overwrite=False and the marker exists: nothing is written, no error
    well.put_markers(['pytest A'], np.array([900.0], dtype=np.float32))
    assert well.isok
    assert well.marker_info(['pytest A'])[0]['dah'] == pytest.approx(1200.5)

    # overwrite=True replaces the marker, without a color it is white
    well.put_markers(['pytest A'], np.array([900.0], dtype=np.float32), overwrite=True)
    del well
    well = Well(survey, 'F03-4')
    assert pytest_helper.approx(well.marker_info(['pytest A']), rel=0.001) == [
                                    {'name': 'pytest A', 'color': '#ffffff', 'dah': 900.0}
                                ]

    with pytest.raises(ValueError):
        well.put_markers(['pytest C'], dah)

    # invalid color format
    with pytest.raises(ValueError):
        well.put_markers(['pytest C'], np.array([300.0], dtype=np.float32), ['not-a-color'])

    assert well.delete_markers(markernms)
    del well
    well = Well(survey, 'F03-4')
    assert not any(nm in well.marker_names for nm in markernms)
    assert len(well.marker_names) == nmarkers

def test_Well_d2t_model(survey):
    well = Well(survey, 'F03-4')
    d2t = well.d2t_model()
    assert list(d2t.keys()) == ['dah', 'twt']
    assert d2t['dah'].size == d2t['twt'].size == 7265
    # starts at the seismic reference datum (KB is 34.1 m above it), twt in milliseconds
    np.testing.assert_allclose(d2t['dah'][:2], [34.1, 502.5], rtol=1e-6)
    np.testing.assert_allclose(d2t['twt'][:2], [0.0, 504.704], rtol=1e-6)
    assert d2t['dah'][-1] == pytest.approx(1858.2)
    assert d2t['twt'][-1] == pytest.approx(1812.268)
    assert np.all(np.diff(d2t['twt']) > 0)

    # the checkshot model ends at the well's TD, the FS6 marker depth is a checkshot at 840 ms
    cs = well.d2t_model(checkshot=True)
    assert cs['dah'].size == cs['twt'].size == 21
    assert cs['dah'][-1] == pytest.approx(well.info()['td'])
    fs6 = well.marker_info(['FS6'])[0]['dah']
    assert cs['twt'][np.argmin(np.abs(cs['dah'] - fs6))] == pytest.approx(840.0)

    df = well.d2t_model_dataframe(checkshot=True)
    assert list(df.columns) == ['dah', 'twt']
    np.testing.assert_allclose(df['twt'].to_numpy(), cs['twt'])

def test_Well_d2t_model_by_name_index(survey):
    well = Well(survey, 'F03-4')
    # F3 wells store one time-depth model and one checkshot model each, the active one at index 0
    assert well.d2t_model_names == ['Integrated Depth/Time Model']
    assert well.active_d2t_model_index == 0
    assert well.checkshot_model_names == ['/d29/surveys/F3_Demo/WellInfo/Checkshot/F03-4_TD.txt']
    assert well.active_checkshot_model_index == 0

    for checkshot, names, active in [(False, well.d2t_model_names, well.active_d2t_model_index),
                                     (True, well.checkshot_model_names, well.active_checkshot_model_index)]:
        default = well.d2t_model(checkshot=checkshot)
        for d2t in [well.d2t_model(name=names[active], checkshot=checkshot),
                    well.d2t_model(index=active, checkshot=checkshot)]:
            np.testing.assert_array_equal(d2t['dah'], default['dah'])
            np.testing.assert_array_equal(d2t['twt'], default['twt'])

        df = well.d2t_model_dataframe(index=active, checkshot=checkshot)
        np.testing.assert_array_equal(df['twt'].to_numpy(), default['twt'])

    assert well.d2t_model(index=0)['dah'].size == 7265
    assert well.d2t_model(index=0, checkshot=True)['dah'].size == 21

    with pytest.raises(ValueError, match='no time-depth model named: no such model'):
        well.d2t_model(name='no such model')
    with pytest.raises(ValueError, match='no checkshot model named: Integrated Depth/Time Model'):
        well.d2t_model(name='Integrated Depth/Time Model', checkshot=True)
    with pytest.raises(ValueError, match='no time-depth model at index: 1'):
        well.d2t_model(index=1)
    with pytest.raises(ValueError, match='no checkshot model at index: 1'):
        well.d2t_model(index=1, checkshot=True)
    with pytest.raises(ValueError):
        well.d2t_model(index=-1)
    with pytest.raises(ValueError):
        well.d2t_model(name=well.d2t_model_names[0], index=0)
