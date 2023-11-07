/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackgather.h"

#include "genericnumer.h"
#include "flatposdata.h"
#include "ioobj.h"
#include "iopar.h"
#include "ioman.h"
#include "keystrs.h"
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
#include "veldesc.h"

static PerThreadObjectRepository<SeisTrc> rettrc_;

namespace PreStack
{

const char* Gather::sDataPackCategory()		{ return "Pre-Stack Gather"; }
const char* Gather::sKeyPostStackDataID()	{ return "Post Stack Data"; }
const char* Gather::sKeyStaticsID()		{ return "Statics"; }
const char* GatherSetDataPack::sDataPackCategory()
{ return "Pre-Stack Gather Set"; }


// PreStack::Gather

Gather::Gather()
    : FlatDataPack(sDataPackCategory(),new Array2DImpl<float>(1,1))
    , coord_(0.,0.)
    , offsettype_(SI().xyInFeet() ? Seis::OffsetFeet : Seis::OffsetMeter)
    , zdomaininfo_(new ZDomain::Info(SI().zDomainInfo()))
{
}


Gather::Gather( const Gather& oth )
    : FlatDataPack(oth)
    , velocitymid_(oth.velocitymid_)
    , storagemid_(oth.storagemid_)
    , staticsmid_(oth.staticsmid_)
    , iscorr_(oth.iscorr_)
    , offsettype_(oth.offsettype_)
    , zdomaininfo_(new ZDomain::Info(oth.zDomain()))
    , tk_(oth.tk_)
    , coord_(oth.coord_)
    , azimuths_(oth.azimuths_)
    , zrg_(oth.zrg_)
{}


Gather::Gather( const FlatPosData& fposdata, Seis::OffsetType offsettype,
		const ZDomain::Info& zinfo )
    : FlatDataPack( sDataPackCategory(),
        new Array2DImpl<float>(fposdata.nrPts(true),fposdata.nrPts(false)) )
    , offsettype_(offsettype)
    , zdomaininfo_(new ZDomain::Info(zinfo))
    , coord_(0.,0.)
{
    posdata_ = fposdata;
    const StepInterval<double> zsamp = fposdata.range( false );
    zrg_.set( mCast(float,zsamp.start), mCast(float,zsamp.stop),
	      mCast(float,zsamp.step) );
}


Gather::~Gather()
{
    delete zdomaininfo_;
}


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
	deleteAndNullPtr( arr2d_ );
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
	deleteAndNullPtr( arr2d_ );
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

    SeisTrcBuf tbuf( true );
    if ( !rdr.getGather(tk.position(),tbuf) )
    {
	if ( errmsg )
	    (*errmsg) = rdr.errMsg();

	deleteAndNullPtr( arr2d_ );
	return false;
    }

    const ZDomain::Info& zinfo = SeisStoreAccess::zDomain( &ioobj );
    if ( zinfo.isTime() || zinfo.isDepth() )
	setZDomain( zinfo );

    bool iscorr;
    if ( SeisPSIOProvider::getGatherCorrectedYN(ioobj.pars(),iscorr) )
	iscorr_ = iscorr;

    Seis::OffsetType offsettype;
    if ( SeisPSIOProvider::getGatherOffsetType(ioobj.pars(),offsettype) )
	setOffsetType( offsettype );

    if ( !setFromTrcBuf(tbuf,comp,iscorr_,offsettype_,zDomain(),true) )
       return false;

    velocitymid_.setUdf();
    ioobj.pars().get( VelocityDesc::sKeyVelocityVolume(), velocitymid_ );
    staticsmid_.setUdf();
    ioobj.pars().get( sKeyStaticsID(), staticsmid_ );

    tk_ = tk;
    setName( ioobj.name() );

    storagemid_ = ioobj.key();

    return true;
}


bool Gather::setFromTrcBuf( SeisTrcBuf& tbuf, int comp,
			    const GatherSetDataPack& gdp, bool snapzrgtosi )
{
    return setFromTrcBuf( tbuf, comp, gdp.isCorrected(), gdp.offsetType(),
			  gdp.zDomain(), snapzrgtosi );
}


bool Gather::setFromTrcBuf( SeisTrcBuf& tbuf, int comp, bool iscorrected,
			    Seis::OffsetType offstyp,
			    const ZDomain::Info& zinfo, bool snapzrgtosi )
{
    tbuf.sort( true, SeisTrcInfo::Offset );

    bool isset = false;
    ZSampling zrg;
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
	    zrg = trc->info().sampling.interval( trcsz );
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
	deleteAndNullPtr( arr2d_ );
	return false;
    }

    setZDomain( zinfo );
    setOffsetType( offstyp );
    if ( snapzrgtosi && zDomain() == SI().zDomainInfo() )
    {
	SI().snapZ( zrg.start );
	SI().snapZ( zrg.stop );
    }

    zrg_ = zrg;
    int nrsamples = zrg.nrSteps()+1;
    if ( zrg.step>0 && (zrg.stop-zrg.atIndex(nrsamples-1))>fabs(zrg.step*0.5) )
	nrsamples++;

    const Array2DInfoImpl newinfo( tbuf.size(), nrsamples );
    if ( !arr2d_ || !arr2d_->setInfo(newinfo) )
    {
	delete arr2d_;
	arr2d_ = new Array2DImpl<float>( newinfo );
	if ( !arr2d_ || !arr2d_->isOK() )
	{
	    deleteAndNullPtr( arr2d_ );
	    return false;
	}
    }

    azimuths_.setSize( tbuf.size(), mUdf(float) );

    for ( int trcidx=tbuf.size()-1; trcidx>=0; trcidx-- )
    {
	const SeisTrc* trc = tbuf.get( trcidx );
	for ( int idx=0; idx<nrsamples; idx++ )
	{
	    const float val = trc->getValue( zrg.atIndex(idx), comp );
	    arr2d_->set( trcidx, idx, val );
	}

	azimuths_[trcidx] = trc->info().azimuth;
    }

    double offset;
    float* offsets = tbuf.getHdrVals(SeisTrcInfo::Offset, offset);
    if ( !offsets )
	return false;

    posData().setX1Pos( offsets, tbuf.size(), offset );
    StepInterval<double> pzrg( zrg.start, zrg.stop, zrg.step );
    posData().setRange( false, pzrg );

    coord_ = crd;
    if ( tbuf.isEmpty() )
	tk_.setPosition( SI().transform(coord_) );
    else
	tk_ = tbuf.first()->info().trcKey();

    return true;
}


const char* Gather::dimName( bool dim0 ) const
{
    return dim0 ? sKey::Offset().buf()
		: ::toString( zDomain().def_.uiUnitStr() );
}


void Gather::getAuxInfo( int idim0, int idim1, IOPar& par ) const
{
    par.set( "X", coord_.x );
    par.set( "Y", coord_.y );
    const float z = (float) posData().position( false, idim1 ) *
		    zDomain().def_.userFactor();

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


bool Gather::isCorrected() const
{
    return iscorr_;
}


bool Gather::isOffsetAngle() const
{
    return Seis::isOffsetAngle( offsettype_ );
}


bool Gather::isOffsetInMeters() const
{
    return offsettype_ == Seis::OffsetMeter;
}


bool Gather::isOffsetInFeet() const
{
    return offsettype_ == Seis::OffsetFeet;
}


Seis::OffsetType Gather::offsetType() const
{
    return offsettype_;
}


Gather& Gather::setCorrected( bool yn )
{
    iscorr_ = yn;
    return *this;
}


Gather& Gather::setOffsetType( Seis::OffsetType typ )
{
    offsettype_ = typ;
    return *this;
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


const ZDomain::Info& Gather::zDomain() const
{
    return *zdomaininfo_;
}


bool Gather::zIsTime() const
{
    return zDomain().isTime();
}


bool Gather::zInMeter() const
{
    return zDomain().isDepthMeter();
}


bool Gather::zInFeet() const
{
    return zDomain().isDepthFeet();
}


Gather& Gather::setZDomain( const ZDomain::Info& zinfo )
{
    if ( (!zinfo.isTime() && !zinfo.isDepth()) || zinfo == zDomain() )
	return *this;

    delete zdomaininfo_;
    zdomaininfo_ = new ZDomain::Info( zinfo );
    return *this;
}


bool Gather::getVelocityID(const MultiID& stor, MultiID& vid )
{
    PtrMan<IOObj> ioobj = IOM().get( stor );
    return ioobj && ioobj->pars().get( VelocityDesc::sKeyVelocityVolume(), vid);
}


const BinID& Gather::getBinID() const
{
    return tk_.position();
}

Gather& Gather::setBinID( const BinID& bid )
{
    tk_.setPosition( bid );
    return *this;
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
    , offsettype_(SI().xyInFeet() ? Seis::OffsetFeet : Seis::OffsetMeter)
    , zdomaininfo_(new ZDomain::Info(SI().zDomainInfo()))
{
}


GatherSetDataPack::GatherSetDataPack( const char* /* ctgery */,
				      const ObjectSet<Gather>& gathers )
    : DataPack(sDataPackCategory())
    , gathers_(gathers)
    , arr3d_(*new GatherSetArray3D(gathers_))
    , offsettype_(SI().xyInFeet() ? Seis::OffsetFeet : Seis::OffsetMeter)
    , zdomaininfo_(new ZDomain::Info(SI().zDomainInfo()))
{
    const OD::String& nm = name();
    for ( auto* gather : gathers_ )
	gather->setName( nm );

    if ( gathers.isEmpty() )
	return;

    const Gather& gather = *gathers.first();
    setCorrected( gather.isCorrected() );
    setOffsetType( gather.offsetType() );
    setZDomain( gather.zDomain() );
}


GatherSetDataPack::~GatherSetDataPack()
{
    delete &arr3d_;
    delete zdomaininfo_;
}


void GatherSetDataPack::setName( const char* nm )
{
    DataPack::setName( nm );
    for ( auto* gather : gathers_ )
	gather->setName( nm );
}


int GatherSetDataPack::nrGathers() const
{
    return gathers_.size();
}


ConstRefMan<Gather> GatherSetDataPack::getGather( int idx ) const
{
    return gathers_.validIdx(idx) ? gathers_.get( idx ) : nullptr;
}


ConstRefMan<Gather> GatherSetDataPack::getGather( const BinID& bid ) const
{
    for ( const auto* gather : gathers_ )
    {
	if ( gather->getBinID() == bid )
	    return gather;
    }

    return nullptr;
}


Interval<float> GatherSetDataPack::offsetRange() const
{
    Interval<float> offrg( 0.f, 0.f );
    if ( !gathers_.isEmpty() )
    {
	const Gather& gather = *gathers_.first();
        offrg.set(gather.getOffset(0),gather.getOffset( gather.size(true)-1));
    }
    return offrg;
}


float GatherSetDataPack::offsetRangeStep() const
{
    float offsetstep = mUdf(float);
    if ( !gathers_.isEmpty() )
    {
	const Gather& gather = *gathers_.first();
        offsetstep = gather.getOffset(1)-gather.getOffset(0);
    }

    return offsetstep;
}


TrcKey GatherSetDataPack::getTrcKeyByIdx( int idx ) const
{
    return gathers_.validIdx(idx) ? gathers_.get( idx )->getTrcKey()
				  : TrcKey::udf();
}


DataPackID GatherSetDataPack::getGatherIDByIdx( int idx ) const
{
    return gathers_.validIdx( idx ) ? gathers_.get(idx)->id()
				    : DataPackID::udf();
}


DataPackID GatherSetDataPack::getGatherID( const BinID& bid ) const
{
    for ( const auto* gather : gathers_ )
    {
	if ( gather->getBinID() == bid )
	    return gather->id();
    }

    return DataPackID::udf();
}


void GatherSetDataPack::obtainGathers()
{
    DataPackMgr& dpm = DPM( DataPackMgr::FlatID() );
    for ( auto* gather : gathers_ )
    {
	if ( !dpm.isPresent(gather->id()) )
	    dpm.add( gather );
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


ZSampling GatherSetDataPack::zRange() const
{
    if ( gathers_.isEmpty() )
	return ZSampling::udf();

    ZSampling zrg = gathers_[0]->zRange();
    for ( int idx=1; idx<gathers_.size(); idx++ )
	zrg.include( gathers_[idx]->zRange(), false );

    return zrg;
}


bool GatherSetDataPack::isCorrected() const
{
    return iscorr_;
}


bool GatherSetDataPack::isOffsetAngle() const
{
    return Seis::isOffsetAngle( offsettype_ );
}


bool GatherSetDataPack::isOffsetInMeters() const
{
    return offsettype_ == Seis::OffsetMeter;
}


bool GatherSetDataPack::isOffsetInFeet() const
{
    return offsettype_ == Seis::OffsetFeet;
}


Seis::OffsetType GatherSetDataPack::offsetType() const
{
    return offsettype_;
}


GatherSetDataPack& GatherSetDataPack::setOffsetType( Seis::OffsetType typ )
{
    offsettype_ = typ;
    return *this;
}


GatherSetDataPack& GatherSetDataPack::setCorrected( bool yn )
{
    iscorr_ = yn;
    return *this;
}


const ZDomain::Info& GatherSetDataPack::zDomain() const
{
    return *zdomaininfo_;
}


bool GatherSetDataPack::zIsTime() const
{
    return zDomain().isTime();
}


bool GatherSetDataPack::zInMeter() const
{
    return zDomain().isDepthMeter();
}


bool GatherSetDataPack::zInFeet() const
{
    return zDomain().isDepthFeet();
}


GatherSetDataPack& GatherSetDataPack::setZDomain( const ZDomain::Info& zinfo )
{
    if ( (!zinfo.isTime() && !zinfo.isDepth()) || zinfo == zDomain() )
	return *this;

    delete zdomaininfo_;
    zdomaininfo_ = new ZDomain::Info( zinfo );
    return *this;
}


void GatherSetDataPack::addGather( Gather& gather )
{
    if ( gathers_.isEmpty() )
    {
	setCorrected( gather.isCorrected() );
	setOffsetType( gather.offsetType() );
	setZDomain( gather.zDomain() );
    }

    gather.setName( name() );
    gathers_.add( &gather );
}


void GatherSetDataPack::finalize()
{
    static_cast<GatherSetArray3D&>( arr3d_ ).setCache();
}

} // namespace PreStack
