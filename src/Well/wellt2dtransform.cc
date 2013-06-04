
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
#include "multiid.h"
#include "position.h"
#include "welldata.h"
#include "wellman.h"
#include "welld2tmodel.h"
#include "welltrack.h"
#include "zdomain.h"

#include <cmath>

WellT2DTransform::WellT2DTransform()
    : ZAxisTransform(ZDomain::Time(),ZDomain::Depth())
    , data_(0)
{
}


bool WellT2DTransform::calcDepths()
{
    const Well::D2TModel* d2t = data_ ? data_->d2TModel() : 0;
    if ( !d2t )
	return false;

    const Well::Track& track = data_->track();
    const int d2tsz = d2t->size();
    float dah0 = mUdf(float);
    float time0 = 0; int idx0 = 0;
    for ( ; idx0<d2tsz && mIsUdf(dah0); idx0++ )
    {
	time0 = d2t->t( idx0 );
	dah0 = d2t->getDah( time0, track );
    }
    if ( mIsUdf(dah0) )
    {
	errmsg_ = "Z Transform: Well Depth to time model has no valid points";
	return false;
    }

    times_ += time0; depths_ += dah0;
    float vertdepth = dah0;
    for ( int idx=idx0; idx<d2tsz-1; idx++ )
    {
	const float prevtime = d2t->t( idx );
	const float nexttime = d2t->t( idx+1 );
	const float dah1 = d2t->getDah( prevtime, track );
	const float dah2 = d2t->getDah( nexttime, track );
	if ( mIsUdf(dah1) || mIsUdf(dah2) )
	    continue;

	const Coord3 prevcrd = track.getPos( dah1 );
	const Coord3 nextcrd = track.getPos( dah2 );
	const float hyp = dah2 - dah1;
	const Coord dc( prevcrd.x-nextcrd.x, prevcrd.y-nextcrd.y );
	const double hdist = (float) Math::Sqrt( dc.x*dc.x + dc.y*dc.y );
	vertdepth += (float)Math::Sqrt( (hyp*hyp) - (hdist*hdist) );

	times_ += nexttime;
	depths_ += vertdepth;
    }

    if ( times_.size() < 2 )
    {
	times_ += 2.f * times_[0];
	depths_ += 2.f * depths_[0];
    }
    return true;
}


void WellT2DTransform::doTransform( const BinID& bid,
					const SamplingData<float>& sd,
					int ressz, float* res, bool back ) const
{
    const int possz = times_.size();
    if ( possz < 2 )
    {
	for ( int idx=0; idx<ressz; idx++ )
	    res[idx] = (float)idx;
	return;
    }

    const TypeSet<float>& frompos = back ? depths_ : times_;
    const TypeSet<float>& topos =  !back ? depths_ : times_;

    for ( int residx=0; residx<ressz; residx++ )
    {
	const float curfrom = sd.start + residx*sd.step;
	if ( curfrom <= frompos[0] )
	{
	    const float slope = (topos[1]-topos[0]) / (frompos[1]-frompos[0]);
	    res[residx] = topos[0] - slope * (frompos[0] - curfrom);
	}
	else if ( curfrom >= frompos[possz-1] )
	{
	    const float slope = (topos[possz-1] - topos[possz-2])
			    / (frompos[possz-1] - frompos[possz-2]);
	    res[residx] = topos[possz-1] + slope * (curfrom - frompos[possz-1]);
	}
	else
	{
	    for ( int pidx=0; pidx<possz-1; pidx++ )
	    {
		if ( curfrom>=frompos[pidx] && curfrom<frompos[pidx+1] )
		{
		    res[residx] = Interpolate::linear1D(
				    frompos[pidx], topos[pidx],
				    frompos[pidx+1], topos[pidx+1],
				    curfrom );
		    break;
		}
	    }
	}
    }
}


void WellT2DTransform::transform( const BinID& bid,
					const SamplingData<float>& sd,
					int ressz, float* res ) const
{
    doTransform( bid, sd, ressz, res, false );
}



void WellT2DTransform::transformBack( const BinID& bid,
					    const SamplingData<float>& sd,
					    int ressz, float* res ) const
{
    doTransform( bid, sd, ressz, res, true );
}


Interval<float> WellT2DTransform::getZInterval( bool time ) const
{
    const int sz = times_.size();
    if ( sz < 1 )
	return Interval<float>(0,0);

    return Interval<float>( time ? times_[0] : depths_[0],
	    		    time ? times_[sz-1] : depths_[sz-1] );
}


bool WellT2DTransform::usePar( const IOPar& iop )
{
    if ( !ZAxisTransform::usePar(iop) )
	return false;
    if ( !tozdomaininfo_.hasID() )
	{ errmsg_ = "Z Transform: No ID for Well provided"; return false; }

    data_ = Well::MGR().get( MultiID(tozdomaininfo_.getID()) );
    if ( !data_ )
    {
	errmsg_ = "Z Transform: Cannot find Well with ID ";
	errmsg_ += tozdomaininfo_.getID();
	return false;
    }

    return calcDepths();
}
