/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellt2dtransform.h"

#include "iopar.h"
#include "interpol1d.h"
#include "binidvalue.h"
#include "keystrs.h"
#include "multiid.h"
#include "odmemory.h"
#include "position.h"
#include "survinfo.h"
#include "wellman.h"
#include "welld2tmodel.h"
#include "welltrack.h"
#include "zdomain.h"
#include "zvalseriesimpl.h"


WellT2DTransform::WellT2DTransform()
    : ZAxisTransform(ZDomain::Time(),ZDomain::Depth())
{
}


WellT2DTransform::WellT2DTransform( const MultiID& wllid )
    : ZAxisTransform(ZDomain::Time(),ZDomain::Depth())
{
    setWellID( wllid );
}


WellT2DTransform::~WellT2DTransform()
{
}


bool WellT2DTransform::isOK() const
{
    if ( !data_ || !tdmodel_.isOK() )
	return false;

    return ZAxisTransform::isOK();
}


void WellT2DTransform::doTransform( const SamplingData<float>&,
				    int, float*, bool ) const
{
}


float WellT2DTransform::getGoodZStep() const
{
    return getZInterval( false, true ).step;
}


Interval<float> WellT2DTransform::getZInterval( bool from ) const
{
    return getZInterval( from, true );
}


Interval<float> WellT2DTransform::getZRange( bool ) const
{
    return Interval<float>::udf();
}


bool WellT2DTransform::setWellID( const MultiID& mid )
{
    tozdomaininfo_.pars_.set( sKey::ID(), mid );

    Well::LoadReqs lreqs( Well::D2T );
    data_ = Well::MGR().get( mid, lreqs );
    if ( !data_ )
    {
	errmsg_ = tr("Z Transform: Cannot find Well with ID %1")
		    .arg( tozdomaininfo_.getID() );

	return false;
    }

    return calcDepths();
}


void WellT2DTransform::fillPar( IOPar& par ) const
{
    ZAxisTransform::fillPar( par );
    if ( data_ )
	par.set( sKey::ID(), data_->multiID() );
}


bool WellT2DTransform::usePar( const IOPar& par )
{
    if ( !ZAxisTransform::usePar(par) )
	return false;

    if ( !tozdomaininfo_.hasID() )
	{ errmsg_ = tr("Z Transform: No ID for Well provided"); return false; }

    if ( !setWellID(tozdomaininfo_.getID()) )
	return false;

    return true;
}


bool WellT2DTransform::calcDepths()
{
    const Well::D2TModel* d2t = data_ ? data_->d2TModel() : nullptr;
    if ( !d2t )
	return false;

    return d2t->getTimeDepthModel( *data_, tdmodel_ );
}


void WellT2DTransform::transformTrc( const TrcKey&,
				     const SamplingData<float>& sd,
				     int sz, float* res ) const
{
    doTransform( sd, fromZDomainInfo(), sz, res );
}



void WellT2DTransform::transformTrcBack( const TrcKey&,
					 const SamplingData<float>& sd,
					 int sz, float* res ) const
{
    doTransform( sd, toZDomainInfo(), sz, res );
}


void WellT2DTransform::doTransform( const SamplingData<float>& sd,
				    const ZDomain::Info& sdzinfo,
				    int sz, float* res ) const
{
     if ( sd.isUdf() )
     {
	 OD::sysMemValueSet( res, mUdf(float), sz );
	 return;
     }

     const RegularZValues zvals( sd, sz, sdzinfo );
     if ( zvals.isTime() )
	 for ( od_int64 idx=0; idx<sz; idx++ )
	     res[idx] = tdmodel_.getDepth( float (zvals[idx]) );
     else
	 for ( od_int64 idx=0; idx<sz; idx++ )
	     res[idx] = tdmodel_.getTime( float (zvals[idx]) );
}


ZSampling WellT2DTransform::getZInterval( const ZSampling& zsamp,
					  const ZDomain::Info& from,
					  const ZDomain::Info& to,
					  bool makenice ) const
{
    ZSampling ret = getWorkZSampling( zsamp, from, to );
    if ( makenice && from != to )
    {
	const int userfac = to.def_.userFactor();
	float zstep = ret.step;
	zstep = zstep<1e-3f ? 1.0f : mNINT32(zstep*userfac);
	zstep /= userfac;
	ret.step = zstep;

	const Interval<float>& rg = ret;
	const int startidx = rg.indexOnOrAfter( rg.start, zstep );
	ret.start = zstep * mNINT32( rg.atIndex( startidx, zstep ) / zstep );
	const int stopidx = rg.indexOnOrAfter( rg.stop, zstep );
	ret.stop = zstep * mNINT32( rg.atIndex( stopidx, zstep ) / zstep );
    }

    return ret;
}


ZSampling WellT2DTransform::getZInterval( bool isfrom, bool makenice ) const
{
    const ZDomain::Info& from = SI().zDomainInfo();
    const ZDomain::Info& to = isfrom ? fromZDomainInfo() : toZDomainInfo();
    return getZInterval( SI().zRange(true), from, to, makenice );
}


ZSampling WellT2DTransform::getWorkZSampling( const ZSampling& zsamp,
					      const ZDomain::Info& from,
					      const ZDomain::Info& to ) const
{
    if ( !isOK() )
	return ZSampling::udf();

    const int nrsamples = zsamp.nrSteps();
    ZSampling ret = zsamp;
    if ( from.isTime() && to.isDepth() )
    {
	ret.start = tdmodel_.getDepth( ret.start );
	ret.stop = tdmodel_.getDepth( ret.stop );
    }
    else if ( from.isDepth() && to.isTime() )
    {
	ret.start = tdmodel_.getTime( ret.start );
	ret.stop = tdmodel_.getTime( ret.stop );
    }

    if ( to != from )
	ret.step = (ret.width()) / (nrsamples==0 ? 1 : nrsamples);

    return ret;
}
