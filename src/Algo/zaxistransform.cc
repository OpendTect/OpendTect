/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 2005
-*/

static const char* rcsID = "$Id: zaxistransform.cc,v 1.20 2010-07-06 17:28:49 cvsnanne Exp $";

#include "zaxistransform.h"

#include "iopar.h"
#include "keystrs.h"
#include "survinfo.h"
#include "zdomain.h"


mImplFactory( ZAxisTransform, ZATF );


ZAxisTransform* ZAxisTransform::create( const IOPar& par )
{
    const FixedString str = par.find( sKey::Name );
    if ( !str )
	return 0;

    ZAxisTransform* res = ZATF().create( str );
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

    

    
    

ZAxisTransform::ZAxisTransform()
{}


ZAxisTransform::~ZAxisTransform()
{ }


int ZAxisTransform::addVolumeOfInterest( const CubeSampling&, bool )
{ return -1; }


void ZAxisTransform::setVolumeOfInterest( int, const CubeSampling&, bool )
{}

int ZAxisTransform::addVolumeOfInterest( const char*, const CubeSampling&, bool)
{ return -1; }

void ZAxisTransform::setVolumeOfInterest( int, const char*, const CubeSampling&,					  bool )
{}


void ZAxisTransform::removeVolumeOfInterest( int ) {}


const char* ZAxisTransform::getToZDomainUnit( bool ) const
{ return sKey::EmptyString; }


void ZAxisTransform::getToZDomainInfo( ZDomain::Info& info ) const
{
    info.zfactor_ = getZFactor();
    info.name_ = getToZDomainString();
    info.id_ = getZDomainID();
    info.unitstr_ = getToZDomainUnit( false );
}


bool ZAxisTransform::loadDataIfMissing(int,TaskRunner*)
{ return !needsVolumeOfInterest(); }


float ZAxisTransform::transform( const Coord3& pos ) const
{ return transform( BinIDValue( SI().transform(pos), pos.z ) ); }


float ZAxisTransform::transform( const BinIDValue& pos ) const
{
    float res = mUdf(float);
    transform( pos.binid, SamplingData<float>(pos.value,1), 1, &res );
    return res;
}


float ZAxisTransform::transformBack( const Coord3& pos ) const
{ return transformBack( BinIDValue( SI().transform(pos), pos.z ) ); }


float ZAxisTransform::transformBack( const BinIDValue& pos ) const
{
    float res = mUdf(float);
    transformBack( pos.binid, SamplingData<float>(pos.value,1), 1, &res );
    return res;
}


void ZAxisTransform::transform( const char* linenm, int trcnr,
		const SamplingData<float>& sd, int sz, float* res ) const
{
    transform( BinID(lineIndex(linenm),trcnr), sd, sz, res );
}


float ZAxisTransform::transform( const char* linenm, int trcnr, float z ) const
{
    float res = mUdf(float);
    transform( linenm, trcnr, SamplingData<float>(z,1), 1, &res );
    return res;
}


void ZAxisTransform::transformBack( const char* linenm, int trcnr,
		const SamplingData<float>& sd, int sz, float* res ) const
{
    transformBack( BinID(lineIndex(linenm),trcnr), sd, sz, res );
}


float ZAxisTransform::transformBack( const char* linenm, int trcnr,
				     float z ) const
{
    float res = mUdf(float);
    transformBack( linenm, trcnr, SamplingData<float>(z,1), 1, &res );
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


const char* ZAxisTransform::getFromZDomainString() const
{ return SI().getZDomainString(); }


void ZAxisTransform::fillPar( IOPar& par ) const
{
    par.set( sKey::Name, name() );
}


ZAxisTransformSampler::ZAxisTransformSampler( const ZAxisTransform& trans,
					      bool b,
					      const SamplingData<double>& nsd )
    : transform_(trans)
    , back_(b)
    , bid_(0,0)
    , sd_(nsd)
{ transform_.ref(); }


ZAxisTransformSampler::~ZAxisTransformSampler()
{ transform_.unRef(); }


void ZAxisTransformSampler::setLineName( const char* lnm )
{ bid_.inl = transform_.lineIndex(lnm); }

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

    const BinIDValue bidval( BinIDValue(bid_,sd_.atIndex(idx)) );
    return back_ ? transform_.transformBack( bidval )
	         : transform_.transform( bidval );
}


void ZAxisTransformSampler::computeCache( const Interval<int>& range )
{
    const int sz = range.width()+1;
    cache_.setSize( sz );
    const SamplingData<float> cachesd( sd_.atIndex(range.start), sd_.step );
    if ( back_ ) transform_.transformBack( bid_, cachesd, sz, cache_.arr() );
    else transform_.transform( bid_, cachesd, sz, cache_.arr() );
    firstcachesample_ = range.start;
}
