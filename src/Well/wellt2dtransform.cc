
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          July 2010
 RCS:           $Id: wellt2dtransform.cc,v 1.3 2010/11/30 16:48:16 cvskris Exp $
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
    const Well::D2TModel* wllmodel = data_ ? data_->d2TModel() : 0;
    if ( !wllmodel )
	return false;

    const Well::Track& track = data_->track();
    const int modelsz = wllmodel->size();
    const float time0 = wllmodel->t( 0 );
    const float dah0 = wllmodel->getDah( time0 );
    times_ += time0; depths_ += dah0;
    float vertdepth = dah0;
    for ( int idx=0; idx<=modelsz-2; idx++ )
    {
	const float prevtime = wllmodel->t( idx );
	const float dah1 = wllmodel->getDah( prevtime );
	const Coord3 prevcrd = track.getPos( dah1 );
	const float nexttime = wllmodel->t( idx+1 );
	times_ += nexttime;
	const float dah2 = wllmodel->getDah( nexttime );
	const Coord3 nextcrd = track.getPos( dah2 );
	const float hyp = dah2 - dah1;
	const float dist = sqrt( ((prevcrd.x-nextcrd.x)*(prevcrd.x-nextcrd.x)) +
			   ((prevcrd.y-nextcrd.y)*(prevcrd.y-nextcrd.y)) ); 
	vertdepth += sqrt( (hyp*hyp) - (dist*dist) );
	depths_ += vertdepth;
    }
    
    return true;
}


void WellT2DTransform::transform( const BinID& bid,
					const SamplingData<float>& sd,
					int sz, float* res ) const
{
    if ( !sz )
	return;

    for ( int idx=0; idx<sz; idx++ )
    {
	float time = sd.start + idx*sd.step;
	for ( int didx=0; didx<=times_.size()-2; didx++ )
	{
	    if ( time>times_[didx] && time<times_[didx+1] )
	    {
		res[idx] = Interpolate::linear1D( times_[didx], depths_[didx],
						  times_[didx+1], depths_[didx],
						  time );
		break;
	    }
	}
    }
}



void WellT2DTransform::transformBack( const BinID& bid,
					    const SamplingData<float>& sd,
					    int sz, float* res ) const
{
    for ( int idx=0; idx<sz; idx++ )
    {
	float depth = sd.start + idx*sd.step;
	for ( int didx=0; didx<=times_.size()-2; didx++ )
	{
	    if ( depth>depths_[didx] && depth<depths_[didx+1] )
	    {
		res[idx] = Interpolate::linear1D( depths_[didx], times_[didx],
						  depths_[didx+1], times_[didx],
						  depth );
		break;
	    }
	}
    }
}


Interval<float> WellT2DTransform::getZInterval( bool time ) const
{
    Interval<float> zrg( 0, 0 );
    zrg.start = depths_[0];
    zrg.stop = depths_[ depths_.size()-1];

    return zrg;
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

    calcDepths();
    return true;;
}
