/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 2005
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "zaxistransform.h"

#include "iopar.h"
#include "keystrs.h"
#include "survinfo.h"
#include "zdomain.h"


mImplFactory( ZAxisTransform, ZAxisTransform::factory );


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


int ZAxisTransform::addVolumeOfInterest( const CubeSampling&, bool )
{ return -1; }


void ZAxisTransform::setVolumeOfInterest( int, const CubeSampling&, bool )
{}

int ZAxisTransform::addVolumeOfInterest2D( const char*, const CubeSampling&, bool)
{ return -1; }

void ZAxisTransform::setVolumeOfInterest2D( int, const char*, const CubeSampling&,
					    bool )
{}


void ZAxisTransform::removeVolumeOfInterest( int ) {}


bool ZAxisTransform::loadDataIfMissing(int,TaskRunner*)
{ return !needsVolumeOfInterest(); }


float ZAxisTransform::transform( const Coord3& pos ) const
{ return transform( BinIDValue(SI().transform(pos),(float) pos.z) ); }


float ZAxisTransform::transform( const BinIDValue& pos ) const
{
    float res = mUdf(float);
    transform( pos.binid, SamplingData<float>(pos.value,1), 1, &res );
    return res;
}


float ZAxisTransform::transformBack( const Coord3& pos ) const
{ return transformBack( BinIDValue(SI().transform(pos),(float) pos.z) ); }


float ZAxisTransform::transformBack( const BinIDValue& pos ) const
{
    float res = mUdf(float);
    transformBack( pos.binid, SamplingData<float>(pos.value,1), 1, &res );
    return res;
}


void ZAxisTransform::transform2D( const char* linenm, int trcnr,
		const SamplingData<float>& sd, int sz, float* res ) const
{
    transform( BinID(lineIndex(linenm),trcnr), sd, sz, res );
}


float ZAxisTransform::transform2D( const char* linenm, int trcnr, float z ) const
{
    float res = mUdf(float);
    transform2D( linenm, trcnr, SamplingData<float>(z,1), 1, &res );
    return res;
}


void ZAxisTransform::transformBack2D( const char* linenm, int trcnr,
		const SamplingData<float>& sd, int sz, float* res ) const
{
    transformBack( BinID(lineIndex(linenm),trcnr), sd, sz, res );
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
    return SI().zRange(true).step;
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


float ZAxisTransform::zScale() const
{ return SI().zScale(); }


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
    , bid_(0,0)
    , sd_(nsd)
    , is2d_(is2d)
{ transform_.ref(); }


ZAxisTransformSampler::~ZAxisTransformSampler()
{ transform_.unRef(); }


void ZAxisTransformSampler::setLineName( const char* lnm )
{
    if ( !is2d_ )
	return;
    bid_.inl = transform_.lineIndex(lnm);
    curlinenm_ = lnm;
}

void ZAxisTransformSampler::setTrcNr( int trcnr )
{ bid_.crl = trcnr; }


float ZAxisTransformSampler::operator[](int idx) const
{
    const int cachesz = cache_.size();
    if ( cachesz )
    {
	const int cacheidx = idx-firstcachesample_;
	if ( cacheidx>=0 && cacheidx<cachesz )
	    return cache_[cacheidx];
    }

    const BinIDValue bidval( BinIDValue(bid_,(float) sd_.atIndex(idx)) );
    return back_ ? ( is2d_ ? transform_.transformBack2D(curlinenm_,bid_.crl,
						      bidval.value)
			   : transform_.transformBack(bidval) )
	         : ( is2d_ ? transform_.transform2D(curlinenm_,bid_.crl,
			     			  bidval.value)
			   : transform_.transform(bidval) );
}


void ZAxisTransformSampler::computeCache( const Interval<int>& range )
{
    const int sz = range.width()+1;
    cache_.setSize( sz );
    const SamplingData<float> cachesd( (float)sd_.atIndex(range.start),
					(float)sd_.step );
    if ( back_ )
    {
	if ( is2d_ )
	    transform_.transformBack2D( curlinenm_, bid_.crl, cachesd,
				      sz, cache_.arr() );
	else
	    transform_.transformBack( bid_, cachesd, sz, cache_.arr() );
    }
    else
    {
	if ( is2d_ )
	    transform_.transform2D( curlinenm_, bid_.crl, cachesd,
				  sz, cache_.arr() );
	else
	    transform_.transform( bid_, cachesd, sz, cache_.arr() );
    }

    firstcachesample_ = range.start;
}
