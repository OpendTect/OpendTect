
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          July 2010
 RCS:           $Id: wellt2dtransform.cc,v 1.1 2010-07-15 10:08:01 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "wellt2dtransform.h"

#include "iopar.h"
#include "interpol1d.h"
#include "position.h"
#include "welldata.h"
#include "wellman.h"
#include "welld2tmodel.h"
#include "welltrack.h"
#include "zdomain.h"

#include <cmath>

const char* Well:: WellT2DTransform::sName()
{ return "WellT2D"; }


void Well::WellT2DTransform::initClass()
{ ZATF().addCreator( create, sName() ); }


ZAxisTransform* Well::WellT2DTransform::create()
{ return new WellT2DTransform; }


Well::WellT2DTransform::WellT2DTransform()
{
    mid_ = 0; data_ = 0;
}


void Well::WellT2DTransform::transform( const BinID& bid,
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


bool Well::WellT2DTransform::calcDepths()
{
    Well::D2TModel* wllmodel = data_->d2TModel();
    if ( !wllmodel )
	return false;

    Well::Track track = data_->track();
    const int modelsz = wllmodel->size();
    const float time0 = wllmodel->t( 0 );
    const float dah0 = wllmodel->getDepth( time0 );
    times_ += time0; depths_ += dah0;
    float vertdepth = dah0;
    for ( int idx=0; idx<=modelsz-2; idx++ )
    {
	const float prevtime = wllmodel->t( idx );
	const float dah1 = wllmodel->getDepth( prevtime );
	const Coord3 prevcrd = track.getPos( dah1 );
	const float nexttime = wllmodel->t( idx+1 );
	times_ += nexttime;
	const float dah2 = wllmodel->getDepth( nexttime );
	const Coord3 nextcrd = track.getPos( dah2 );
	const float hyp = dah2 - dah1;
	const float dist = sqrt( ((prevcrd.x-nextcrd.x)*(prevcrd.x-nextcrd.x)) +
			   ((prevcrd.y-nextcrd.y)*(prevcrd.y-nextcrd.y)) ); 
	vertdepth += sqrt( (hyp*hyp) - (dist*dist) );
	depths_ += vertdepth;
    }
    
    return true;
}


void Well::WellT2DTransform::transformBack( const BinID& bid,
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


Interval<float> Well::WellT2DTransform::getZInterval( bool time ) const
{
    Interval<float> zrg( 0, 0 );
    zrg.start = depths_[0];
    zrg.stop = depths_[ depths_.size()-1];

    return zrg;
}


const char* Well::WellT2DTransform::getToZDomainString() const
{ return ZDomain::sKeyDepth(); }


const char* Well::WellT2DTransform::getZDomainID() const
{ return ""; }


bool Well::WellT2DTransform::fillPar( IOPar& iop )
{ return true; }


bool Well::WellT2DTransform::usePar( const IOPar& iop )
{
    iop.get( "ID", mid_ );
    if ( !mid_ )
	return false;

    data_ = Well::MGR().get( mid_ );
    if ( !data_ )
	return false;

    calcDepths();

    return true;;
}
