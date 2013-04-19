/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "welld2tmodel.h"
#include "welltrack.h"
#include "idxable.h"
#include "iopar.h"
#include "stratlevel.h"
#include "tabledef.h"
#include "velocitycalc.h"

const char* Well::D2TModel::sKeyTimeWell()	{ return "=Time"; }
const char* Well::D2TModel::sKeyDataSrc()	{ return "Data source"; }


Well::D2TModel& Well::D2TModel::operator =( const Well::D2TModel& d2t )
{
    if ( &d2t != this )
    {
	setName( d2t.name() );
	desc = d2t.desc; datasource = d2t.datasource;
	dah_ = d2t.dah_; t_ = d2t.t_;
    }
    return *this;
}


float Well::D2TModel::getTime( float dh, const Track& track ) const
{
    float twt = mUdf(float);
    Interval<double> depths( mUdf(double), mUdf(double) );
    Interval<float> times( mUdf(float), mUdf(float) );

    if ( !getVelocityBounds(dh,track,depths,times) )
	return twt;

    double reqz = track.getPos(dh).z;
    const double curvel = getVelocity( dh, track );
    twt = times.start + 2.f* (float)( ( reqz - depths.start ) / curvel );

    return twt;
}


double Well::D2TModel::getVelocity( float dh, const Track& track ) const
{
    double velocity = mUdf(double);
    Interval<double> depths( mUdf(double), mUdf(double) );
    Interval<float> times( mUdf(float), mUdf(float) );

    if ( !getVelocityBounds(dh,track,depths,times) )
	return velocity;

    velocity = 2.f * depths.width() / (double)times.width();

    return velocity;
}


bool Well::D2TModel::getVelocityBounds( float dh, const Track& track,
					Interval<double>& depths,
					Interval<float>& times ) const
{
    const int dtsize = size();
    int idah = IdxAble::getLowIdx(dah_,dtsize,dh);

    if ( idah < 0 )
	idah = 0;
    if ( idah >= (dtsize-1) )
	idah = getDahIndex( dh, track);
		
    depths.start = track.getPos(dah_[idah]).z;
    depths.stop = track.getPos(dah_[idah+1]).z;
    times.start = t_[idah];
    times.stop = t_[idah+1];

    bool reversedz = times.isRev() || depths.isRev();
    bool sametwt = mIsZero(times.width(),1e-6) || mIsZero(depths.width(),1e-6);

    if ( reversedz || sametwt )
	return getOldVelocityBounds(dh,track,depths,times);

    return true;
}


int Well::D2TModel::getDahIndex( float dh, const Track& track) const
{
    const int dtsize = size();
    double reqdh, dhtop, dhbase;
    int idah = 0;
    idah = IdxAble::getLowIdx(dah_,dtsize,dh);
    if ( idah >= (dtsize-1) )
    {
	if ( track.getPos(dah_[idah]).z > track.getPos(dh).z )
	{
	    double reqz = track.getPos( dh ).z;
	    for ( int idx=0; idx<track.nrPoints(); idx++ )
	    {
		if ( track.pos(idx).z > reqz )
		{
		    dhtop = mCast( double, track.dah( idx-1 ) );
		    dhbase = mCast( double, track.dah( idx ) );
		    reqdh = dhtop + ( (dhbase-dhtop)*(reqz-track.pos(idx-1).z) /
			    	    (track.pos(idx).z-track.pos(idx-1).z) );
		    idah = IdxAble::getLowIdx( dah_, dtsize, reqdh );
		    break;
		}
	    }
	}
    }
	
    if ( idah < 0 )
	idah = 0;

    if ( idah >= (dtsize-1) )
	idah = dtsize-2; 

    return idah;
}


bool Well::D2TModel::getOldVelocityBounds( float dh, const Track& track,
					   Interval<double>& depths,
					   Interval<float>& times ) const
{
    const int dtsize = size();
    int idah = IdxAble::getLowIdx( dah_, dtsize, dh );
    if ( idah < 0 )
	idah = 0;

    if ( idah >= (dtsize-1) )
	idah = getDahIndex( dh, track );
		
    depths.start = track.getPos( dah_[idah] ).z;
    times.start = t_[idah];
    depths.stop = mUdf(double);
    times.stop = mUdf(float);

    for ( int idx=idah+1; idx<dtsize; idx++ )
    {
	const double curz = track.getPos( dah_[idx] ).z;
	const float curtwt = t_[idx];
	if ( curz > depths.start && curtwt > times.start )
	{
	    depths.stop = curz;
	    times.stop = curtwt;
	    break;
	}
    }

    if ( mIsUdf(times.stop) ) // found nothing good below, search above
    {
	depths.stop = depths.start;
	times.stop = times.start;
	depths.start = mUdf(double);
	times.start = mUdf(float);
	for ( int idx=idah-1; idx>=0; idx-- )
	{
	    const double curz = track.getPos( dah_[idx] ).z;
	    const float curtwt = t_[idx];
	    if ( curz < depths.stop && curtwt < times.stop )
	    {
		depths.start = curz;
		times.start = curtwt;
		break;
	    }
	}
	if ( mIsUdf(times.start) )
	    return false; // could not get a single good velocity
    }

    return true;
}


float Well::D2TModel::getDah( float time, const Track& track ) const
{ return TimeDepthModel::getDepth( dah_.arr(), t_.arr(), size(), time ); }


bool Well::D2TModel::insertAtDah( float dh, float val )
{
    mWellDahObjInsertAtDah( dh, val, t_, true  );
    return true;
}
