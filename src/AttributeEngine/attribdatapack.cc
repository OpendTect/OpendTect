/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          January 2007
 RCS:           $Id: attribdatapack.cc,v 1.24 2007-12-13 09:07:04 cvshelene Exp $
________________________________________________________________________

-*/

#include "attribdatapack.h"

#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "attribdatacubes.h"
#include "attribdataholder.h"
#include "attribdataholderarray.h"
#include "bufstringset.h"
#include "flatposdata.h"
#include "iopar.h"
#include "keystrs.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "survinfo.h"

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
    , usemultcubes_( cubeidx == -1 )
{
    cube_.ref();
    if ( usemultcubes_ )
	createA2DSFromMultCubes();
    else
	createA2DSFromSingCube( cubeidx );
    
    arr2dsl_->init();

    setPosData();
}


Flat3DDataPack::~Flat3DDataPack()
{
    delete arr2dsl_;
    cube_.unRef();
}


void Flat3DDataPack::createA2DSFromSingCube( int cubeidx )
{
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
}
	

//TODO addapt for usesingtrc_
void Flat3DDataPack::createA2DSFromMultCubes()
{
    pErrMsg( "Not impl" );
}


Array2D<float>& Flat3DDataPack::data()
{
    return *arr2dsl_;
}


//TODO addapt for usesingtrc_
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


//TODO addapt for usesingtrc_
Coord3 Flat3DDataPack::getCoord( int i0, int i1 ) const
{
    int inlidx = i0; int crlidx = 0; int zidx = i1;
    if ( dir_ == CubeSampling::Inl )
	{ inlidx = 0; crlidx = i0; }
    else if ( dir_ == CubeSampling::Z )
	{ crlidx = i1; zidx = 0; }

    const CubeSampling& cs = cube_.cubeSampling();
    const Coord c = SI().transform( cs.hrg.atIndex(inlidx,crlidx) );
    return Coord3(c.x,c.y,cs.zrg.atIndex(zidx));
}


#define mKeyInl eString(SeisTrcInfo::Fld,SeisTrcInfo::BinIDInl)
#define mKeyCrl eString(SeisTrcInfo::Fld,SeisTrcInfo::BinIDCrl)
#define mKeyX eString(SeisTrcInfo::Fld,SeisTrcInfo::CoordX)
#define mKeyY eString(SeisTrcInfo::Fld,SeisTrcInfo::CoordY)

//TODO addapt for usesingtrc_
const char* Flat3DDataPack::dimName( bool dim0 ) const
{
    return dim0 ? (dir_==CubeSampling::Inl ? mKeyCrl : mKeyInl)
		: (dir_==CubeSampling::Z ? mKeyCrl : "Z");
}


//TODO addapt for usesingtrc_
void Flat3DDataPack::getAltDim0Keys( BufferStringSet& bss ) const
{
    if ( dir_== CubeSampling::Crl )
	bss.add( mKeyInl );
    else
	bss.add( mKeyCrl );
    bss.add( mKeyX );
    bss.add( mKeyY );
}


//TODO addapt for usesingtrc_
double Flat3DDataPack::getAltDim0Value( int ikey, int i0 ) const
{
    if ( ikey < 1 || ikey > 3 )
	 return FlatDataPack::getAltDim0Value( ikey, i0 );

    const Coord3 c( getCoord(i0,0) );
    return ikey == 1 ? c.x : c.y;
}


//TODO addapt for usesingtrc_
void Flat3DDataPack::getAuxInfo( int i0, int i1, IOPar& iop ) const
{
    const Coord3 c( getCoord(i0,i1) );
    iop.set( mKeyX, c.x );
    iop.set( mKeyY, c.y );
    iop.set( "Z", c.z );
}


Flat2DDataPack::Flat2DDataPack( DescID did )
    : ::FlatDataPack(categoryStr(true))
    , DataPackCommon(did)
{
    SeisTrcInfo::getAxisCandidates( Seis::Line, tiflds_ );
}


void Flat2DDataPack::dumpInfo( IOPar& iop ) const
{
    ::FlatDataPack::dumpInfo( iop );
    DataPackCommon::dumpInfo( iop );
}


void Flat2DDataPack::getAltDim0Keys( BufferStringSet& bss ) const
{
    for ( int idx=0; idx<tiflds_.size(); idx++ )
	bss.add( eString(SeisTrcInfo::Fld,tiflds_[idx]) );
}


const char* Flat2DDataPack::dimName( bool dim0 ) const
{
    return dim0 ? "Distance" : "Z";
}


Flat2DDHDataPack::Flat2DDHDataPack( DescID did, const Data2DHolder& dh,
       				    bool usesingtrc )
    : Flat2DDataPack( did )
    , dh_( dh )
    , usesingtrc_( usesingtrc )
{
    dh_.ref();

    array3d_ = new DataHolderArray( dh_.dataset_, false );
    arr2dsl_ = new Array2DSlice<float>( *array3d_ );

    arr2dsl_->setPos( 0, usesingtrc ? 1 : 0 );
    arr2dsl_->setDimMap( 0, usesingtrc ? 0 : 1 );
    arr2dsl_->setDimMap( 1, 2 );
    arr2dsl_->init();

    setPosData();
}


Flat2DDHDataPack::~Flat2DDHDataPack()
{
    delete arr2dsl_;
    delete array3d_;
    dh_.unRef();
}

Array2D<float>& Flat2DDHDataPack::data()
{
    return *arr2dsl_;
}


void Flat2DDHDataPack::setPosData()
{
    const int nrpos = usesingtrc_ ? array3d_->info().getSize(0)
				  : dh_.trcinfoset_.size();
    if ( nrpos < 1 ) return;

    if ( usesingtrc_ )
	posdata_.setRange( true, StepInterval<double>( 0, nrpos-1, 1 ) );
    else
    {
	float* pos = new float[nrpos];
	pos[0] = 0;
	Coord prevcrd = dh_.trcinfoset_[0]->coord;
	for ( int idx=1; idx<nrpos; idx++ )
	{
	    Coord crd = dh_.trcinfoset_[idx]->coord;
	    pos[idx] = pos[idx-1] + dh_.trcinfoset_[idx-1]->coord.distTo( crd );
	    prevcrd = crd;
	}
	posdata_.setX1Pos( pos, nrpos, 0 );
    }

    const StepInterval<float> zrg = dh_.getCubeSampling().zrg;
    posdata_.setRange( false, mStepIntvD(zrg) );
}


//TODO addapt for usesingtrc_
double Flat2DDHDataPack::getAltDim0Value( int ikey, int i0 ) const
{
    return i0 < 0 || i0 >= dh_.trcinfoset_.size()
	|| ikey < 0 || ikey >= tiflds_.size()
	 ? FlatDataPack::getAltDim0Value( ikey, i0 )
	 : dh_.trcinfoset_[i0]->getValue( tiflds_[ikey] );
}


//TODO addapt for usesingtrc_
void Flat2DDHDataPack::getAuxInfo( int i0, int i1, IOPar& iop ) const
{
    if ( i0 < 0 || i0 >= dh_.trcinfoset_.size() )
	return;
    const SeisTrcInfo& ti = *dh_.trcinfoset_[i0];
    ti.getInterestingFlds( Seis::Line, iop );
    iop.set( "Z-Coord", ti.samplePos(i1) );
}


//TODO addapt for usesingtrc_
Coord3 Flat2DDHDataPack::getCoord( int i0, int i1 ) const
{
    if ( dh_.trcinfoset_.isEmpty() ) return Coord3();

    if ( i0 < 0 ) i0 = 0;
    if ( i0 >= dh_.trcinfoset_.size() ) i0 = dh_.trcinfoset_.size() - 1;
    const SeisTrcInfo& ti = *dh_.trcinfoset_[i0];
    return Coord3( ti.coord, ti.sampling.atIndex(i1) );
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


CubeDataPack::~CubeDataPack()
{
    cube_.unRef();
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


FlatRdmTrcsDataPack::FlatRdmTrcsDataPack( DescID did, const SeisTrcBuf& sb,
       					  TypeSet<BinID>* path )
    : Flat2DDataPack(did)
{
    seisbuf_ = new SeisTrcBuf( true );
    sb.copyInto(*seisbuf_);

    int nrtrcs = seisbuf_->size();
    int nrsamp = nrtrcs ? seisbuf_->get(0)->size() : 0;
    arr2d_ = new Array2DImpl<float>( nrtrcs, nrsamp );
    fill2DArray( path );
    setPosData( path );
}


FlatRdmTrcsDataPack::~FlatRdmTrcsDataPack()
{
    delete arr2d_;
    if ( seisbuf_ ) seisbuf_->erase();
}


void FlatRdmTrcsDataPack::setPosData( TypeSet<BinID>* path )
{
    const int nrpos = seisbuf_->size();
    if ( nrpos < 1 || ( path && path->size() < nrpos ) ) return;

    float* pos = new float[nrpos]; pos[0] = 0;
    Coord prevcrd;
    int loopmaxidx = path ? path->size() : nrpos;
    int x0arridx = -1;
    for ( int idx=0; idx<loopmaxidx; idx++ )
    {
	int trcidx = path ? seisbuf_->find( (*path)[idx] ) : idx;
	if ( trcidx < 0 ) continue;

	x0arridx++;	
	Coord crd = seisbuf_->get(trcidx)->info().coord;
	if ( x0arridx > 0 )
	{
	    float distnnm1 = prevcrd.distTo(crd);
	    pos[x0arridx] = pos[x0arridx-1] + fabs( distnnm1 );
	}
	prevcrd = crd;
    }

    int nrsamp = seisbuf_->get(0)->size();
    const StepInterval<float> zrg = 
			seisbuf_->get(0)->info().sampling.interval( nrsamp );
    posdata_.setX1Pos( pos, nrpos, 0 );
    posdata_.setRange( false, mStepIntvD(zrg) );
}
    

double FlatRdmTrcsDataPack::getAltDim0Value( int ikey, int i0 ) const
{
    return i0<0 || i0>=seisbuf_->size() || ikey<0 || ikey>=tiflds_.size()
	 ? FlatDataPack::getAltDim0Value( ikey, i0 )
	 : seisbuf_->get(i0)->info().getValue( tiflds_[ikey] );
}


void FlatRdmTrcsDataPack::getAuxInfo( int i0, int i1, IOPar& iop ) const
{
    if ( i0 < 0 || i0 >= seisbuf_->size() )
	return;
    const SeisTrcInfo& ti = seisbuf_->get(i0)->info();
    ti.getInterestingFlds( Seis::Line, iop );
    iop.set( "Z-Coord", ti.samplePos(i1) );
}


Coord3 FlatRdmTrcsDataPack::getCoord( int i0, int i1 ) const
{
    if ( seisbuf_->isEmpty() ) return Coord3();

    if ( i0 < 0 ) i0 = 0;
    if ( i0 >= seisbuf_->size() ) i0 = seisbuf_->size() - 1;
    const SeisTrcInfo& ti = seisbuf_->get(i0)->info();
    return Coord3( ti.coord, ti.sampling.atIndex(i1) );
}


void FlatRdmTrcsDataPack::fill2DArray( TypeSet<BinID>* path )
{
    if ( seisbuf_->isEmpty() || !arr2d_ ) return;
    
    int nrtrcs = seisbuf_->size();
    if ( path && path->size() < nrtrcs ) return;
    
    int nrsamp = seisbuf_->get(0)->size();
    int loopmaxidx = path ? path->size() : nrtrcs;
    int x0arridx = -1;
    
    for ( int idt=0; idt<loopmaxidx; idt++ )
    {
	int trcidx = path ? seisbuf_->find( (*path)[idt] ) : idt;
	if ( trcidx < 0 ) continue;
	
	x0arridx++;
	SeisTrc* trc = seisbuf_->get( trcidx );
	for ( int idz=0; idz<nrsamp; idz++ )
	    arr2d_->set( x0arridx, idz, trc->get(idz,0) );
    //rem: assuming that interesting data is at component 0;
    //allways true if coming from the engine, from where else?
    }
}


} // namespace Attrib
