import pytest
import json
import odbind as odb

def test_Survey_class():
    assert 'F3_Demo_2020' in odb.Survey.names()
    f3demo = odb.Survey("F3_Demo_2020")
    info =  {
                'name': "F3_Demo_2020",
                'type': "2D3D",
                'crs': "EPSG:23031",
                'xyunit': 'm',
                'zunit': 'ms',
                'srd': 0
            }
    assert f3demo.info() == info
    assert f3demo.bin(610693.97, 6078694.00) == (300, 500)
    assert f3demo.bincoords(610693.97, 6078694.00) == pytest.approx((300.0, 500.0), rel=0.1)
    assert f3demo.coords(300, 500) == pytest.approx((610693.97, 6078694.00), rel=0.01)
    feature =   {
                    'type': 'FeatureCollection',
                    'features': [{
                                    'type': 'Feature',
                                    'properties': info,
                                    'geometry': {'type': 'Polygon',
                                    'coordinates': [[['4.644803', '54.796120'], ['4.643676', '54.942126'],
                                                    ['5.014355', '54.942512'], ['5.014145', '54.796514'],
                                                    ['4.644803', '54.796120']]]}
                                }]
                }
    assert json.loads(f3demo.feature()) == feature
    assert f3demo.has2d == True
    assert f3demo.has3d == True

