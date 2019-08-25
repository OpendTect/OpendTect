/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          July 2010
________________________________________________________________________

-*/

#include "wellt2dtransform.h"

#include "iopar.h"
#include "interpol1d.h"
#include "binidvalue.h"
#include "dbkey.h"
#include "keystrs.h"
#include "position.h"
#include "survinfo.h"
#include "welldata.h"
#include "wellmanager.h"
#include "welld2tmodel.h"
#include "welltrack.h"
#include "trckey.h"
#include "zdomain.h"


WellT2DTransform::WellT2DTransform()
    : ZAxisTransform(ZDomain::Time(),ZDomain::Depth())
{
}


WellT2DTransform::WellT2DTransform( const DBKey& wllid )
    : ZAxisTransform(ZDomain::Time(),ZDomain::Depth())
{
    setWellID( wllid );
}


bool WellT2DTransform::isOK() const
{
    return data_ && tdmodel_.isOK();
}


bool WellT2DTransform::calcDepths()
{
    const Well::D2TModel* d2t = data_ ? &data_->d2TModel() : 0;
    if ( !d2t || d2t->isEmpty() )
	return false;

    return d2t->getTimeDepthModel( *data_, tdmodel_ );
}


void WellT2DTransform::doTransform( const SamplingData<float>& sd,
				    int ressz, float* res, bool back ) const
{
    const bool survistime = SI().zIsTime();
    const bool depthsinfeet = SI().depthsInFeet();

    for ( int idx=0; idx<ressz; idx++ )
    {
	float inp = sd.atIndex( idx );
	if ( back )
	{
	    if ( survistime && depthsinfeet ) inp *= mFromFeetFactorF;
	    res[idx] = tdmodel_.getTime( inp );
	}
	else
	{
	    res[idx] = tdmodel_.getDepth( inp );
	    if ( survistime && depthsinfeet ) res[idx] *= mToFeetFactorF;
	}
    }
}


void WellT2DTransform::transformTrc( const TrcKey&,
				     const SamplingData<float>& sd,
				     int ressz, float* res ) const
{
    doTransform( sd, ressz, res, false );
}



void WellT2DTransform::transformTrcBack( const TrcKey&,
					 const SamplingData<float>& sd,
					 int ressz, float* res ) const
{
    doTransform( sd, ressz, res, true );
}


float WellT2DTransform::getGoodZStep() const
{
    if ( !SI().zIsTime() )
	return SI().zStep();

    const Interval<float> zrg = getZRange( false );
    const int userfac = toZDomainInfo().userFactor();
    const int nrsteps = SI().zRange().nrSteps();
    float zstep = zrg.width() / (nrsteps==0 ? 1 : nrsteps);
    zstep = zstep<1e-3f ? 1.0f : mNINT32(zstep*userfac);
    zstep /= userfac;
    return zstep;
}


Interval<float> WellT2DTransform::getZInterval( bool time ) const
{
    Interval<float> zrg = getZRange( time );
    const float step = getGoodZStep();
    const int userfac = toZDomainInfo().userFactor();
    const int stopidx = zrg.indexOnOrAfter( zrg.stop, step );
    zrg.stop = zrg.atIndex( stopidx, step );
    zrg.stop = mCast(float,mNINT32(zrg.stop*userfac))/userfac;
    return zrg;
}


Interval<float> WellT2DTransform::getZRange( bool time ) const
{
    Interval<float> zrg = SI().zRange();
    const bool survistime = SI().zIsTime();
    if ( time && survistime )
	return zrg;

    const TrcKey tk( BinID(0,0) );
    if ( survistime && !time )
    {
	zrg.start = ZAxisTransform::transformTrc( tk, zrg.start );
	zrg.stop = ZAxisTransform::transformTrc( tk, zrg.stop );
    }
    else if ( !survistime && time )
    {
	zrg.start = ZAxisTransform::transformTrcBack( tk, zrg.start );
	zrg.stop = ZAxisTransform::transformTrcBack( tk, zrg.stop );
    }

    return zrg;
}


bool WellT2DTransform::setWellID( const DBKey& mid )
{
    tozdomaininfo_.pars_.set( sKey::ID(), mid );

    uiRetVal uirv;
    data_ = Well::MGR().fetch( mid, Well::LoadReqs(Well::Trck).add(Well::D2T),
				uirv );
    if ( !data_ )
	{ errmsg_ = uirv; return false; }

    return calcDepths();
}


void WellT2DTransform::fillPar( IOPar& par ) const
{
    ZAxisTransform::fillPar( par );
    if ( data_ )
	par.set( sKey::ID(), data_->dbKey() );
}


bool WellT2DTransform::usePar( const IOPar& par )
{
    if ( !ZAxisTransform::usePar(par) )
	return false;

    if ( !tozdomaininfo_.hasID() )
	{ errmsg_ = tr("Z Transform: No ID for Well provided"); return false; }

    if ( !setWellID( DBKey(tozdomaininfo_.getID()) ) )
	return false;

    return true;
}
