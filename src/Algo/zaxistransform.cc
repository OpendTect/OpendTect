/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "zaxistransform.h"

#include "iopar.h"
#include "keystrs.h"
#include "scaler.h"
#include "survinfo.h"
#include "binidvalue.h"
#include "zdomain.h"
#include "survgeom.h"


mImplFactory( ZAxisTransform, ZAxisTransform::factory );


ZAxisTransform* ZAxisTransform::create( const IOPar& par )
{
    const BufferString str = par.find( sKey::Name() );
    if ( str.isEmpty() )
	return nullptr;

    ZAxisTransform* res = factory().create( str );
    if ( !res )
	return nullptr;

    res->ref();
    if ( !res->usePar( par ) )
    {
	res->unRef();
	return nullptr;
    }

    return res;
}


ZAxisTransform::ZAxisTransform( const ZDomain::Def& from,
				const ZDomain::Def& to )
    : fromzdomaininfo_(*new ZDomain::Info(from))
    , tozdomaininfo_(*new ZDomain::Info(to))
{
    if ( from.isDepth() )
	fromzdomaininfo_.setDepthUnit( SI().depthType() );
    if ( to.isDepth() )
	tozdomaininfo_.setDepthUnit( SI().depthType() );

    datafromzdominfo_ = &ZDomain::Info::getFrom( fromzdomaininfo_ );
    datatozdominfo_ = &ZDomain::Info::getFrom( tozdomaininfo_ );
}


ZAxisTransform::~ZAxisTransform()
{
    delete &fromzdomaininfo_;
    delete &tozdomaininfo_;
}


int ZAxisTransform::addVolumeOfInterest( const TrcKeyZSampling&, bool )
{
    return -1;
}


void ZAxisTransform::setVolumeOfInterest( int, const TrcKeyZSampling&, bool )
{}


void ZAxisTransform::removeVolumeOfInterest( int )
{}


bool ZAxisTransform::loadDataIfMissing(int,TaskRunner*)
{
    return !needsVolumeOfInterest();
}


bool ZAxisTransform::isReferenceHorizon( const MultiID& horid,
					 float& refz ) const
{
    return false;
}


float ZAxisTransform::transformTrc( const TrcKey& trckey, float z ) const
{
    SamplingData<float> sd( z, 1 );
    float res;
    transformTrc( trckey, sd, 1, &res );
    return res;
}


float ZAxisTransform::transformTrcBack( const TrcKey& trckey, float z ) const
{
    SamplingData<float> sd( z, 1 );
    float res;
    transformTrcBack( trckey, sd, 1, &res );
    return res;
}


void ZAxisTransform::transform( const BinID& bid,
				const SamplingData<float>& sd,
				int sz,float* res) const
{
    transformTrc( TrcKey(bid), sd, sz, res );
}


void ZAxisTransform::transformBack( const BinID& bid,
				    const SamplingData<float>& sd,
				    int sz, float* res ) const
{
    transformTrcBack( TrcKey(bid), sd, sz, res );
}


float ZAxisTransform::transform( const Coord3& pos ) const
{
    return transform( BinIDValue(SI().transform(pos),(float) pos.z_) );
}


float ZAxisTransform::transform( const BinIDValue& pos ) const
{
    float res = mUdf(float);
    transform( pos, SamplingData<float>(pos.val(),1), 1, &res );
    return res;
}


float ZAxisTransform::transformBack( const Coord3& pos ) const
{
    return transformBack( BinIDValue(SI().transform(pos),(float) pos.z_) );
}


float ZAxisTransform::transformBack( const BinIDValue& pos ) const
{
    float res = mUdf(float);
    transformBack( pos, SamplingData<float>(pos.val(),1), 1, &res );
    return res;
}


void ZAxisTransform::transform2D( const char* linenm, int trcnr,
		const SamplingData<float>& sd, int sz, float* res ) const
{
    const Pos::GeomID gid = Survey::GM().getGeomID( linenm );
    const TrcKey trckey( gid, trcnr );
    transformTrc( trckey, sd, sz, res );
}


float ZAxisTransform::transform2D( const char* linenm, int trcnr,
				   float z ) const
{
    float res = mUdf(float);
    transform2D( linenm, trcnr, SamplingData<float>(z,1), 1, &res );
    return res;
}


void ZAxisTransform::transformBack2D( const char* linenm, int trcnr,
		const SamplingData<float>& sd, int sz, float* res ) const
{
    const Pos::GeomID gid = Survey::GM().getGeomID( linenm );
    const TrcKey trckey( gid, trcnr );
    transformTrcBack( trckey, sd, sz, res );
}


float ZAxisTransform::transformBack2D( const char* linenm, int trcnr,
				     float z ) const
{
    float res = mUdf(float);
    transformBack2D( linenm, trcnr, SamplingData<float>(z,1), 1, &res );
    return res;
}


ZSampling ZAxisTransform::getModelZSampling() const
{
    return getZInterval( false );
}


ZSampling ZAxisTransform::getZInterval( const ZSampling& zsamp,
					const ZDomain::Info& from,
					const ZDomain::Info& to,
					bool makenice ) const
{
    ZSampling ret = getWorkZSampling( zsamp, from, to );
    if ( makenice && from != to )
    {
	const int userfac = to.def_.userFactor();
	float zstep = ret.step_;
	zstep = zstep<1e-3f ? 1.0f : mNINT32(zstep*userfac);
	zstep /= userfac;
	ret.step_ = zstep;

	const Interval<float>& rg = ret;
	const int startidx = rg.indexOnOrAfter( rg.start_, zstep );
	ret.start_ = zstep * mNINT32( rg.atIndex( startidx, zstep ) / zstep );
	const int stopidx = rg.indexOnOrAfter( rg.stop_, zstep );
	ret.stop_ = zstep * mNINT32( rg.atIndex( stopidx, zstep ) / zstep );
    }

    return ret;
}


ZSampling ZAxisTransform::getZInterval( bool isfrom, bool makenice,
						const ZSampling* zsamp ) const
{
    const ZSampling& samp = zsamp ? *zsamp : SI().zRange( true );
    if ( zsamp )
    {
	if ( isfrom )
	    return getZInterval( samp, fromZDomainInfo(), toZDomainInfo(),
								    makenice );
	else
	    return  getZInterval( samp, toZDomainInfo(), fromZDomainInfo(),
								    makenice );
    }

    const ZDomain::Info& from = SI().zDomainInfo();
    const ZDomain::Info& to = isfrom ? fromZDomainInfo() : toZDomainInfo();
    return getZInterval( samp, from, to, makenice );

}


ZDomain::Info& ZAxisTransform::fromZDomainInfo()
{
    if ( !datafromzdominfo_ || fromzdomaininfo_==*datafromzdominfo_ )
	return fromzdomaininfo_;

    return const_cast<ZDomain::Info&>(*datafromzdominfo_);
}


const ZDomain::Info& ZAxisTransform::fromZDomainInfo() const
{
    return getNonConst(*this).fromZDomainInfo();
}


ZDomain::Info& ZAxisTransform::toZDomainInfo()
{
    if ( !datatozdominfo_ || tozdomaininfo_==*datatozdominfo_ )
	return tozdomaininfo_;

    return const_cast<ZDomain::Info&>(*datatozdominfo_);
}


const ZDomain::Info& ZAxisTransform::toZDomainInfo() const
{
    return getNonConst(*this).toZDomainInfo();
}


const char* ZAxisTransform::fromZDomainKey() const
{
    return fromzdomaininfo_.key();
}


const char* ZAxisTransform::toZDomainKey() const
{
    return tozdomaininfo_.key();
}


void ZAxisTransform::setDataFromZDomainInfo( const ZDomain::Info& dfzdinf )
{
    datafromzdominfo_ = &dfzdinf;
}


void ZAxisTransform::setDataToZDomainInfo( const ZDomain::Info& dtzdinf )
{
    datatozdominfo_ = &dtzdinf;
}


const ZDomain::Info& ZAxisTransform::zDomain( bool from ) const
{
    const ZDomain::Info& zdom = from ? fromZDomainInfo()
				     : toZDomainInfo();
    return ZDomain::Info::getFrom( zdom );
}


void ZAxisTransform::fillPar( IOPar& par ) const
{
    par.set( sKey::Name(), factoryKeyword() );
}


float ZAxisTransform::toZScale() const
{
    if ( toZDomainInfo().isDepth() )
    {
	return SI().defaultXYtoZScale(
		SI().depthsInFeet() ? SurveyInfo::Feet : SurveyInfo::Meter,
		SI().xyUnit() );
    }
    else if ( toZDomainInfo().isTime() )
	return SI().defaultXYtoZScale( SurveyInfo::Second, SI().xyUnit() );

    return SI().zScale( false );
}


bool ZAxisTransform::usePar( const IOPar& par )
{
    MultiID mid;
    par.get( sKey::ID(), mid );
    if ( mid.isUdf() )
	par.get( IOPar::compKey(ZDomain::sKey(),sKey::ID()), mid );

    if ( mid.isUdf() )
	par.get( "ZDomain ID", mid );

    fromzdomaininfo_.setID( mid );
    tozdomaininfo_.setID( mid );
    return true;
}


ZAxisTransformSampler::ZAxisTransformSampler( const ZAxisTransform& trans,
					      bool b,
					      const SamplingData<double>& nsd,
					      bool is2d )
    : transform_(trans)
    , back_(b)
    , trckey_(Pos::IdxPair(0,0),is2d)
    , sd_(nsd)
    , is2d_(is2d)
{
    transform_.ref();
}


ZAxisTransformSampler::~ZAxisTransformSampler()
{
    transform_.unRef();
}


void ZAxisTransformSampler::setLineName( const char* lnm )
{
    if ( !is2d_ )
	return;

    trckey_.setGeomID( Survey::GM().getGeomID( lnm ) );
}


void ZAxisTransformSampler::setTrcNr( int trcnr )
{
    trckey_.setTrcNr(trcnr );
}


void ZAxisTransformSampler::setBinID( const BinID& bid )
{
    if ( is2d_ )
	{ pErrMsg("Incorrect usage"); }

    trckey_ = TrcKey( bid );
}


float ZAxisTransformSampler::operator[](int idx) const
{
    const int cachesz = cache_.size();
    if ( cachesz )
    {
	const int cacheidx = idx-firstcachesample_;
	if ( cacheidx>=0 && cacheidx<cachesz )
	    return cache_[cacheidx];
    }

    const SamplingData<float> sd( (float)sd_.atIndex(idx), 1 );

    float res = mUdf(float);
    if ( back_ )
	transform_.transformTrcBack( trckey_, sd, 1, &res );
    else
	transform_.transformTrc( trckey_, sd, 1, &res );

    return res;
}


void ZAxisTransformSampler::computeCache( const Interval<int>& range )
{
    const int sz = range.width()+1;
    cache_.setSize( sz );
    const SamplingData<float> cachesd( (float)sd_.atIndex(range.start_),
				       (float)sd_.step_ );
    if ( back_ )
	transform_.transformTrcBack( trckey_, cachesd, sz, cache_.arr() );
    else
	transform_.transformTrc( trckey_, cachesd, sz, cache_.arr() );

    firstcachesample_ = range.start_;
}
