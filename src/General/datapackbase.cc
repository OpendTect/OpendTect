/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Jan 2007
-*/


#include "datapackbase.h"

#include "arrayndimpl.h"
#include "binnedvalueset.h"
#include "convmemvalseries.h"
#include "flatposdata.h"
#include "interpol2d.h"
#include "staticstring.h"
#include "dbman.h"
#include "horsubsel.h"
#include "iopar.h"
#include "keystrs.h"
#include "scaler.h"
#include "separstr.h"
#include "survinfo.h"
#include "trckey.h"


// PointDataPack

PointDataPack::PointDataPack( const char* categry )
    : DataPack(categry)
{
}


PointDataPack::PointDataPack( const PointDataPack& oth )
    : DataPack(oth)
{
    copyClassData( oth );
}


PointDataPack::~PointDataPack()
{
    sendDelNotif();
}


mImplMonitorableAssignmentWithNoMembers( PointDataPack, DataPack );


Coord PointDataPack::coord( idx_type idx ) const
{
    return SI().transform( binID(idx) );
}


BinID PointDataPack::binID( idx_type idx ) const
{
    if ( is2D() )
	{ pErrMsg("2D/3D err"); }
    return BinID( lineNr(idx), trcNr(idx) );
}


Bin2D PointDataPack::bin2D( idx_type idx ) const
{
    if ( !is2D() )
	{ pErrMsg("2D/3D err"); }
    return Bin2D( GeomID(lineNr(idx)), trcNr(idx) );
}


Pos::GeomID PointDataPack::geomID( idx_type idx ) const
{
    return is2D() ? GeomID(lineNr(idx)) : GeomID::get3D();
}



// FlatDataPack
FlatDataPack::FlatDataPack( const char* cat )
    : DataPack(cat)
    , arr2d_(0)
    , posdata_(*new FlatPosData)
{
    // We cannot call init() here: size() does not dispatch virtual here
    // Subclasses with no arr3d_ will have to do a position init 'by hand'
}


FlatDataPack::FlatDataPack( const char* cat, Array2D<float>* arr )
    : DataPack(cat)
    , arr2d_(arr ? arr : new Array2DImpl<float>(1,1))
    , posdata_(*new FlatPosData)
{
    init();
}


FlatDataPack::FlatDataPack( const FlatDataPack& oth )
    : DataPack( oth )
    , arr2d_( oth.arr2d_ ? new Array2DImpl<float>( *oth.arr2d_ ) : 0 )
    , posdata_( *new FlatPosData(oth.posdata_) )
{
}


void FlatDataPack::init()
{
    if ( !arr2d_ )
	return;
    posdata_.setRange( true, StepInterval<double>(0,size(true)-1,1) );
    posdata_.setRange( false, StepInterval<double>(0,size(false)-1,1) );
}


FlatDataPack::~FlatDataPack()
{
    sendDelNotif();
    delete arr2d_;
    delete &posdata_;
}


mImplMonitorableAssignment( FlatDataPack, DataPack );
mImplAlwaysDifferentMonitorableCompareClassData( FlatDataPack )

void FlatDataPack::copyClassData( const FlatDataPack& oth )
{
    delete arr2d_;
    arr2d_ = oth.arr2d_ ? new Array2DImpl<float>( *oth.arr2d_ ) : 0;
    posdata_ = oth.posdata_;
}


Coord3 FlatDataPack::getCoord( idx_type i0, idx_type i1 ) const
{
    return Coord3( posData().range(true).atIndex(i0),
		   posData().range(false).atIndex(i1), mUdf(double) );
}


double FlatDataPack::getAltDim0Value( int ikey, idx_type idim0 ) const
{
    return posdata_.position( true, idim0 );
}


static const float kbfac = ((float)sizeof(float)) / 1024.0;


float FlatDataPack::gtNrKBytes() const
{
    return size(true) * kbfac * size(false);
}


void FlatDataPack::doDumpInfo( IOPar& iop ) const
{
    DataPack::doDumpInfo( iop );

    iop.set( sKey::Type(), "Flat" );
    const auto sz0 = size(true); const auto sz1 = size(false);
    for ( int idim : {0,1} )
    {
	const bool isdim0 = idim == 0;
	FileMultiString fms( dimName( isdim0 ) );
	fms += size( isdim0 );
	iop.set( IOPar::compKey("Dimension",idim), fms );
    }

    const FlatPosData& pd = posData();
    iop.set( "Positions.Dim0", pd.range(true).start, pd.range(true).stop,
			       pd.range(true).step );
    iop.set( "Positions.Dim1", pd.range(false).start, pd.range(false).stop,
			       pd.range(false).step );
    iop.setYN( "Positions.Irregular", pd.isIrregular() );
    if ( sz0 < 1 || sz1 < 1 )
	return;

    Coord3 c( getCoord(0,0) );
    iop.set( "Coord(0,0)", c );
    c = getCoord( sz0-1, sz1-1 );
    iop.set( "Coord(sz0-1,sz1-1)", c );
}


FlatDataPack::size_type FlatDataPack::size( bool dim0 ) const
{
    return data().getSize( dim0 ? 0 : 1 );
}



// MapDataPack
MapDataPack::MapDataPack( const char* cat, Array2D<float>* arr )
    : FlatDataPack(cat,arr)
    , idxsubsel_(new CubeHorSubSel)
    , dim0nm_("In-line")
    , dim1nm_("Cross-line")
{
    posdata_.set( *idxsubsel_ );
}


MapDataPack::MapDataPack( const MapDataPack& oth )
    : FlatDataPack(oth)
{
    copyClassData( oth );
}


MapDataPack::~MapDataPack()
{
    sendDelNotif();
    delete idxsubsel_;
}


mImplMonitorableAssignment( MapDataPack, FlatDataPack )
mImplAlwaysDifferentMonitorableCompareClassData( MapDataPack )


void MapDataPack::copyClassData( const MapDataPack& oth )
{
    dim0nm_ = oth.dim0nm_;
    dim1nm_ = oth.dim1nm_;
    setPositions( *oth.idxsubsel_ );
}


void MapDataPack::setPositions( const IdxSubSel2D& ss2d )
{
    if ( &ss2d != idxsubsel_ )
    {
	delete idxsubsel_;
	idxsubsel_ = static_cast<IdxSubSel2D*>( ss2d.clone() );
	posdata_.set( *idxsubsel_ );
    }
}


void MapDataPack::setDimNames( const char* d0nm, const char* d1nm )
{
    dim0nm_ = d0nm;
    dim1nm_ = d1nm;
}


const char* MapDataPack::dimName( bool dim0 ) const
{
    return dim0 ? dim0nm_ : dim1nm_;
}


void MapDataPack::getAuxInfo( idx_type idim0, idx_type idim1, IOPar& par ) const
{
    const BinID bid( idxsubsel_->binID(idim0,idim1) );
    par.set( dim0nm_, bid.inl() );
    par.set( dim1nm_, bid.crl() );
}


// VolumeDataPack

VolumeDataPack::VolumeDataPack( const char* cat, const BinDataDesc* bdd )
    : DataPack(cat)
    , zdomaininfo_(new ZDomain::Info(ZDomain::Time()))
    , desc_( bdd ? *bdd : BinDataDesc(false,true,sizeof(float)) )
    , scaler_(0)
{
    if ( !DBM().isBad() )
    {
	delete zdomaininfo_;
	zdomaininfo_ = new ZDomain::Info( ZDomain::SI() );
    }
}


VolumeDataPack::VolumeDataPack( const VolumeDataPack& oth )
    : DataPack(oth)
    , zdomaininfo_(0)
    , scaler_(0)
{
    copyClassData( oth );
}


VolumeDataPack::~VolumeDataPack()
{
    sendDelNotif();
    deepErase( arrays_ );
    deleteAndZeroPtr( zdomaininfo_ );
    deleteAndZeroPtr( scaler_ );
}


mImplMonitorableAssignment( VolumeDataPack, DataPack )
mImplAlwaysDifferentMonitorableCompareClassData( VolumeDataPack )


void VolumeDataPack::copyClassData( const VolumeDataPack& oth )
{
    componentnames_ = oth.componentnames_;
    refnrs_ = oth.refnrs_;
    desc_ = oth.desc_;

    deepErase( arrays_ );
    deleteAndZeroPtr( zdomaininfo_ );
    deleteAndZeroPtr( scaler_ );

    if ( oth.zdomaininfo_ )
	zdomaininfo_ = new ZDomain::Info( *oth.zdomaininfo_ );
    if ( oth.scaler_ )
	scaler_ = oth.scaler_->clone();

    for ( auto idx=0; idx<oth.arrays_.size(); idx++ )
	arrays_ += new Array3DImpl<float>( *oth.arrays_[idx] );
}


VolumeDataPack::glob_idx_type VolumeDataPack::getNearestGlobalIdx(
						const TrcKey& tk ) const
{
    if ( tk.isUdf() )
	return -1;
    const auto gidx = gtGlobalIdx( tk );
    if ( gidx >= 0 )
	return gidx;

    double mindistsq = mUdf(double);
    glob_idx_type bestidx = -1;
    const auto nrtrcs = nrPositions();
    for ( glob_idx_type idx=0; idx<nrtrcs; idx++ )
    {
	TrcKey curtk;
	getTrcKey( idx, curtk );
	if ( curtk.isUdf() )
	    continue;

	const auto curdistsq = tk.sqDistTo( curtk );
	if ( curdistsq < mindistsq )
	{
	    mindistsq = curdistsq;
	    bestidx = idx;
	}
    }

    return bestidx;
}


void VolumeDataPack::getPositions( BinnedValueSet& bvs ) const
{
    bvs.setEmpty();
    bvs.setIs2D( is2D() );
    const auto nrpos = nrPositions();
    for ( glob_idx_type idx=0; idx<nrpos; idx++ )
    {
	TrcKey curtk;
	getTrcKey( idx, curtk );
	if ( is2D() )
	    bvs.add( curtk.bin2D() );
	else
	    bvs.add( curtk.binID() );
    }
}


void VolumeDataPack::getPath( TrcKeyPath& tkp ) const
{
    tkp.setEmpty();
    const auto nrpos = nrPositions();
    for ( glob_idx_type idx=0; idx<nrpos; idx++ )
    {
	TrcKey curtk;
	getTrcKey( idx, curtk );
	tkp.add( curtk );
    }
}


OffsetValueSeries<float> VolumeDataPack::getTrcStorage( int comp,
				glob_idx_type globaltrcidx ) const
{
    const Array3D<float>* array = arrays_[comp];
    const auto* stor = array ? array->getStorage() : nullptr;
    od_int64 offs = 0; od_int64 zsz;
    if ( stor )
    {
	zsz = array->getSize( 2 );
	offs = (od_int64)globaltrcidx * zsz;
    }
    else
    {
	zsz = 1;
	static Array3DImpl<float> dummy( 1, 1, zsz );
	stor = dummy.getStorage();
    }

    return OffsetValueSeries<float>( *stor, offs, zsz );
}


const float* VolumeDataPack::getTrcData( int comp,
			    glob_idx_type globaltrcidx ) const
{
    return mSelf().getTrcData( comp, globaltrcidx );
}


float* VolumeDataPack::getTrcData( int comp, glob_idx_type globaltrcidx )
{
    if ( !arrays_.validIdx(comp) )
	return 0;

    Array3D<float>* array = arrays_[comp];
    if ( !array->getData() )
	return 0;
    return array->getData() + (od_int64)globaltrcidx * array->getSize(2);
}


bool VolumeDataPack::getCopiedTrcData( int comp, glob_idx_type globaltrcidx,
				       Array1D<float>& out ) const
{
    if ( !arrays_.validIdx(comp) )
	return false;

    const auto nrz = zRange().nrSteps() + 1;
    const float* dataptr = getTrcData( comp, globaltrcidx );
    float* outptr = out.getData();
    if ( dataptr )
    {
	if ( outptr )
	    OD::sysMemCopy( outptr, dataptr, nrz * sizeof(float) );
	else
	{
	    for ( auto idz=0; idz<nrz; idz++ )
		out.set( idz, dataptr[idz] );
	}

	return true;
    }

    const Array1DInfoImpl info1d( nrz );
    if ( out.getSize(0) != nrz && !out.setInfo(info1d) )
	return false;

    const Array3D<float>& array = *arrays_[comp];
    if ( array.getStorage() )
    {
	const OffsetValueSeries<float> offstor(
				       getTrcStorage(comp,globaltrcidx) );
	if ( outptr )
	{
	    for ( auto idz=0; idz<nrz; idz++ )
		outptr[idz] = offstor.value( idz );
	}
	else
	{
	    for ( auto idz=0; idz<nrz; idz++ )
		out.set( idz, offstor.value( idz ) );
	}

	return true;
    }

    const od_int64 offset = ((od_int64)globaltrcidx) * nrz;
    mAllocLargeVarLenArr( int, pos, array.nrDims() );
    if ( !array.info().getArrayPos(offset,pos) )
	return false;

    const auto idx = pos[0];
    const auto idy = pos[1];
    if ( outptr )
    {
	for ( auto idz=0; idz<nrz; idz++ )
	    outptr[idz] = array.get( idx, idy, idz );
    }
    else
    {
	for ( auto idz=0; idz<nrz; idz++ )
	    out.set( idz, array.get( idx, idy, idz ) );
    }

    return true;
}


void VolumeDataPack::setComponentName( const char* nm, int component )
{
    if ( componentnames_.validIdx(component) )
	componentnames_[component]->set( nm );
}


const char* VolumeDataPack::getComponentName( int component ) const
{
    return componentnames_.validIdx(component)
	? componentnames_[component]->buf() : 0;
}


int VolumeDataPack::getComponentIdx( const char* nm, int defcompidx ) const
{
    const int compidx = componentnames_.indexOf( nm );
    if ( compidx >= 0 )
	return compidx;

    // Trick for old steering cubes (see uiattribpartserv.cc)
    if ( stringEndsWith("Inline Dip",nm) && componentnames_.size()==2 )
	return 0;
    if ( stringEndsWith("Crossline Dip",nm) && componentnames_.size()==2 )
	return 1;

    if ( componentnames_.validIdx(defcompidx) )
	return defcompidx;

    return -1;
}


const char* VolumeDataPack::categoryStr( bool isvertical, bool is2d )
{
    mDeclStaticString( vret );
    vret = IOPar::compKey( is2d ? sKey::Attribute2D() : sKey::Attribute(), "V");
    return isvertical ? vret.str() : (is2d ? sKey::Attribute2D().str()
					   : sKey::Attribute().str());
}


float VolumeDataPack::getRefNr( glob_idx_type globaltrcidx ) const
{
    return refnrs_.validIdx(globaltrcidx) ? refnrs_[globaltrcidx] : mUdf(float);
}


BinID VolumeDataPack::binID( glob_idx_type gidx ) const
{
    if ( is2D() )
	{ pErrMsg("2D/3D err"); }
    TrcKey tk; gtTrcKey( gidx, tk );
    return tk.binID();
}


Bin2D VolumeDataPack::bin2D( glob_idx_type gidx ) const
{
    if ( !is2D() )
	{ pErrMsg("2D/3D err"); }
    TrcKey tk; gtTrcKey( gidx, tk );
    return tk.bin2D();
}


VolumeDataPack::glob_idx_type
VolumeDataPack::globalIdx( const BinID& bid ) const
{
    if ( is2D() )
	{ pErrMsg("2D/3D err"); }
    return gtGlobalIdx( TrcKey(bid) );
}


VolumeDataPack::glob_idx_type
VolumeDataPack::globalIdx( const Bin2D& b2d ) const
{
    if ( !is2D() )
	{ pErrMsg("2D/3D err"); }
    return gtGlobalIdx( TrcKey(b2d) );
}


VolumeDataPack::glob_idx_type
VolumeDataPack::nearestGlobalIdx( const BinID& bid ) const
{
    if ( is2D() )
	{ pErrMsg("2D/3D err"); }
    return getNearestGlobalIdx( TrcKey(bid) );
}


VolumeDataPack::glob_idx_type
VolumeDataPack::nearestGlobalIdx( const Bin2D& b2d ) const
{
    if ( !is2D() )
	{ pErrMsg("2D/3D err"); }
    return getNearestGlobalIdx( TrcKey(b2d) );
}


void VolumeDataPack::setZDomain( const ZDomain::Info& zinf )
{
    delete zdomaininfo_;
    zdomaininfo_ = new ZDomain::Info( zinf );
}


void VolumeDataPack::setScaler( const Scaler& scaler )
{
    delete scaler_;
    scaler_ = scaler.clone();
}


void VolumeDataPack::deleteScaler()
{
    deleteAndZeroPtr( scaler_ );
}


void VolumeDataPack::setDataDesc( const BinDataDesc& dc )
{
    if ( dc != desc_ && !isEmpty() )
	deepErase( arrays_ );

    desc_ = dc;
}


float VolumeDataPack::gtNrKBytes() const
{
    const int nrcomps = nrComponents();
    if ( nrcomps == 0 ) return 0.0f;
    return nrcomps * arrays_[0]->totalSize() * desc_.nrBytes() / 1024.f;
}


void VolumeDataPack::doDumpInfo( IOPar& iop ) const
{
    DataPack::doDumpInfo( iop );

    Scaler::putToPar( iop, scaler_ );
    DataCharacteristics( desc_ ).putUserType( iop );
}


bool VolumeDataPack::addArray( size_type sz0, size_type sz1, size_type sz2,
				bool initvals )
{
    float dummy; const BinDataDesc floatdesc( dummy );
    Array3DImpl<float>* arr = 0;
    if ( desc_ == floatdesc )
    {
	arr = new Array3DImpl<float>( sz0, sz1, sz2 );
	if ( !arr->isOK() )
	{
	    delete arr;
	    return false;
	}
    }
    else
    {
	arr = new Array3DImpl<float>( 0, 0, 0 );
	ConvMemValueSeries<float>* stor =
		new ConvMemValueSeries<float>( 0, desc_, scaler_ );
	arr->setStorage( stor );
	arr->setSize( sz0, sz1, sz2 );
	if ( !stor->storArr() )
	{
	    delete arr;
	    return false;
	}
    }

    if ( initvals )
	arr->setAll( mUdf(float) );

    arrays_ += arr;
    return true;
}
