/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/


#include "prestackgather.h"

#include "genericnumer.h"
#include "flatposdata.h"
#include "ioobj.h"
#include "ioobjtags.h"
#include "iopar.h"
#include "ioman.h"
#include "perthreadrepos.h"
#include "ptrman.h"
#include "samplfunc.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "seispsioprov.h"
#include "seispsread.h"
#include "seisread.h"
#include "seistrctr.h"
#include "seistrcprop.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "keystrs.h"

static PerThreadObjectRepository<SeisTrc> rettrc_;

namespace PreStack
{

const char* Gather::sDataPackCategory()		{ return "Pre-Stack Gather"; }
const char* Gather::sKeyIsAngleGather()		{ return "Angle Gather"; }
const char* Gather::sKeyIsCorr()		{ return "Is Corrected"; }
const char* Gather::sKeyZisTime()		{ return "Z Is Time"; }
const char* Gather::sKeyPostStackDataID()	{ return "Post Stack Data"; }
const char* Gather::sKeyStaticsID()		{ return "Statics"; }
const char* GatherSetDataPack::sDataPackCategory()
{ return "Pre-Stack Gather Set"; }

Gather::Gather()
    : FlatDataPack( sDataPackCategory(), new Array2DImpl<float>(1,1) )
    , offsetisangle_( false )
    , iscorr_( true )
    , coord_( 0, 0 )
    , zit_( SI().zIsTime() )
{}



Gather::Gather( const Gather& gather )
    : FlatDataPack( gather )
    , velocitymid_( gather.velocitymid_ )
    , storagemid_( gather.storagemid_ )
    , staticsmid_( gather.staticsmid_ )
    , offsetisangle_( gather.offsetisangle_ )
    , iscorr_( gather.iscorr_ )
    , zit_( gather.zit_ )
    , tk_( gather.tk_ )
    , coord_( gather.coord_ )
    , azimuths_( gather.azimuths_ )
    , zrg_(gather.zrg_)
{}


Gather::Gather( const FlatPosData& fposdata )
    : FlatDataPack( sDataPackCategory(),
        new Array2DImpl<float>(fposdata.nrPts(true),fposdata.nrPts(false)) )
    , offsetisangle_( false )
    , iscorr_( true )
    , coord_( 0, 0 )
    , zit_( SI().zIsTime() )
{
    posdata_ = fposdata;
    const StepInterval<double> zsamp = fposdata.range( false );
    zrg_.set( mCast(float,zsamp.start), mCast(float,zsamp.stop),
	      mCast(float,zsamp.step) );
}


Gather::~Gather()
{}


bool Gather::readFrom( const MultiID& mid, const BinID& bid, int comp,
		       uiString* errmsg )
{
    const TrcKey tk( bid );
    return readFrom( mid, tk, comp, errmsg );
}


bool Gather::readFrom( const MultiID& mid, const TrcKey& tk, int comp,
		       uiString* errmsg )
{
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
    {
	if ( errmsg ) (*errmsg) = tr("No valid gather selected.");
	deleteAndZeroPtr( arr2d_ );
	return false;
    }

    return readFrom( *ioobj, tk, comp, errmsg );
}


bool Gather::readFrom( const IOObj& ioobj, const BinID& bid, int comp,
		       uiString* errmsg )
{
    const TrcKey tk( bid );
    return readFrom( ioobj, tk, comp, errmsg );
}


bool Gather::readFrom( const IOObj& ioobj, const TrcKey& tk, int comp,
		       uiString* errmsg )
{
    PtrMan<SeisPSReader> rdr = SPSIOPF().getReader( ioobj, tk );
    if ( !rdr )
    {
	if ( errmsg )
	    (*errmsg) = tr("This Prestack data store cannot be handled.");
	deleteAndZeroPtr( arr2d_ );
	return false;
    }

    return readFrom( ioobj, *rdr, tk, comp, errmsg );
}


bool Gather::readFrom( const MultiID& mid, const int trcnr,
		       const char* linename, int comp, uiString* errmsg )
{
    const TrcKey tk( Survey::GM().getGeomID( linename ), trcnr );
    return readFrom( mid, tk, comp, errmsg );
}


bool Gather::readFrom( const IOObj& ioobj, const int trcnr,
		       const char* linename, int comp, uiString* errmsg )
{
    const TrcKey tk( Survey::GM().getGeomID( linename ), trcnr );
    return readFrom( ioobj, tk, comp, errmsg );
}


bool Gather::readFrom( const IOObj& ioobj, SeisPSReader& rdr, const BinID& bid,
		       int comp, uiString* errmsg )
{
    TrcKey tk;
    if ( rdr.is3D() )
	tk.setPosition( bid );
    else if ( rdr.is2D() )
    {
	const Pos::GeomID gid = rdr.geomID();
	if ( Survey::is2DGeom(gid) )
	    tk.setGeomID( rdr.geomID() ).setTrcNr( bid.trcNr() );
	else
	    { pErrMsg("Invalid geomID for 2D"); }
    }

    return readFrom( ioobj, rdr, tk, comp, errmsg );
}


bool Gather::readFrom( const IOObj& ioobj, SeisPSReader& rdr, const TrcKey& tk,
		       int comp, uiString* errmsg )
{
    if ( tk.isUdf() )
	return false;

    PtrMan<SeisTrcBuf> tbuf = new SeisTrcBuf( true );
    if ( !rdr.getGather(tk.position(),*tbuf) )
    {
	if ( errmsg ) (*errmsg) = rdr.errMsg();
	deleteAndZeroPtr( arr2d_ );
	return false;
    }

    if ( !setFromTrcBuf(*tbuf,comp,true) )
       return false;

    ioobj.pars().getYN(sKeyZisTime(),zit_);

    velocitymid_.setUdf();
    GetVelocityVolumeTag( ioobj, velocitymid_ );
    staticsmid_.setUdf();
    ioobj.pars().get( sKeyStaticsID(), staticsmid_ );

    offsetisangle_ = false;
    ioobj.pars().getYN(sKeyIsAngleGather(), offsetisangle_ );

    iscorr_ = true;
    if ( !ioobj.pars().getYN(sKeyIsCorr(),iscorr_) )
	ioobj.pars().getYN( "Is NMO Corrected", iscorr_ );

    tk_ = tk;
    setName( ioobj.name() );

    storagemid_ = ioobj.key();

    return true;
}


bool Gather::setFromTrcBuf( SeisTrcBuf& tbuf, int comp, bool snapzrgtosi )
{
    tbuf.sort( true, SeisTrcInfo::Offset );

    bool isset = false;
    StepInterval<float> zrg;
    Coord crd( coord_ );
    for ( int idx=tbuf.size()-1; idx>-1; idx-- )
    {
	const SeisTrc* trc = tbuf.get( idx );
	if ( mIsUdf( trc->info().offset ) )
	    delete tbuf.remove( idx );

	const int trcsz = trc->size();
	if ( !isset )
	{
	    isset = true;
	    zrg.setFrom( trc->info().sampling.interval( trcsz ) );
	    crd = trc->info().coord;
	}
	else
	{
	    zrg.start = mMIN( trc->info().sampling.start, zrg.start );
	    zrg.stop = mMAX( trc->info().sampling.atIndex(trcsz-1), zrg.stop );
	    zrg.step = mMIN( trc->info().sampling.step, zrg.step );
	}
    }

    if ( !isset )
    {
	deleteAndZeroPtr( arr2d_ );
	return false;
    }

    if ( snapzrgtosi )
    {
	SI().snapZ(zrg.start);
	SI().snapZ(zrg.stop);
    }

    zrg_ = zrg;
    int nrsamples = zrg.nrSteps()+1;
    if ( zrg.step>0 && (zrg.stop-zrg.atIndex(nrsamples-1))>fabs(zrg.step*0.5) )
	nrsamples++;

    const Array2DInfoImpl newinfo( tbuf.size(), nrsamples );
    if ( !arr2d_ || !arr2d_->setInfo( newinfo ) )
    {
	delete arr2d_;
	arr2d_ = new Array2DImpl<float>( newinfo );
	if ( !arr2d_ || !arr2d_->isOK() )
	{
	    deleteAndZeroPtr( arr2d_ );
	    return false;
	}
    }

    azimuths_.setSize( tbuf.size(), mUdf(float) );

    for ( int trcidx=tbuf.size()-1; trcidx>=0; trcidx-- )
    {
	const SeisTrc* trc = tbuf.get( trcidx );
	for ( int idx=0; idx<nrsamples; idx++ )
	{
	    const float val = trc->getValue( zrg.atIndex( idx ), comp );
	    arr2d_->set( trcidx, idx, val );
	}

	azimuths_[trcidx] = trc->info().azimuth;
    }

    double offset;
    float* offsets = tbuf.getHdrVals(SeisTrcInfo::Offset, offset);
    if ( !offsets )
	return false;

    posData().setX1Pos( offsets, tbuf.size(), offset );
    StepInterval<double> pzrg(zrg.start, zrg.stop, zrg.step);
    posData().setRange( false, pzrg );

    zit_ = SI().zIsTime();
    coord_ = crd;
    if ( tbuf.isEmpty() )
	tk_.setPosition( SI().transform(coord_) );
    else
	tk_ = tbuf.first()->info().trcKey();

    return true;
}


const char* Gather::dimName( bool dim0 ) const
{
    return dim0 ? sKey::Offset() :
			(SI().zIsTime() ? sKey::Time() : sKey::Depth());
}


void Gather::getAuxInfo( int idim0, int idim1, IOPar& par ) const
{
    par.set( "X", coord_.x );
    par.set( "Y", coord_.y );
    float z = (float) posData().position( false, idim1 );
    if ( zit_ ) z *= 1000;
    par.set( "Z", z );
    par.set( sKey::Offset(), getOffset(idim0) );
    if ( azimuths_.validIdx(idim0) )
	par.set( sKey::Azimuth(), getAzimuth(idim0) );
    if ( !is3D() )
	par.set( sKey::TraceNr(), tk_.trcNr() );
    else
	par.set( sKey::Position(), tk_.position().toString() );
}


int Gather::getSeis2DTraceNr() const
{
    return tk_.trcNr();
}

const char* Gather::getSeis2DName() const
{
    return is2D() ? Survey::GM().getName( tk_.geomID() ) : nullptr;
}


void Gather::setOffsetIsAngle( bool yn )
{
    offsetisangle_ = yn;
}


float Gather::getOffset( int idx ) const
{ return (float) posData().position( true, idx ); }


float Gather::getAzimuth( int idx ) const
{
    return azimuths_[idx];
}


OffsetAzimuth Gather::getOffsetAzimuth( int idx ) const
{
    return OffsetAzimuth( (float) posData().position( true, idx ),
			  azimuths_[idx] );
}


bool Gather::getVelocityID(const MultiID& stor, MultiID& vid )
{
    PtrMan<IOObj> ioobj = IOM().get( stor );
    return ioobj && GetVelocityVolumeTag( *ioobj, vid );
}


const BinID& Gather::getBinID() const
{
    return tk_.position();
}

void Gather::setBinID( const BinID& bid )
{
    tk_.setPosition( bid );
}

#define mIfNonZero \
const float val = data().get( offset, idz ); \
if ( val && !mIsUdf(val) )

void Gather::detectInnerMutes( int* res, int taperlen ) const
{
    const int nroffsets = size( !offsetDim() );
    const int nrz = size( offsetDim() );

    int muteend = 0;
    for ( int offset=0; offset<nroffsets; offset++ )
    {
	for ( int idz=nrz-1; idz>=muteend; idz-- )
	{
	    mIfNonZero
		muteend = idz+1;
	}

	res[offset] = muteend-taperlen;
    }
}


void Gather::detectOuterMutes( int* res, int taperlen ) const
{
    const int nroffsets = size( !offsetDim() );
    const int nrz = size( offsetDim() );

    int muteend = nrz-1;
    for ( int offset=nroffsets-1; offset>=0; offset-- )
    {
	for ( int idz=0; idz<=muteend; idz++ )
	{
	    mIfNonZero
		muteend = idz-1;
	}

	res[offset] = muteend + taperlen;
    }
}


class GatherSetArray3D : public Array3D<float>
{
public:
		GatherSetArray3D(ObjectSet<Gather>&);
		~GatherSetArray3D();

    bool	isOK() const override;

    const Array3DInfo&	info() const override	{ return *info_; }

    void	set(int,int,int,float) override;
    float	get(int,int,int) const override;

    void	setCache();

private:

    const float* getData_() const override	{ return nullptr; }

    ObjectSet<Gather>& gathers_;
    Array3DInfo*	info_ = nullptr;
};


GatherSetArray3D::GatherSetArray3D( ObjectSet<Gather>& gathers )
    : gathers_(gathers)
    , info_(nullptr)
{
    setCache();
}


GatherSetArray3D::~GatherSetArray3D()
{
    delete info_;
}


void GatherSetArray3D::setCache()
{
    int nroffs = 0;
    int nrsamples = 0;
    for ( const auto* gather : gathers_ )
    {
	const Array2DInfo& gathinfo = gather->data().info();
	const int nroff = gathinfo.getSize( Gather::offsetDim() );
	const int nrz = gathinfo.getSize( Gather::zDim() );
	if ( nroff > nroffs )
	    nroffs = nroff;
	if ( nrz > nrsamples )
	    nrsamples = nrz;
    }

    if ( nroffs < 1 || nrsamples < 1 )
	return;

    delete info_;
    info_ = new Array3DInfoImpl( gathers_.size(), nroffs, nrsamples );
}


bool GatherSetArray3D::isOK() const
{
    return info_ && info_->getSize(0) == gathers_.size();
}


void GatherSetArray3D::set( int idx, int idy, int idz, float val )
{
#ifdef __debug__
    if ( !gathers_.validIdx(idx) || !isOK() )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif

    gathers_.get( idx )->data().set( idy, idz, val );
}


float GatherSetArray3D::get( int idx, int idy, int idz ) const
{
#ifdef __debug__
    if ( !gathers_.validIdx(idx) || !isOK() )
	{ pErrMsg("Invalid access"); DBG::forceCrash(true); }
#endif

    return gathers_.get( idx )->data().get( idy, idz );
}



GatherSetDataPack::GatherSetDataPack( const char* /* ctgery */ )
    : DataPack( sDataPackCategory() )
    , arr3d_(*new GatherSetArray3D(gathers_))
{
}


GatherSetDataPack::GatherSetDataPack( const char* /* ctgery */,
				      const ObjectSet<Gather>& gathers )
    : DataPack( sDataPackCategory() )
    , gathers_( gathers )
    , arr3d_(*new GatherSetArray3D(gathers_))
{
    for ( auto* gather : gathers_ )
	gather->setName( name() );
}


GatherSetDataPack::~GatherSetDataPack()
{
    delete &arr3d_;
    DataPackMgr& dpm = DPM( DataPackMgr::FlatID() );
    while ( !gathers_.isEmpty() )
    {
	auto* gather = gathers_.pop();
	const DataPack::ID id = gather->id();
	if ( dpm.haveID(id) )
	    dpm.release( id );
	else
	    delete gather;
    }
}


const Gather* GatherSetDataPack::getGather( const BinID& bid ) const
{
    for ( const auto* gather : gathers_ )
    {
	if ( gather->getBinID() == bid )
	    return gather;
    }

    return nullptr;
}


void GatherSetDataPack::obtainGathers()
{
    DataPackMgr& dpm = DPM( DataPackMgr::FlatID() );
    for ( auto* gather : gathers_ )
    {
	if ( !dpm.haveID(gather->id()) )
	    dpm.addAndObtain( gather );
    }
}


void GatherSetDataPack::fill( Array2D<float>& inp, int offsetidx ) const
{
    for ( int idx=0; idx<gathers_.size(); idx++ )
    {
	const Array2D<float>& data = gathers_[idx]->data();
	for ( int idz=0; idz<data.info().getSize(0); idz++ )
	    inp.set( idx, idz, data.get( offsetidx, idz ) );
    }
}


void GatherSetDataPack::fill( SeisTrcBuf& inp, int offsetidx ) const
{
    for ( const auto* gather : gathers_ )
    {
	const int gathersz = gather->size(false);
	auto* trc = new SeisTrc( gathersz );
	const TrcKey tk( gather->getBinID(), !gather->is3D() );
	trc->info().setTrcKey( tk ).calcCoord();
	const SamplingData<double>& sd = gather->posData().range( false);
	trc->info().sampling.set((float) sd.start, (float) sd.step );
	const Array2D<float>& data = gather->data();
	for ( int idz=0; idz<gathersz; idz++ )
	    trc->set( idz, data.get( offsetidx, idz ), 0 );

	inp.add( trc );
    }
}


void GatherSetDataPack::fill( SeisTrcBuf& inp, Interval<float> stackrg ) const
{
    const int gathersz = gathers_.size();
    TypeSet<int> offidxs;
    for ( int idx=0; idx<gathersz; idx++ )
    {
	const Gather& gather = *gathers_[idx];
	const int offsz = gather.size( true );

	for ( int idoff=0; idoff<offsz; idoff++ )
	{
	    if ( stackrg.includes( gather.getOffset( idoff ), true ) )
		offidxs.addIfNew( idoff );
	}
    }
    if ( offidxs.isEmpty() )
	return;

    for ( int idx=0; idx<offidxs.size(); idx++ )
    {
	if ( inp.isEmpty() )
	    fill( inp, offidxs[idx] );
	else
	{
	    SeisTrcBuf buf(false); fill( buf, offidxs[idx] );
	    for ( int idgather=0; idgather<gathersz; idgather++ )
	    {
		SeisTrcPropChg stckr( *inp.get( idgather ) );
		stckr.stack( *buf.get( idgather ) );
	    }
	}
    }
}


const SeisTrc* GatherSetDataPack::getTrace(int gatheridx,int offsetidx) const
{
    return gtTrace( gatheridx, offsetidx );
}


SeisTrc* GatherSetDataPack::getTrace( int gatheridx, int offsetidx )
{
    return gtTrace( gatheridx, offsetidx );
}


SeisTrc* GatherSetDataPack::gtTrace( int gatheridx, int offsetidx ) const
{
    SeisTrc& rettrc = rettrc_.getObject();
    const auto& gather = *gathers_[gatheridx];
    const Array2D<float>& data = gather.data();
    const int nrsamples = data.info().getSize( 1 );

    rettrc.reSize( nrsamples, false );
    rettrc.info().setTrcKey( gather.getTrcKey() ).calcCoord();
    rettrc.info().offset = gather.getOffset( offsetidx );
    //TODO: set azimuth ?
    const SamplingData<double>& sd = gather.posData().range( false );
    rettrc.info().sampling.set( (float)sd.start, (float)sd.step );
    for ( int isamp=0; isamp<nrsamples; isamp++ )
	rettrc.set( isamp, data.get(offsetidx,isamp), 0 );

    return &rettrc;
}


StepInterval<float> GatherSetDataPack::zRange() const
{
    if ( gathers_.isEmpty() )
	return StepInterval<float>::udf();

    StepInterval<float> zrg = gathers_[0]->zRange();
    for ( int idx=1; idx<gathers_.size(); idx++ )
	zrg.include( gathers_[idx]->zRange(), false );

    return zrg;
}


void GatherSetDataPack::addGather( Gather& gather )
{
    gather.setName( name() );
    gathers_.add( &gather );
}


void GatherSetDataPack::finalize()
{
    static_cast<GatherSetArray3D&>( arr3d_ ).setCache();
}

} // namespace PreStack
