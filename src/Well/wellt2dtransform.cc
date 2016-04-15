/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          July 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "wellt2dtransform.h"

#include "iopar.h"
#include "interpol1d.h"
#include "binidvalue.h"
#include "multiid.h"
#include "position.h"
#include "survinfo.h"
#include "welldata.h"
#include "wellman.h"
#include "welld2tmodel.h"
#include "welltrack.h"
#include "zdomain.h"
#include "keystrs.h"


WellT2DTransform::WellT2DTransform()
    : ZAxisTransform(ZDomain::Time(),ZDomain::Depth())
    , data_(0)
{
}


WellT2DTransform::WellT2DTransform( const MultiID& wllid )
    : ZAxisTransform(ZDomain::Time(),ZDomain::Depth())
    , data_(0)
{
    setWellID( wllid );
}


WellT2DTransform::~WellT2DTransform()
{
    if ( data_ )
	data_->unRef();
}


bool WellT2DTransform::isOK() const
{
    return data_ && tdmodel_.isOK();
}


bool WellT2DTransform::calcDepths()
{
    const Well::D2TModel* d2t = data_ ? data_->d2TModel() : 0;
    if ( !d2t )
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
	return SI().zRange(true).step;

    const Interval<float> zrg = getZInterval( false );
    const int nrsamples = SI().zRange( false ).nrSteps();
    return zrg.width() / (nrsamples==0 ? 1 : nrsamples);
}


Interval<float> WellT2DTransform::getZInterval( bool time ) const
{
    Interval<float> zrg = SI().zRange( true );
    const bool survistime = SI().zIsTime();
    if ( time && survistime ) return zrg;

    const BinIDValue startbidval( 0, 0, zrg.start );
    const BinIDValue stopbidval( 0, 0, zrg.stop );
    if ( survistime && !time )
    {
	zrg.start = ZAxisTransform::transform( startbidval );
	zrg.stop = ZAxisTransform::transform( stopbidval );
    }
    else if ( !survistime && time )
    {
	zrg.start = ZAxisTransform::transformBack( startbidval );
	zrg.stop = ZAxisTransform::transformBack( stopbidval );
    }

    return zrg;
}


bool WellT2DTransform::setWellID( const MultiID& mid )
{
    tozdomaininfo_.pars_.set( sKey::ID(), mid );

    data_ = Well::MGR().get( mid );
    if ( !data_ )
    {
	errmsg_ = tr("Z Transform: Cannot find Well with ID %1")
		    .arg( tozdomaininfo_.getID() );

	return false;
    }

    data_->ref();
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

    if ( !setWellID(MultiID(tozdomaininfo_.getID() ) ) )
	return false;

    return true;
}
