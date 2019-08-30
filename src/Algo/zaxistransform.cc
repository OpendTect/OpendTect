/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 2005
-*/


#include "zaxistransform.h"

#include "iopar.h"
#include "keystrs.h"
#include "survinfo.h"
#include "binidvalue.h"
#include "zdomain.h"
#include "survgeom2d.h"
#include "trckey.h"


mImplClassFactory( ZAxisTransform, factory );


ZAxisTransform* ZAxisTransform::create( const IOPar& par )
{
    const FixedString str = par.find( sKey::Name() );
    if ( !str )
	return 0;

    ZAxisTransform* res = factory().create( str );
    if ( !res )
	return 0;

    res->ref();
    if ( !res->usePar( par ) )
    {
	res->unRef();
	return 0;
    }

    return res;
}


ZAxisTransform::ZAxisTransform( const ZDomain::Def& from,
				const ZDomain::Def& to )
    : fromzdomaininfo_(*new ZDomain::Info(from))
    , tozdomaininfo_(*new ZDomain::Info(to))
{}


ZAxisTransform::~ZAxisTransform()
{
    delete &fromzdomaininfo_;
    delete &tozdomaininfo_;
}


int ZAxisTransform::addVolumeOfInterest( const TrcKeyZSampling&, bool )
{ return -1; }

void ZAxisTransform::setVolumeOfInterest( int, const TrcKeyZSampling&, bool )
{}

void ZAxisTransform::removeVolumeOfInterest( int ) {}

bool ZAxisTransform::loadDataIfMissing( int, const TaskRunnerProvider& )
{
    return !needsVolumeOfInterest();
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
    return transform(
	BinIDValue(SI().transform(Coord(pos.x_,pos.y_)),(float) pos.z_) );
}


float ZAxisTransform::transform( const BinIDValue& pos ) const
{
    float res = mUdf(float);
    transform( pos, SamplingData<float>(pos.val(),1), 1, &res );
    return res;
}


float ZAxisTransform::transformBack( const Coord3& pos ) const
{
    return transformBack(
	    BinIDValue(SI().transform(Coord(pos.x_,pos.y_)),(float) pos.z_) );
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
    const TrcKey trckey( SurvGeom::getGeomID(linenm), trcnr );
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
    const TrcKey trckey( SurvGeom::getGeomID(linenm), trcnr );
    transformTrcBack( trckey, sd, sz, res );
}


float ZAxisTransform::transformBack2D( const char* linenm, int trcnr,
				     float z ) const
{
    float res = mUdf(float);
    transformBack2D( linenm, trcnr, SamplingData<float>(z,1), 1, &res );
    return res;
}


float ZAxisTransform::getZIntervalCenter( bool from ) const
{
    const Interval<float> rg = getZInterval( from );
    if ( mIsUdf(rg.start) || mIsUdf(rg.stop) )
	return mUdf(float);

    return rg.center();
}


float ZAxisTransform::getGoodZStep() const
{
    return SI().zRange().step;
}


const ZDomain::Info& ZAxisTransform::fromZDomainInfo() const
{ return const_cast<ZAxisTransform*>(this)->fromZDomainInfo(); }

const ZDomain::Info& ZAxisTransform::toZDomainInfo() const
{ return const_cast<ZAxisTransform*>(this)->toZDomainInfo(); }

const char* ZAxisTransform::fromZDomainKey() const
{ return fromzdomaininfo_.key(); }

const char* ZAxisTransform::toZDomainKey() const
{ return tozdomaininfo_.key(); }


void ZAxisTransform::fillPar( IOPar& par ) const
{
    par.set( sKey::Name(), factoryKeyword() );
}


float ZAxisTransform::toZScale() const
{
    if ( toZDomainInfo().def_.isDepth() )
    {
	return SI().defaultXYtoZScale(
		SI().depthsInFeet() ? SurveyInfo::Feet : SurveyInfo::Meter,
		SI().xyUnit() );
    }
    else if (  toZDomainInfo().def_.isTime() )
    {
	return SI().defaultXYtoZScale( SurveyInfo::Second, SI().xyUnit() );
    }

    return SI().zScale();
}


bool ZAxisTransform::usePar( const IOPar& par )
{
    const char* res = par.find( sKey::ID() );
    if ( !res || !*res )
	res = par.find( IOPar::compKey(ZDomain::sKey(),sKey::ID()) );
    if ( !res || !*res )
	res = par.find( "ZDomain ID" );

    fromzdomaininfo_.setID( res );
    tozdomaininfo_.setID( res );
    return true;
}


ZAxisTransformSampler::ZAxisTransformSampler( const ZAxisTransform& trans,
					      bool b,
					      const SamplingData<double>& nsd,
					      bool is2d	)
    : transform_(trans)
    , back_(b)
    , trckey_(*new TrcKey(BinID(0,0)))
    , sd_(nsd)
    , is2d_(is2d)
{
    transform_.ref();
}


ZAxisTransformSampler::~ZAxisTransformSampler()
{
    transform_.unRef();
    delete &trckey_;
}


void ZAxisTransformSampler::setTrcNr( int trcnr )
{
    trckey_.setTrcNr( trcnr );
}


void ZAxisTransformSampler::setBinID( const BinID& bid )
{
    if ( is2d_ )
	trckey_.setPos( Pos::GeomID(bid.inl()), bid.crl() );
    else
	trckey_.setPos( bid );
}


void ZAxisTransformSampler::setGeomID( Pos::GeomID geomid )
{
    trckey_.setGeomID( geomid );
}


void ZAxisTransformSampler::setTrcKey( const TrcKey& tk )
{
    trckey_ = tk;
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
    const SamplingData<float> cachesd( (float)sd_.atIndex(range.start),
					(float)sd_.step );
    if ( back_ )
    {
	transform_.transformTrcBack( trckey_, cachesd, sz, cache_.arr() );
    }
    else
    {
	transform_.transformTrc( trckey_, cachesd, sz, cache_.arr() );
    }

    firstcachesample_ = range.start;
}
