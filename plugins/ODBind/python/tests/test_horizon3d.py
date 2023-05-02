import pytest
from odbind.survey import Survey
from odbind.horizon3d import Horizon3D

def test_Horizon3D_class():
    f3demo = Survey("F3_Demo_2020")
    hors = Horizon3D.names(f3demo)
    assert 'Demo 4 --> Truncation' in hors
    hor = Horizon3D(f3demo, 'Trim_D0 --> FS4')

    info = hor.info()
    assert info['name'] == 'Trim_D0 --> FS4'
    assert info['inl_range'] == [104, 747, 1]
    assert info['crl_range'] == [303, 1247, 1]
    assert info['z_range'] == pytest.approx([574.454, 1126.021])
    assert info['attrib_count'] == 0

    assert hor.attribnames == []
    assert 'SD_44Hz[-8,24ms]' in Horizon3D(f3demo, 'Demo 1 --> MFS4').attribnames
