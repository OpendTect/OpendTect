import pytest
import json
import numpy as np
from odbind.survey import Survey
from odbind.seismic3d import Seismic3D

def test_Seismic3D_class():
    f3demo = Survey("F3_Demo_2020")
    vols = Seismic3D.names(f3demo)
    vol = Seismic3D(f3demo, '4 Dip steered median filter')

    trc, info = vol.trace[100]
    assert trc[0][0::100]==pytest.approx(np.array([0., -1496., 2160., -3589., 682.], dtype=np.float32))
    assert info['comp'] == ['mdf2']
    assert info['iline'] == 100 
    assert info['xline'] == 400 
    assert info['x'] == pytest.approx([608334.55])
    assert info['y'] == pytest.approx([6073626.34])
    assert info['twt'] == pytest.approx([0.0, 1848.0, 4.0])
    assert info['dims'] == ['twt']

    trc, info = vol.trace[100, 400]
    assert trc[0][0::100]==pytest.approx(np.array([0., -1496., 2160., -3589., 682.], dtype=np.float32))
    assert info['comp'] == ['mdf2']
    assert info['iline'] == 100 
    assert info['xline'] == 400 
    assert info['x'] == pytest.approx([608334.55])
    assert info['y'] == pytest.approx([6073626.34])
    assert info['twt'] == pytest.approx([0.0, 1848.0, 4.0])
    assert info['dims'] == ['twt']

    trc, info = vol.trace[-1]
    assert trc[0][0::100]==pytest.approx(np.array([73., -1021.,   242.,  4072., -2849.], dtype=np.float32))
    assert info['comp'] == ['mdf2']
    assert info['iline'] == 750 
    assert info['xline'] == 1250 
    assert info['x'] == pytest.approx([629122.55])
    assert info['y'] == pytest.approx([6090463.28])
    assert info['twt'] == pytest.approx([0.0, 1848.0, 4.0])
    assert info['dims'] == ['twt']

    inl = []
    crl = []
    for _, info in vol.trace[0:100:10]:
        inl.append(info['iline'])
        crl.append(info['xline'])
    assert inl == [100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100]
    assert crl == [300, 310, 320, 330, 340, 350, 360, 370, 380, 390, 400]

    inl = []
    crl = []
    for _, info in vol.trace[100:120:10,300:320:10]:
        inl.append(info['iline'])
        crl.append(info['xline'])
    assert inl == [100, 100, 100, 110, 110, 110, 120, 120, 120]
    assert crl == [300, 310, 320, 300, 310, 320, 300, 310, 320]

    with Seismic3D.create(f3demo,'pytest',[200,210,1],[400,410,1],[200,400,4],['comp1'],'CBVS', True, True) as newvol:
        newvol.trace[:,:] = vol.trace[200:210,400:410]

    newvol = Seismic3D(f3demo,'pytest')
    info =  {
                'name': 'pytest',
                'inl_range': [200, 210, 1],
                'crl_range': [400, 410, 1],
                'z_range': [200., 400., 4.],
                'zunit' : 'ms',
                'comp_count': 1,
                'storage_dtype': 'Float`Signed`4`IEEE`Yes',
                'nrsamp': 51,
                'bin_count': 121,
                'trc_count': 121,
            }
    assert newvol.info() == info
    trc,_ = newvol.trace[205,405]
    assert trc[0][0::20]==pytest.approx(np.array([-78., 530., -1106.], dtype=np.float32))
    Seismic3D.delete(f3demo, ['pytest'])

    with Seismic3D.create(f3demo,'pytest',[200,200,1],[440,460,1],[200,400,4],['comp1'],'CBVS', True, True) as newvol:
        newvol.trace[:] = vol.trace[40:60]

    newvol = Seismic3D(f3demo,'pytest')
    info =  {
                'name': 'pytest',
                'inl_range': [200, 200, 1],
                'crl_range': [440, 460, 1],
                'z_range': [200., 400., 4.],
                'zunit' : 'ms',
                'comp_count': 1,
                'storage_dtype': 'Float`Signed`4`IEEE`Yes',
                'nrsamp': 51,
                'bin_count': 21,
                'trc_count': 21,
            }
    assert newvol.info() == info
    trc,_ = newvol.trace[200,450]
    assert trc[0][0::20]==pytest.approx(np.array([-7080., -4624., 1178.], dtype=np.float32))
    Seismic3D.delete(f3demo, ['pytest'])

    with Seismic3D.create(f3demo,'pytest',[190,210,1],[450,450,1],[200,400,4],['comp1'],'CBVS', True, True) as newvol:
        newvol.trace[:] = vol.trace[40:60]

    newvol = Seismic3D(f3demo,'pytest')
    info =  {
                'name': 'pytest',
                'inl_range': [190, 210, 1],
                'crl_range': [450, 450, 1],
                'z_range': [200., 400., 4.],
                'zunit' : 'ms',
                'comp_count': 1,
                'storage_dtype': 'Float`Signed`4`IEEE`Yes',
                'nrsamp': 51,
                'bin_count': 21,
                'trc_count': 21,
            }
    assert newvol.info() == info
    trc,_ = newvol.trace[200,450]
    assert trc[0][0::20]==pytest.approx(np.array([-7080., -4624., 1178.], dtype=np.float32))
    Seismic3D.delete(f3demo, ['pytest'])

    with Seismic3D.create(f3demo,'pytest',[190,210,1],[440,460,1],[200,400,4],['comp1'],'CBVS', True, True) as newvol:
        newvol.trace[190,440] = vol.trace[50]
        newvol.trace[200,450] = vol.trace[50]
        newvol.trace[210,460] = vol.trace[50]

    newvol = Seismic3D(f3demo,'pytest')
    trc,_ = newvol.trace[200,450]
    assert trc[0][0::20]==pytest.approx(np.array([-7080., -4624., 1178.], dtype=np.float32))
    Seismic3D.delete(f3demo, ['pytest'])

    with Seismic3D.create(f3demo,'pytest',[190,210,1],[440,460,1],[200,400,4],['comp1'],'CBVS', True, True) as newvol:
        newvol.trace[200,450] = np.arange(0., 100., 4., dtype=np.float32)

    newvol = Seismic3D(f3demo,'pytest')
    trc,_ = newvol.trace[200,450]
    np.testing.assert_equal(trc[0][0::20], np.array([0., 80., np.nan], dtype=np.float32))
    Seismic3D.delete(f3demo, ['pytest'])

    with Seismic3D.create(f3demo,'pytest',[190,210,1],[440,460,1],[200,400,4],['comp1','comp2'],'CBVS', True, True) as newvol:
        newvol.trace[200,450] = (np.arange(0., 1000., 4., dtype=np.float32), np.arange(1000., 0., -4., dtype=np.float32),) 

    newvol = Seismic3D(f3demo,'pytest')
    trc,_ = newvol.trace[200,450]
    assert trc[0][0::20]==pytest.approx(np.array([0., 80., 160.], dtype=np.float32))
    assert trc[1][0::20]==pytest.approx(np.array([1000., 920., 840.], dtype=np.float32))
    Seismic3D.delete(f3demo, ['pytest'])
