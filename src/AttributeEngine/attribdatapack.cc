/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          January 2007
 RCS:           $Id: attribdatapack.cc,v 1.13 2007-02-20 18:15:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "attribdatapack.h"
#include "attribdatacubes.h"
#include "attribdataholder.h"
#include "attribdataholderarray.h"
#include "flatposdata.h"
#include "arraynd.h"
#include "arrayndslice.h"
#include "seisinfo.h"
#include "survinfo.h"
#include "keystrs.h"

#define mStepIntvD( rg ) \
    StepInterval<double>( rg.start, rg.stop, rg.step )

namespace Attrib
{

Flat3DDataPack::Flat3DDataPack( const DataCubes& dc, int cubeidx )
    : FlatDataPack(sKey::Attribute)
    , cube_(dc)
    , arr2dsl_(0)
{
    cube_.ref();
    int unuseddim, dim0, dim1;
    if ( cube_.getInlSz() < 2 )
    {
	unuseddim = DataCubes::cInlDim();
	dim0 = DataCubes::cCrlDim();
	dim1 = DataCubes::cZDim();
    }
    else if ( cube_.getCrlSz() < 2 )
    {
	unuseddim = DataCubes::cCrlDim();
	dim0 = DataCubes::cInlDim();
	dim1 = DataCubes::cZDim();
    }
    else
    {
	unuseddim = DataCubes::cZDim();
	dim0 = DataCubes::cInlDim();
	dim1 = DataCubes::cCrlDim();
    }

    arr2dsl_ = new Array2DSlice<float>( cube_.getCube(cubeidx) );
    arr2dsl_->setPos( unuseddim, 0 );
    arr2dsl_->setDimMap( 0, dim0 );
    arr2dsl_->setDimMap( 1, dim1 );
    arr2dsl_->init();

    setPosData();
}
					

Flat3DDataPack::~Flat3DDataPack()
{
    delete arr2dsl_;
    cube_.unRef();
}


Array2D<float>& Flat3DDataPack::data()
{
    return *arr2dsl_;
}


void Flat3DDataPack::setPosData()
{
    const CubeSampling cs = cube_.cubeSampling();
    CubeSampling::Dir dir = cs.defaultDir();
    posdata_.setRange( true, dir==CubeSampling::Inl
	    ? mStepIntvD(cs.hrg.crlRange()) : mStepIntvD(cs.hrg.inlRange()) );
    posdata_.setRange( false, mStepIntvD(cs.zrg) );
}


void Flat3DDataPack::getAuxInfo( int i0, int i1, IOPar& par ) const
{
    //TODO implement from trace headers
}


Coord3 Flat3DDataPack::getCoord( int i0, int i1 ) const
{
//TODO implementation needs change for horizontal slices
    const CubeSampling& cs = cube_.cubeSampling();
    Coord c = SI().transform( cs.hrg.atIndex(i0,i1) );
    return Coord3(c.x,c.y,0);
}



Flat2DDataPack::Flat2DDataPack( const Data2DHolder& dh )
    : FlatDataPack(sKey::Attribute)
    , dh_(dh)
{
    dh_.ref();

    array3d_ = new DataHolderArray( dh_.dataset_ );
    arr2dsl_ = new Array2DSlice<float>( *array3d_ );
    arr2dsl_->setPos( 0, 0 );
    arr2dsl_->setDimMap( 0, 1 );
    arr2dsl_->setDimMap( 1, 2 );
    arr2dsl_->init();

    setPosData();
}


Flat2DDataPack::~Flat2DDataPack()
{
    delete arr2dsl_;
    delete array3d_;
    dh_.unRef();
}

Array2D<float>& Flat2DDataPack::data()
{
    return *arr2dsl_;
}


void Flat2DDataPack::setPosData()
{
    const int nrpos = dh_.trcinfoset_.size();
    if ( nrpos < 1 ) return;

    float* pos = new float[nrpos]; pos[0] = 0;
    Coord prevcrd = dh_.trcinfoset_[0]->coord;
    for ( int idx=1; idx<nrpos; idx++ )
    {
	Coord crd = dh_.trcinfoset_[idx]->coord;
	pos[idx] = dh_.trcinfoset_[idx-1]->coord.distTo( crd );
	prevcrd = crd;
    }

    const StepInterval<float> zrg = dh_.getCubeSampling().zrg;
    posdata_.setX1Pos( pos, nrpos, 0 );
    posdata_.setRange( false, mStepIntvD(zrg) );
}


void Flat2DDataPack::getAuxInfo( int i0, int i1, IOPar& par ) const
{
    //TODO implement from trace headers
}


Coord3 Flat2DDataPack::getCoord( int i0, int i1 ) const
{
    return Coord3( dh_.trcinfoset_[i0]->coord, 0 );
}


CubeDataPack::CubeDataPack( const Attrib::DataCubes& dc, int ci )
    : ::CubeDataPack(sKey::Attribute)
    , cube_(dc)
    , cubeidx_(ci)
{
    cube_.ref();
    cs_ = cube_.cubeSampling();
}


Array3D<float>& CubeDataPack::data()
{
    return const_cast<Attrib::DataCubes&>(cube_).getCube( cubeidx_ );
}


void CubeDataPack::getAuxInfo( int, int, int, IOPar& ) const
{
    //TODO implement from trace headers
}


} // namespace Attrib
