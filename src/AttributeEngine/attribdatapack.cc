/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          January 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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

static const FixedString sAttribute2D()	{ return "Attribute2D"; }

const char* DataPackCommon::categoryStr( bool vertical, bool is2d )
{
    static BufferString vret;
    vret = IOPar::compKey( is2d ? sAttribute2D() : sKey::Attribute(),"V" );
    return vertical ? vret.buf() : is2d ? sAttribute2D() : sKey::Attribute();
}


void DataPackCommon::dumpInfo( IOPar& iop ) const
{
    iop.set( "Source type", sourceType() );
    iop.set( "Attribute.ID", descID().asInt() );
    BufferString isstoredstr = IOPar::compKey( sKey::Attribute(), sKey::Stored() );
    iop.set( isstoredstr.buf(), descID().isStored() );
    iop.set( "Vertical", isVertical() );
}


Flat3DDataPack::Flat3DDataPack( DescID did, const DataCubes& dc, int cubeidx )
    : ::FlatDataPack(categoryStr(true))
    , DataPackCommon(did)
    , cube_(dc)
    , cubeidx_( cubeidx )	       
    , arr2dsl_(0)
    , arr2dsource_(0)
    , usemultcubes_( cubeidx == -1 )
{
    cube_.ref();
    if ( cube_.getInlSz() < 2 )
	dir_ = CubeSampling::Inl;
    else if ( cube_.getCrlSz() < 2 )
	dir_ = CubeSampling::Crl;
    else
	dir_ = CubeSampling::Z;
	
    if ( usemultcubes_ )
    {
	createA2DSFromMultCubes();
	cubeidx_ = -1;
    }
    else
	createA2DSFromSingCube( cubeidx );
    
    arr2dsl_->init();

    setPosData();
}


Flat3DDataPack::~Flat3DDataPack()
{
    if ( arr2dsource_ ) delete arr2dsource_;
    delete arr2dsl_;
    cube_.unRef();
}


void Flat3DDataPack::createA2DSFromSingCube( int cubeidx )
{
    int unuseddim, dim0, dim1;
    if ( dir_==CubeSampling::Inl )
    {
	unuseddim = DataCubes::cInlDim();
	dim0 = DataCubes::cCrlDim();
	dim1 = DataCubes::cZDim();
    }
    else if ( dir_==CubeSampling::Crl )
    {
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

    if ( !arr2dsl_ )
	arr2dsl_ = new Array2DSlice<float>( cube_.getCube(cubeidx) );

    arr2dsl_->setPos( unuseddim, 0 );
    arr2dsl_->setDimMap( 0, dim0 );
    arr2dsl_->setDimMap( 1, dim1 );
}
	

void Flat3DDataPack::createA2DSFromMultCubes()
{
    const CubeSampling cs = cube_.cubeSampling();
    bool isvert = cs.nrZ() > 1;
    int dim0sz = isvert ? cube_.nrCubes()
			: dir_==CubeSampling::Inl ? cube_.getCrlSz()
						  : cube_.getInlSz();
    int dim1sz = isvert ? cube_.getZSz() : cube_.nrCubes();

    arr2dsource_ = new Array2DImpl<float>( dim0sz, dim1sz );
    for ( int id0=0; id0<dim0sz; id0++ )
    {
	for ( int id1=0; id1<dim1sz; id1++ )
	{
	    float val = isvert ? cube_.getCube(id0).get( 0, 0, id1 )
			       : dir_==CubeSampling::Inl 
			       		? cube_.getCube(id1).get( 0, id0, 0 )
			       		: cube_.getCube(id1).get( id0, 0, 0 );
	    arr2dsource_->set( id0, id1, val );
	}
    }
    
    arr2dsl_ = new Array2DSlice<float>( *arr2dsource_ );
    arr2dsl_->setDimMap( 0, 0 );
    arr2dsl_->setDimMap( 1, 1 );
}


bool Flat3DDataPack::setDataDir( CubeSampling::Dir dir )
{
    if ( arr2dsource_ )
    {
	pErrMsg("Not supported");
	return false;
    }

    dir_ = dir;
    createA2DSFromSingCube( cubeidx_ );
    return arr2dsl_->init();
}

#define mGetDim( unuseddim ) \
    if ( dir_==CubeSampling::Inl ) \
	unuseddim = DataCubes::cInlDim(); \
    else if ( dir_==CubeSampling::Crl ) \
	unuseddim = DataCubes::cCrlDim(); \
    else \
	unuseddim = DataCubes::cZDim()

int Flat3DDataPack::nrSlices() const
{
    if ( arr2dsource_ ) return 1;

    int unuseddim;
    mGetDim( unuseddim );

    return arr2dsl_->getDimSize( unuseddim );
}


int Flat3DDataPack::getDataSlice() const
{
    if ( arr2dsource_ ) return 0;

    int unuseddim;
    mGetDim( unuseddim );

    return arr2dsl_->getPos( unuseddim );
}


bool Flat3DDataPack::setDataSlice( int pos )
{
    if ( arr2dsource_ )
    {
	pErrMsg("Not supported");
	return false;
    }

    int unuseddim;
    mGetDim( unuseddim );

    if ( pos<0 || pos>=arr2dsl_->getDimSize( unuseddim ) )
	return false;

    arr2dsl_->setPos( unuseddim, pos );
    return true;
}


Array2D<float>& Flat3DDataPack::data()
{
    return *arr2dsl_;
}


void Flat3DDataPack::setPosData()
{
    const CubeSampling cs = cube_.cubeSampling();
    if ( usemultcubes_ )
    {
	StepInterval<int> cubeintv( 0, cube_.nrCubes(), 1 );
	bool isvert = cs.nrZ() > 1;
	posdata_.setRange( true, 
	    isvert ? mStepIntvD(cubeintv)
		   : dir_==CubeSampling::Inl ? mStepIntvD(cs.hrg.crlRange())
					     : mStepIntvD(cs.hrg.inlRange()) );
	posdata_.setRange(false, isvert ? mStepIntvD(cs.zrg)
					: mStepIntvD(cubeintv) );
    }
    else
    {
	posdata_.setRange( true, dir_==CubeSampling::Inl
	    ? mStepIntvD(cs.hrg.crlRange()) : mStepIntvD(cs.hrg.inlRange()) );
	posdata_.setRange( false, dir_==CubeSampling::Z
	    ? mStepIntvD(cs.hrg.crlRange()) : mStepIntvD(cs.zrg) );
    }
}


void Flat3DDataPack::dumpInfo( IOPar& iop ) const
{
    ::FlatDataPack::dumpInfo( iop );
    DataPackCommon::dumpInfo( iop );
}


Coord3 Flat3DDataPack::getCoord( int i0, int i1 ) const
{
    const CubeSampling& cs = cube_.cubeSampling();
    int inlidx = i0; int crlidx = 0; int zidx = i1;
    if ( usemultcubes_ )
    {
	if ( cs.nrZ() > 1 )
	    inlidx = 0;
	else if ( dir_==CubeSampling::Inl )
	    { inlidx = 0; crlidx = i0; zidx = 0; }
	else
	    { zidx = 0; }
    }
    else
    {
	if ( dir_ == CubeSampling::Inl )
	    { inlidx = 0; crlidx = i0; }
	else if ( dir_ == CubeSampling::Z )
	    { crlidx = i1; zidx = 0; }
    }

    const Coord c = SI().transform( cs.hrg.atIndex(inlidx,crlidx) );
    return Coord3(c.x,c.y,cs.zrg.atIndex(zidx));
}


#define mKeyInl SeisTrcInfo::getFldString(SeisTrcInfo::BinIDInl)
#define mKeyCrl SeisTrcInfo::getFldString(SeisTrcInfo::BinIDCrl)
#define mKeyX SeisTrcInfo::getFldString(SeisTrcInfo::CoordX)
#define mKeyY SeisTrcInfo::getFldString(SeisTrcInfo::CoordY)
#define mKeyCube "Series"
//TODO : find a way to get a better name than "Series"

const char* Flat3DDataPack::dimName( bool dim0 ) const
{
    if ( usemultcubes_ )
    {
	if ( cube_.cubeSampling().nrZ() > 1 )
	    return dim0 ? mKeyCube : "Z";
	else if ( dir_ == CubeSampling::Inl )
	    return dim0 ? mKeyCrl : mKeyCube;
	else
	    return dim0 ? mKeyInl : mKeyCube;
    }
    
    return dim0 ? (dir_==CubeSampling::Inl ? mKeyCrl : mKeyInl)
		: (dir_==CubeSampling::Z ? mKeyCrl : "Z");
}


void Flat3DDataPack::getAltDim0Keys( BufferStringSet& bss ) const
{
    if ( usemultcubes_ && cube_.cubeSampling().nrZ() > 1 )
	bss.add( mKeyCube );
    if ( dir_== CubeSampling::Crl )
	bss.add( mKeyInl );
    else
	bss.add( mKeyCrl );
    bss.add( mKeyX );
    bss.add( mKeyY );
}


double Flat3DDataPack::getAltDim0Value( int ikey, int i0 ) const
{
    if ( ikey < 1 || ikey > 3 )
	 return FlatDataPack::getAltDim0Value( ikey, i0 );

    if ( usemultcubes_ && cube_.cubeSampling().nrZ() > 1 )
	return i0;	//TODO: now returning cube idx, what else can we do?
    
    const Coord3 c( getCoord(i0,0) );
    return ikey == 1 ? c.x : c.y;
}


void Flat3DDataPack::getAuxInfo( int i0, int i1, IOPar& iop ) const
{
    const Coord3 c( getCoord(i0,i1) );
    
    BinID bid = SI().transform( c );
    iop.set( mKeyX, c.x );
    iop.set( mKeyY, c.y );
    iop.set( "Inline", bid.inl );
    iop.set( "Crossline", bid.crl );
    iop.set( "Z", c.z*SI().zDomain().userFactor() );

    if ( usemultcubes_ )
	iop.set( mKeyCube, cube_.cubeSampling().nrZ() > 1 ? i0 : i1 );
}


Flat2DDataPack::Flat2DDataPack( DescID did )
    : ::FlatDataPack(categoryStr(true,true))
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
	bss.add( SeisTrcInfo::getFldString(tiflds_[idx]) );
}


const char* Flat2DDataPack::dimName( bool dim0 ) const
{
    return dim0 ? "Distance" : "Z";
}


Flat2DDHDataPack::Flat2DDHDataPack( DescID did, const Data2DHolder& dh,
       				    bool usesingtrc, int component )
    : Flat2DDataPack( did )
    , usesingtrc_( usesingtrc )
    , dataholderarr_( 0 )
    , array2dslice_( 0 )
{
    ConstRefMan<Data2DHolder> dataref( &dh );
    mTryAlloc( dataholderarr_, Data2DArray( dh ) );
    if ( !dataholderarr_ )
	return;
    
    dataholderarr_->ref();

    if ( !dataholderarr_->isOK() )
    {
	dataholderarr_->unRef();
	dataholderarr_ = 0;
	return;
    }

    mTryAlloc( array2dslice_, Array2DSlice<float>(*dataholderarr_->dataset_) );

    if ( !array2dslice_ )
	return;

    if ( usesingtrc )
    {
	array2dslice_->setPos( 1, 0 );
	array2dslice_->setDimMap( 0, 0 );
    }
    else
    {
	const int nrseries = dataholderarr_->dataset_->info().getSize( 0 );
	if ( nrseries==1 )
	    component = 0;
	array2dslice_->setPos( 0, component );
	array2dslice_->setDimMap( 0, 1 );
    }

    array2dslice_->setDimMap( 1, 2 );
    array2dslice_->init();

    setPosData();
}


Flat2DDHDataPack::~Flat2DDHDataPack()
{
    dataholderarr_->unRef();
}


void Flat2DDHDataPack::getPosDataTable( TypeSet<int>& trcnrs,
					TypeSet<float>& dist ) const
{
    trcnrs.erase(); dist.erase();
    const int nrtrcs = dataholderarr_->trcinfoset_.size();
    trcnrs.setSize( nrtrcs, -1 );
    dist.setSize( nrtrcs, -1 );
    for ( int idx=0; idx<nrtrcs; idx++ )
    {
	trcnrs[idx] = dataholderarr_->trcinfoset_[idx]->nr;
	if ( posdata_.width(true)/posdata_.range(true).step > idx )
	    dist[idx] = (float) posdata_.position( true, idx );
	else
	    trcnrs[idx] = -1;
    }
}


const CubeSampling& Flat2DDHDataPack::getCubeSampling() const
{ return dataholderarr_->cubesampling_; }


void Flat2DDHDataPack::getCoordDataTable( const TypeSet<int>& trcnrs,
					  TypeSet<Coord>& coords ) const
{
    if ( trcnrs.size() > 0 )
	coords.setSize( dataholderarr_->trcinfoset_.size(), Coord::udf() );

    for ( int idx=0; idx<trcnrs.size(); idx++ )
    {
	if ( trcnrs[idx] == dataholderarr_->trcinfoset_[idx]->nr )
	    coords[idx] = dataholderarr_->trcinfoset_[idx]->coord;
    }
}


Array2D<float>& Flat2DDHDataPack::data()
{
    return *array2dslice_;
}


void Flat2DDHDataPack::getLineName( BufferString& nm ) const
{
    nm = linenm_;
}

#define mNrTrcDim 1


void Flat2DDHDataPack::setPosData()
{
    const int nrpos =
	    usesingtrc_ ? dataholderarr_->dataset_->info().getSize(0)
			: dataholderarr_->dataset_->info().getSize(mNrTrcDim);
    if ( nrpos < 1 ) return;

    if ( usesingtrc_ )
	posdata_.setRange( true, StepInterval<double>( 0, nrpos-1, 1 ) );
    else
    {
	float* pos = new float[nrpos];
	pos[0] = 0;
	Coord prevcrd = dataholderarr_->trcinfoset_[0]->coord;
	for ( int idx=1; idx<nrpos; idx++ )
	{
	    Coord crd = dataholderarr_->trcinfoset_[idx]->coord;
	    pos[idx] = (float) (pos[idx-1] +
				   dataholderarr_->trcinfoset_[idx-1]->coord.distTo( crd ));
	    prevcrd = crd;
	}

	posdata_.setX1Pos( pos, nrpos, 0 );
    }

    const StepInterval<float> zrg = dataholderarr_->cubesampling_.zrg;
    posdata_.setRange( false, mStepIntvD(zrg) );
}


double Flat2DDHDataPack::getAltDim0Value( int ikey, int i0 ) const
{
    const int nrpos =
	    usesingtrc_ ? dataholderarr_->dataset_->info().getSize(0)
			: dataholderarr_->dataset_->info().getSize(mNrTrcDim);
    bool isi0wrong = i0<0 || i0>=nrpos;

    if ( isi0wrong || !tiflds_.validIdx(ikey) )
	return FlatDataPack::getAltDim0Value( ikey, i0 );

     if ( usesingtrc_ )
	return i0;	//what else can we do?


    return dataholderarr_->trcinfoset_[i0]->getValue( tiflds_[ikey] );
}


void Flat2DDHDataPack::getAuxInfo( int i0, int i1, IOPar& iop ) const
{
    int trcinfoidx = usesingtrc_ ? 0 : i0;
    if ( trcinfoidx<0 || trcinfoidx>= dataholderarr_->trcinfoset_.size() )
	return;
    
    const SeisTrcInfo& ti = *dataholderarr_->trcinfoset_[ trcinfoidx ];
    ti.getInterestingFlds( Seis::Line, iop );
    iop.set( "Z-Coord", ti.sampling.atIndex(i1)*SI().zDomain().userFactor() );
}


Coord3 Flat2DDHDataPack::getCoord( int i0, int i1 ) const
{
    if ( dataholderarr_->trcinfoset_.isEmpty() ) return Coord3();

    if ( i0 < 0 || usesingtrc_ ) i0 = 0;
    if ( i0 >= dataholderarr_->trcinfoset_.size() )
	i0 = dataholderarr_->trcinfoset_.size() - 1;
    const SeisTrcInfo& ti = *dataholderarr_->trcinfoset_[i0];
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
    , path_(new TypeSet<BinID>(*path))  
{
    seisbuf_ = new SeisTrcBuf( true );
    sb.copyInto(*seisbuf_);

    const int nrtrcs = seisbuf_->size();
    const int arrsz0 = path ? path->size() : nrtrcs; 
    const int nrsamp = nrtrcs ? seisbuf_->get(0)->size() : 0;
    arr2d_ = new Array2DImpl<float>( arrsz0, nrsamp );
    fill2DArray( path );
    setPosData( path );
}


FlatRdmTrcsDataPack::~FlatRdmTrcsDataPack()
{
    delete arr2d_;
    if ( seisbuf_ ) seisbuf_->erase();
    delete path_;
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
	    float distnnm1 = (float) prevcrd.distTo(crd);
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
    iop.set( "Z-Coord", ti.samplePos(i1)*SI().zDomain().userFactor() );
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
