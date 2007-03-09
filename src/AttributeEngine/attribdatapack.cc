/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          January 2007
 RCS:           $Id: attribdatapack.cc,v 1.19 2007-03-09 14:20:25 cvshelene Exp $
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
#include "iopar.h"

#define mStepIntvD( rg ) \
    StepInterval<double>( rg.start, rg.stop, rg.step )

namespace Attrib
{

const char* DataPackCommon::categoryStr( bool vertical )
{
    static BufferString vret( IOPar::compKey(sKey::Attribute,"V") );
    return vertical ? vret.buf() : sKey::Attribute;
}


void DataPackCommon::dumpInfo( IOPar& iop ) const
{
    iop.set( "Source type", sourceType() );
    iop.set( "Attribute.ID", descID().asInt() );
    iop.set( "Vertical", isVertical() );
}


Flat3DDataPack::Flat3DDataPack( DescID did, const DataCubes& dc, int cubeidx )
    : ::FlatDataPack(categoryStr(true))
    , DataPackCommon(did)
    , cube_(dc)
    , arr2dsl_(0)
{
    cube_.ref();
    int unuseddim, dim0, dim1;
    if ( cube_.getInlSz() < 2 )
    {
	dir_ = CubeSampling::Inl;
	unuseddim = DataCubes::cInlDim();
	dim0 = DataCubes::cCrlDim();
	dim1 = DataCubes::cZDim();
    }
    else if ( cube_.getCrlSz() < 2 )
    {
	dir_ = CubeSampling::Crl;
	unuseddim = DataCubes::cCrlDim();
	dim0 = DataCubes::cInlDim();
	dim1 = DataCubes::cZDim();
    }
    else
    {
	dir_ = CubeSampling::Z;
	unuseddim = DataCubes::cZDim();
	dim0 = DataCubes::cInlDim();
	dim1 = DataCubes::cCrlDim();
	setCategory( categoryStr(false) );
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
    posdata_.setRange( true, dir_==CubeSampling::Inl
	    ? mStepIntvD(cs.hrg.crlRange()) : mStepIntvD(cs.hrg.inlRange()) );
    posdata_.setRange( false, dir_==CubeSampling::Z
	    ? mStepIntvD(cs.hrg.crlRange()) : mStepIntvD(cs.zrg) );
}


void Flat3DDataPack::dumpInfo( IOPar& iop ) const
{
    ::FlatDataPack::dumpInfo( iop );
    DataPackCommon::dumpInfo( iop );
}


void Flat3DDataPack::getAuxInfo( int i0, int i1, IOPar& iop ) const
{
    Coord3 c( getCoord(i0,i1) );
    BinID bid = SI().transform( c );
    iop.set( eString(SeisTrcInfo::Fld,SeisTrcInfo::CoordX), c.x );
    iop.set( eString(SeisTrcInfo::Fld,SeisTrcInfo::CoordY), c.y );
    iop.set( "Z", c.z );
}


Coord3 Flat3DDataPack::getCoord( int i0, int i1 ) const
{
    int inlidx = i0; int crlidx = 0; int zidx = i1;
    if ( dir_ == CubeSampling::Inl )
	{ inlidx = 0; crlidx = i0; }
    else if ( dir_ == CubeSampling::Z )
	{ crlidx = i1; zidx = 0; }

    const CubeSampling& cs = cube_.cubeSampling();
    Coord c = SI().transform( cs.hrg.atIndex(inlidx,crlidx) );
    return Coord3(c.x,c.y,cs.zrg.atIndex(zidx));
}


const char* Flat3DDataPack::dimName( bool dim0 ) const
{
    return dim0 ? (dir_==CubeSampling::Inl ? "Crl" : "Inl")
		: (dir_==CubeSampling::Z ? "Crl" : "Z");
}



Flat2DDataPack::Flat2DDataPack( DescID did, const Data2DHolder& dh )
    : ::FlatDataPack(categoryStr(true))
    , DataPackCommon(did)
    , dh_(dh)
    , srctyp_("2D")
{
    dh_.ref();

    array3d_ = new DataHolderArray( dh_.dataset_, false );
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
	pos[idx] = pos[idx-1] + dh_.trcinfoset_[idx-1]->coord.distTo( crd );
	prevcrd = crd;
    }

    const StepInterval<float> zrg = dh_.getCubeSampling().zrg;
    posdata_.setX1Pos( pos, nrpos, 0 );
    posdata_.setRange( false, mStepIntvD(zrg) );
}


void Flat2DDataPack::dumpInfo( IOPar& iop ) const
{
    ::FlatDataPack::dumpInfo( iop );
    DataPackCommon::dumpInfo( iop );
}


void Flat2DDataPack::getAuxInfo( int i0, int i1, IOPar& iop ) const
{
    if ( dh_.trcinfoset_.isEmpty() || i0 < 0 || i0 >= dh_.trcinfoset_.size() )
	return;
    const SeisTrcInfo& ti = *dh_.trcinfoset_[i0];
    ti.getInterestingFlds( Seis::Line, iop );
    iop.set( "Z-Coord", ti.samplePos(i1) );
}


Coord3 Flat2DDataPack::getCoord( int i0, int i1 ) const
{
    if ( dh_.trcinfoset_.isEmpty() ) return Coord3();

    if ( i0 < 0 ) i0 = 0;
    if ( i0 >= dh_.trcinfoset_.size() ) i0 = dh_.trcinfoset_.size() - 1;
    const SeisTrcInfo& ti = *dh_.trcinfoset_[i0];
    return Coord3( ti.coord, ti.sampling.atIndex(i1) );
}


const char* Flat2DDataPack::dimName( bool dim0 ) const
{
    return dim0 ? "Distance" : "Z";
}


CubeDataPack::CubeDataPack( DescID did, const DataCubes& dc, int ci )
    : ::CubeDataPack(categoryStr(false))
    , DataPackCommon(did)
    , cube_(dc)
    , cubeidx_(ci)
{
    cube_.ref();
    cs_ = cube_.cubeSampling();
}


Array3D<float>& CubeDataPack::data()
{
    return const_cast<DataCubes&>(cube_).getCube( cubeidx_ );
}


void CubeDataPack::dumpInfo( IOPar& iop ) const
{
    ::CubeDataPack::dumpInfo( iop );
    DataPackCommon::dumpInfo( iop );
}


void CubeDataPack::getAuxInfo( int, int, int, IOPar& ) const
{
    //TODO implement from trace headers
}


} // namespace Attrib
