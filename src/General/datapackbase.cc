/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2007
-*/


#include "datapackbase.h"

#include "arrayndimpl.h"
#include "convmemvalseries.h"
#include "flatposdata.h"
#include "interpol2d.h"
#include "dbman.h"
#include "iopar.h"
#include "keystrs.h"
#include "scaler.h"
#include "separstr.h"
#include "survinfo.h"

// MapDataPackXYRotater
class MapDataPackXYRotater : public ParallelTask
{
public:
    MapDataPackXYRotater( MapDataPack& mdp )
	: mdp_( mdp )
    {
	float anglenorth = mCast(float,
		 Math::Abs( mCast(double,SI().angleXInl()) - 90.*mDeg2RadD ) );
	if ( anglenorth > M_PI_2f )
	    anglenorth =  M_PIf - anglenorth;

	const int inlsz = mdp_.arr2d_->info().getSize(0);
	const int crlsz = mdp_.arr2d_->info().getSize(1);
	const float truelength = inlsz*cos(anglenorth) + crlsz*sin(anglenorth);
	const float truewidth = inlsz*sin(anglenorth) + crlsz*cos(anglenorth);
	const int length = mNINT32( truelength );
	const int width = mNINT32( truewidth );

	delete mdp_.xyrotarr2d_;
	mdp_.xyrotarr2d_ = new Array2DImpl<float>( length+1, width+1 );

	const StepInterval<double> tmpirg( mdp_.posdata_.range(true) );
	const StepInterval<double> tmpcrg( mdp_.posdata_.range(false) );
	hsamp_.set( StepInterval<int>((int)tmpirg.start,(int)tmpirg.stop,
		    (int)tmpirg.step),
		StepInterval<int>((int)tmpcrg.start,(int)tmpcrg.stop,
		    (int)tmpcrg.step) );
	const Coord spt1 =
	    SI().transform( BinID(hsamp_.start_.inl(),hsamp_.start_.crl()) );
	const Coord spt2 =
	    SI().transform( BinID(hsamp_.start_.inl(),hsamp_.stop_.crl()) );
	const Coord spt3 =
	    SI().transform( BinID(hsamp_.stop_.inl(),hsamp_.start_.crl()) );
	const Coord spt4 =
	    SI().transform( BinID(hsamp_.stop_.inl(),hsamp_.stop_.crl()) );
	startpt_ = Coord( mMIN( mMIN(spt1.x_, spt2.x_),
			  mMIN(spt3.x_, spt4.x_) ),
		mMIN( mMIN(spt1.y_, spt2.y_), mMIN(spt3.y_, spt4.y_) ) );
	stoppt_ = Coord( mMAX( mMAX(spt1.x_, spt2.x_), mMAX(spt3.x_, spt4.x_) ),
		mMAX( mMAX(spt1.y_, spt2.y_), mMAX(spt3.y_, spt4.y_) ) );
	xstep_ = ( float ) (stoppt_.x_ - startpt_.x_)/length;
	ystep_ = ( float ) (stoppt_.y_ - startpt_.y_)/width;
    }

    od_int64 nrIterations() const
    { return mdp_.xyrotarr2d_->info().getTotalSz(); }

    bool doFinish( bool success )
    {
	mdp_.xyrotposdata_.setRange( true,
		StepInterval<double>(startpt_.x_,stoppt_.x_,xstep_) );
	mdp_.xyrotposdata_.setRange( false,
		StepInterval<double>(startpt_.y_,stoppt_.y_,ystep_) );

	return success;
    }

    bool doWork( od_int64 start, od_int64 stop, int )
    {
	int startpos[2];
	if ( !mdp_.xyrotarr2d_->info().getArrayPos(start,startpos) )
	    return false;

	ArrayNDIter iter( mdp_.xyrotarr2d_->info() );
	iter.setPos( startpos );

	BinID toreach00;
	const int nriters = mCast( int, stop-start+1 );
	for ( od_int64 idx=0; idx<nriters && shouldContinue();
		idx++, iter.next(), addToNrDone(1) )
	{
	    const int* curpos = iter.getPos();
	    const Coord coord( startpt_.x_+curpos[0]*xstep_,
			       startpt_.y_+curpos[1]*ystep_ );
	    float val = mUdf(float );
	    BinID approxbid = SI().transform( coord );
	    if ( hsamp_.lineRange().includes(approxbid.lineNr(),false)
		    && hsamp_.trcRange().includes(approxbid.trcNr(),false))
	    {
		approxbid = hsamp_.getNearest( approxbid );
		const Coord approxcoord = SI().transform( approxbid );
		float diffx = (float) ((coord.x_-approxcoord.x_) / xstep_);
		float diffy = (float) ((coord.y_-approxcoord.y_) / ystep_);
		toreach00.inl() = diffx>=0 ? 0 : -1;
		toreach00.crl() = diffy>=0 ? 0 : -1;
		const int id0v00 = (approxbid.inl() - hsamp_.start_.inl()) /
			hsamp_.step_.inl() + toreach00.inl();
		const int id1v00 = (approxbid.crl() - hsamp_.start_.crl())/
			hsamp_.step_.crl() + toreach00.crl();
		const float val00 = mdp_.getValAtIdx( id0v00, id1v00 );
		const float val01 = mdp_.getValAtIdx( id0v00 , id1v00+1 );
		const float val10 = mdp_.getValAtIdx( id0v00+1, id1v00 );
		const float val11 = mdp_.getValAtIdx( id0v00+1, id1v00+1 );
		if ( diffx<0 ) diffx = 1 + diffx;
		if ( diffy<0 ) diffy = 1 + diffy;
		val = Interpolate::linearReg2DWithUdf( val00, val01, val10,
						       val11, diffx, diffy );
	    }

	    mdp_.xyrotarr2d_->set( curpos[0], curpos[1], val );
	}

	return true;
    }

protected:

    MapDataPack&	mdp_;
    Coord		startpt_;
    Coord		stoppt_;
    TrcKeySampling		hsamp_;
    float		xstep_;
    float		ystep_;
};



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


Coord PointDataPack::coord( int idx ) const
{
    return SI().transform( binID(idx) );
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
    if ( !arr2d_ ) return;
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


Coord3 FlatDataPack::getCoord( int i0, int i1 ) const
{
    return Coord3( posData().range(true).atIndex(i0),
		   posData().range(false).atIndex(i1), mUdf(double) );
}


double FlatDataPack::getAltDim0Value( int ikey, int idim0 ) const
{
    return posdata_.position( true, idim0 );
}


static const float kbfac = ((float)sizeof(float)) / 1024.0;


float FlatDataPack::nrKBytes() const
{
    return size(true) * kbfac * size(false);
}


void FlatDataPack::dumpInfo( IOPar& iop ) const
{
    DataPack::dumpInfo( iop );
    iop.set( sKey::Type(), "Flat" );
    const int sz0 = size(true); const int sz1 = size(false);
    for ( int idim=0; idim<2; idim++ )
    {
	const bool isdim0 = idim == 0;
	FileMultiString fms( dimName( isdim0 ) ); fms += size( isdim0 );
	iop.set( IOPar::compKey("Dimension",idim), fms );
    }

    const FlatPosData& pd = posData();
    iop.set( "Positions.Dim0", pd.range(true).start, pd.range(true).stop,
			       pd.range(true).step );
    iop.set( "Positions.Dim1", pd.range(false).start, pd.range(false).stop,
			       pd.range(false).step );
    iop.setYN( "Positions.Irregular", pd.isIrregular() );
    if ( sz0 < 1 || sz1 < 1 ) return;

    Coord3 c( getCoord(0,0) );
    iop.set( "Coord(0,0)", c );
    c = getCoord( sz0-1, sz1-1 );
    iop.set( "Coord(sz0-1,sz1-1)", c );
}


int FlatDataPack::size( bool dim0 ) const
{
    return data().info().getSize( dim0 ? 0 : 1 );
}



// MapDataPack
MapDataPack::MapDataPack( const char* cat, Array2D<float>* arr )
    : FlatDataPack(cat,arr)
    , isposcoord_(false)
    , xyrotarr2d_(0)
    , xyrotposdata_(*new FlatPosData)
    , axeslbls_(4,"")
{
}


MapDataPack::MapDataPack( const MapDataPack& oth )
    : FlatDataPack(oth)
    , xyrotarr2d_(0)
    , xyrotposdata_(*new FlatPosData)
{
    copyClassData( oth );
}


MapDataPack::~MapDataPack()
{
    sendDelNotif();
    delete xyrotarr2d_;
    delete &xyrotposdata_;
}


mImplMonitorableAssignment( MapDataPack, FlatDataPack )
mImplAlwaysDifferentMonitorableCompareClassData( MapDataPack )


void MapDataPack::copyClassData( const MapDataPack& oth )
{
    delete xyrotarr2d_;
    if ( oth.xyrotarr2d_ )
	xyrotarr2d_ = new Array2DImpl<float>( *xyrotarr2d_ );
    posdata_ = oth.xyrotposdata_;
    isposcoord_ = oth.isposcoord_;
    axeslbls_ = oth.axeslbls_;
}


void MapDataPack::getAuxInfo( int idim0, int idim1, IOPar& par ) const
{
    const Coord3 pos = getCoord( idim0, idim1 );
    const Coord coord = isposcoord_ ? pos.getXY() :
			SI().transform(BinID(mNINT32(pos.x_),mNINT32(pos.y_)));
    const BinID bid = isposcoord_ ? SI().transform(pos.getXY())
				  : BinID(mNINT32(pos.x_),mNINT32(pos.y_));
    par.set( axeslbls_.get(0), coord.x_ );
    par.set( axeslbls_.get(1), coord.y_ );
    par.set( axeslbls_.get(2), bid.inl() );
    par.set( axeslbls_.get(3), bid.crl() );
}


float MapDataPack::getValAtIdx( int idx, int idy ) const
{
    const int nrrows = arr2d_->info().getSize(0);
    const int nrcols = arr2d_->info().getSize(1);
    return ( idx>=0 && idy>=0 && idx<nrrows && idy<nrcols )
	     ? arr2d_->get( idx, idy ) : mUdf(float);
}


void MapDataPack::setPosCoord( bool isposcoord )
{
    isposcoord_ = isposcoord;
}


void MapDataPack::setProps( StepInterval<double> inlrg,
			    StepInterval<double> crlrg,
			    bool isposcoord, BufferStringSet* dimnames )
{
    posdata_.setRange( true, inlrg );
    posdata_.setRange( false, crlrg );
    if ( dimnames )
    {
	for ( int setidx=0; setidx<dimnames->size(); setidx+=2 )
	    setDimNames( dimnames->get(setidx),
			 dimnames->get(setidx+1), !setidx );
    }

    isposcoord_ = isposcoord;
}


void MapDataPack::initXYRotArray( TaskRunner* tskr )
{
    MapDataPackXYRotater rotator( *this );
    TaskRunner::execute( tskr, rotator );
}


Array2D<float>& MapDataPack::data()
{
    Threads::Locker lck( initlock_ );
    if ( isposcoord_ && !xyrotarr2d_ )
	initXYRotArray( 0 );

    return isposcoord_ ? *xyrotarr2d_ : *arr2d_;
}


FlatPosData& MapDataPack::posData()
{
    Threads::Locker lck( initlock_ );
    if ( isposcoord_ && !xyrotarr2d_ )
	initXYRotArray( 0 );

    return isposcoord_ ? xyrotposdata_ : posdata_;
}


void MapDataPack::setDimNames( const char* xlbl, const char* ylbl, bool forxy )
{
    if ( forxy )
    {
	axeslbls_.get(0) = xlbl;
	axeslbls_.get(1) = ylbl;
    }
    else
    {
	axeslbls_.get(2) = xlbl;
	axeslbls_.get(3) = ylbl;
    }
}


const char* MapDataPack::dimName( bool dim0 ) const
{
    return (dim0 ? isposcoord_ ? axeslbls_.get(0) : axeslbls_.get(2)
		 : isposcoord_ ? axeslbls_.get(1) : axeslbls_.get(3) ).buf();
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

    for ( int idx=0; idx<oth.arrays_.size(); idx++ )
	arrays_ += new Array3DImpl<float>( *oth.arrays_[idx] );
}


int VolumeDataPack::getNearestGlobalIdx( const TrcKey& tk ) const
{
    if ( tk.isUdf() ) return -1;

    const int gidx = getGlobalIdx( tk );
    if ( gidx >= 0 ) return gidx;

    double dist = mUdf(double);
    int bestidx = -1;
    for ( int idx=0; idx<nrTrcs(); idx++ )
    {
	const TrcKey curtk = getTrcKey( idx );
	if ( curtk.isUdf() ) continue;

	const double curdist = tk.distTo( curtk );
	if ( curdist < dist )
	{
	    dist = curdist;
	    bestidx = idx;
	}
    }

    return bestidx;
}


void VolumeDataPack::getPath( TrcKeyPath& path ) const
{
    path.erase();
    for ( int idx=0; idx<nrTrcs(); idx++ )
        path += getTrcKey( idx );
}


const OffsetValueSeries<float>
    VolumeDataPack::getTrcStorage(int comp, int globaltrcidx) const
{
    const Array3D<float>* array = arrays_[comp];
    return OffsetValueSeries<float>( *array->getStorage(),
			(od_int64)globaltrcidx * array->info().getSize(2) );
}


OffsetValueSeries<float> VolumeDataPack::getTrcStorage( int comp,
							int globaltrcidx )
{
    Array3D<float>* array = arrays_[comp];
    return OffsetValueSeries<float>( *array->getStorage(),
			(od_int64)globaltrcidx * array->info().getSize(2) );
}


const float* VolumeDataPack::getTrcData( int comp, int globaltrcidx ) const
{
    if ( !arrays_.validIdx(comp) )
	return 0;

    const Array3D<float>* array = arrays_[comp];
    if ( !array->getData() ) return 0;
    return array->getData() + (od_int64)globaltrcidx * array->info().getSize(2);
}


float* VolumeDataPack::getTrcData( int comp, int globaltrcidx )
{
    if ( !arrays_.validIdx(comp) )
	return 0;

    Array3D<float>* array = arrays_[comp];
    if ( !array->getData() ) return 0;
    return array->getData() + (od_int64)globaltrcidx * array->info().getSize(2);
}


bool VolumeDataPack::getCopiedTrcData( int comp, int globaltrcidx,
				       Array1D<float>& out ) const
{
    if ( !arrays_.validIdx(comp) )
	return false;

    const int nrz = getZRange().nrSteps()+1;
    const float* dataptr = getTrcData( comp, globaltrcidx );
    float* outptr = out.getData();
    if ( dataptr )
    {
	if ( outptr )
	{
	    OD::memCopy( outptr, dataptr, mCast(od_int64,nrz) * sizeof(float) );
	}
	else
	{
	    for ( int idz=0; idz<nrz; idz++ )
		out.set( idz, dataptr[idz] );
	}

	return true;
    }

    const Array1DInfoImpl info1d( nrz );
    if ( out.info().getSize(0) != nrz && !out.setInfo(info1d) )
	return false;

    const Array3D<float>& array = *arrays_[comp];
    const ValueSeries<float>* stor = array.getStorage();
    if ( stor )
    {
	const OffsetValueSeries<float> offstor(
				       getTrcStorage(comp,globaltrcidx) );
	if ( outptr )
	{
	    for ( int idz=0; idz<nrz; idz++ )
		outptr[idz] = offstor.value( idz );
	}
	else
	{
	    for ( int idz=0; idz<nrz; idz++ )
		out.set( idz, offstor.value( idz ) );
	}

	return true;
    }

    const od_uint64 offset = mCast(od_uint64,globaltrcidx) * nrz;
    mAllocLargeVarLenArr( int, pos, array.info().getNDim() );
    if ( !array.info().getArrayPos(offset,pos) )
	return false;

    const int idx = pos[0];
    const int idy = pos[1];
    if ( outptr )
    {
	for ( int idz=0; idz<nrz; idz++ )
	    outptr[idz] = array.get( idx, idy, idz );
    }
    else
    {
	for ( int idz=0; idz<nrz; idz++ )
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

    // Trick for old steering cubes (see uiatribpartserv.cc)
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


float VolumeDataPack::getRefNr( int globaltrcidx ) const
{
    return refnrs_.validIdx(globaltrcidx) ? refnrs_[globaltrcidx] : mUdf(float);
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


float VolumeDataPack::nrKBytes() const
{
    const int nrcomps = nrComponents();
    if ( nrcomps == 0 ) return 0.0f;
    return nrcomps * arrays_[0]->info().getTotalSz() * desc_.nrBytes() / 1024.f;
}


void VolumeDataPack::dumpInfo( IOPar& iop ) const
{
    DataPack::dumpInfo( iop );
    BufferString info;
    if ( scaler_ )
    {
	scaler_->put( info.getCStr() );
	iop.set( sKey::Scale(), info.buf() );
    }

    DataCharacteristics dc( desc_ );
    const FixedString usertypestr =
	DataCharacteristics::toString( dc.userType() );
    if ( usertypestr.size() > 4 )
	info.set( usertypestr.buf()+4 );
    iop.set( sKey::DataStorage(), info.buf() );
}


bool VolumeDataPack::addArray( int sz0, int sz1, int sz2 )
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

    arr->setAll( mUdf(float) );
    arrays_ += arr;
    return true;
}


const Array3DImpl<float>& VolumeDataPack::data( int component ) const
{ return *arrays_[component]; }

Array3DImpl<float>& VolumeDataPack::data( int component )
{ return *arrays_[component]; }
