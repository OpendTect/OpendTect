import pytest
import odbind as odb

def test_Horizon3D_class():
    f3demo = odb.Survey("F3_Demo_2020")
    hors = odb.Horizon3D.names(f3demo)
    assert 'Demo 4 --> Truncation' in hors
    hor = odb.Horizon3D(f3demo, 'Trim_D0 --> FS4')
    assert hor.info() ==    {
                                'name': 'Trim_D0 --> FS4',
                                'inl_range': [104, 747, 1],
                                'crl_range': [303, 1247, 1],
                                'z_range': [0.574454009532928, 1.126021027565],
                                'attrib_count': 0
                            }
    assert hor.attribnames == []
    assert 'SD_44Hz[-8,24ms]' in odb.Horizon3D(f3demo, 'Demo 1 --> MFS4').attribnames
