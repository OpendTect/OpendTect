/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "welld2tmodel.h"

#include "idxable.h"
#include "iopar.h"
#include "mathfunc.h"
#include "stratlevel.h"
#include "survinfo.h"
#include "tabledef.h"
#include "velocitycalc.h"
#include "welldata.h"
#include "welltrack.h"

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


bool Well::D2TModel::operator ==( const Well::D2TModel& d2t ) const
{
    if ( &d2t == this )
	return true;

    if ( d2t.size() != size() )
	return false;

    for ( int idx=0; idx<size(); idx++ )
    {
	if ( !mIsEqual(d2t.dah(idx),dah_[idx],mDefEpsF) ||
	     !mIsEqual(d2t.t(idx),t_[idx],mDefEpsF) )
	    return false;
    }

    return true;
}


bool Well::D2TModel::operator !=( const Well::D2TModel& d2t ) const
{
    return !( d2t == *this );
}


bool Well::D2TModel::insertAtDah( float dh, float val )
{
    mWellDahObjInsertAtDah( dh, val, t_, true  );
    return true;
}


float Well::D2TModel::getTime( float dh, const Track& track ) const
{
    float twt = mUdf(float);
    Interval<double> depths( mUdf(double), mUdf(double) );
    Interval<float> times( mUdf(float), mUdf(float) );

    if ( !getVelocityBoundsForDah(dh,track,depths,times) )
	return twt;

    double reqz = track.getPos(dh).z;
    const double curvel = getVelocityForDah( dh, track );
    twt = times.start + 2.f* (float)( ( reqz - depths.start ) / curvel );

    return twt;
}


float Well::D2TModel::getDepth( float twt, const Track& track ) const
{
    Interval<float> times( mUdf(float), mUdf(float) );
    Interval<double> depths( mUdf(double), mUdf(double) );

    if ( !getVelocityBoundsForTwt(twt,track,depths,times) )
	return mUdf(float);

    const double curvel = getVelocityForTwt( twt, track );
    const double depth = depths.start +
	( ( mCast(double,twt) - mCast(double,times.start) ) * curvel ) / 2.f;

    return mCast( float, depth );
}


float Well::D2TModel::getDah( float twt, const Track& track ) const
{
    const float depth = getDepth( twt, track );
    if ( mIsUdf(depth) )
        return mUdf(float);

    return track.getDahForTVD( depth );
}


double Well::D2TModel::getVelocityForDah( float dh, const Track& track ) const
{
    double velocity = mUdf(double);
    Interval<double> depths( mUdf(double), mUdf(double) );
    Interval<float> times( mUdf(float), mUdf(float) );

    if ( !getVelocityBoundsForDah(dh,track,depths,times) )
	return velocity;

    velocity = 2.f * depths.width() / (double)times.width();

    return velocity;
}


double Well::D2TModel::getVelocityForDepth( float dpt,
					    const Track& track ) const
{
    const float dahval = track.getDahForTVD( dpt );
    return getVelocityForDah( dahval, track );
}


double Well::D2TModel::getVelocityForTwt( float twt, const Track& track ) const
{
    double velocity = mUdf(double);
    Interval<double> depths( mUdf(double), mUdf(double) );
    Interval<float> times( mUdf(float), mUdf(float) );

    if ( !getVelocityBoundsForTwt(twt,track,depths,times) )
	return velocity;

    velocity = 2.f * depths.width() / (double)times.width();

    return velocity;
}


bool Well::D2TModel::getVelocityBoundsForDah( float dh, const Track& track,
					Interval<double>& depths,
					Interval<float>& times ) const
{
    const int idah = getVelocityIdx( dh, track );
    if ( idah <= 0 )
	return false;

    depths.start = track.getPos(dah_[idah-1]).z;
    depths.stop = track.getPos(dah_[idah]).z;
    times.start = t_[idah-1];
    times.stop = t_[idah];

    if ( depths.isUdf() )
        return false;

    bool reversedz = times.isRev() || depths.isRev();
    bool sametwt = mIsZero(times.width(),1e-6f) || mIsZero(depths.width(),1e-6);

    if ( reversedz || sametwt )
	return getOldVelocityBoundsForDah(dh,track,depths,times);

    return true;
}


bool Well::D2TModel::getVelocityBoundsForTwt( float twt, const Track& track,
					      Interval<double>& depths,
					      Interval<float>& times ) const
{
    const int idah = getVelocityIdx( twt, track, false );
    if ( idah <= 0 )
	return false;

    depths.start = track.getPos(dah_[idah-1]).z;
    depths.stop = track.getPos(dah_[idah]).z;
    if ( depths.isUdf() )
	return false;

    times.start = t_[idah-1];
    times.stop = t_[idah];

    bool reversedz = times.isRev() || depths.isRev();
    bool sametwt = mIsZero(times.width(),1e-6f) || mIsZero(depths.width(),1e-6);

    if ( reversedz || sametwt )
	return getOldVelocityBoundsForTwt(twt,track,depths,times);

    return true;
}


int Well::D2TModel::getVelocityIdx( float pos, const Track& track,
				    bool posisdah ) const
{
    const int dtsize = size();
    const int tsize = t_.size();
    if ( dtsize != tsize )
	return -1;

    int idah = posisdah
	     ? IdxAble::getUpperIdx( dah_, dtsize, pos )
	     : IdxAble::getUpperIdx( t_, dtsize, pos );

    if ( idah >= (dtsize-1) && posisdah )
    {
	int idx = 0;
	const int trcksz = track.size();
	const TypeSet<Coord3> allpos = track.getAllPos();
	TypeSet<double> trckposz;
	for ( int idz=0; idz<trcksz; idz++ )
	    trckposz += allpos[idz].z;

	const double reqz = track.getPos( pos ).z;
	idx = IdxAble::getUpperIdx( trckposz, trcksz, reqz );
	if ( idx >= trcksz ) idx--;
	const double dhtop = mCast( double, track.dah(idx-1) );
	const double dhbase = mCast( double, track.dah(idx) );
	const double fraction = ( reqz - track.value(idx-1) ) /
				( track.value(idx) - track.value(idx-1) );
	const float reqdh = mCast( float, dhtop + (fraction * (dhbase-dhtop)) );
	idah = IdxAble::getUpperIdx( dah_, dtsize, reqdh );
    }

    return idah >= dtsize ? dtsize-1 : idah;
}


bool Well::D2TModel::getOldVelocityBoundsForDah( float dh, const Track& track,
						 Interval<double>& depths,
						 Interval<float>& times ) const
{
    const int dtsize = size();
    const int idah = getVelocityIdx( dh, track );
    if ( idah <= 0 )
	return false;

    depths.start = track.getPos( dah_[idah-1] ).z;
    times.start = t_[idah-1];
    depths.stop = mUdf(double);
    times.stop = mUdf(float);

    for ( int idx=idah; idx<dtsize; idx++ )
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
	for ( int idx=idah-2; idx>=0; idx-- )
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


bool Well::D2TModel::getOldVelocityBoundsForTwt( float twt, const Track& track,
						 Interval<double>& depths,
						 Interval<float>& times ) const
{
    const int dtsize = size();
    const int idah = getVelocityIdx( twt, track, false );
    if ( idah <= 0 )
	return false;

    depths.start = track.getPos( dah_[idah-1] ).z;
    times.start = t_[idah-1];
    depths.stop = mUdf(double);
    times.stop = mUdf(float);

    for ( int idx=idah; idx<dtsize; idx++ )
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
	for ( int idx=idah-2; idx>=0; idx-- )
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


void Well::D2TModel::makeFromTrack( const Track& track, float vel,
				    float replvel )
{
    setEmpty();
    if ( track.isEmpty() )
	return;

    const float srddepth = mCast( float, -1.f * SI().seismicReferenceDatum() );
    const float kb  = track.getKbElev();
    const float bulkshift = mIsUdf( replvel ) ? 0.f
			  : ( kb+srddepth )* ( (2.f / vel) - (2.f / replvel) );

    int idahofminz = 0;
    int idahofmaxz = 0;
    float tvdmin = mCast(float,track.pos(0).z);
    float tvdmax = mCast(float,track.pos(0).z);

    for ( int idx=1; idx<track.size(); idx++ )
    {
	const float zpostrack = mCast(float,track.pos(idx).z);
	if ( zpostrack > tvdmax )
	{
	    tvdmax = zpostrack;
	    idahofmaxz = idx;
	}
	else if ( zpostrack < tvdmin )
	{
	    tvdmin = zpostrack;
	    idahofminz = idx;
	}
    }

    if ( tvdmax < srddepth ) // whole track above SRD !
	return;

    float firstdah = track.dah(idahofminz);
    if ( tvdmin < srddepth ) // no write above SRD
    {
	tvdmin = srddepth;
	firstdah = track.getDahForTVD( tvdmin );
    }

    add( firstdah, 2.f*( tvdmin-srddepth )/vel + bulkshift );
    add( track.dah(idahofmaxz), 2.f*( tvdmax-srddepth )/vel + bulkshift );
}


#define mLocalEps	1e-2f

bool Well::D2TModel::getTimeDepthModel( const Well::Data& wd,
					TimeDepthModel& model ) const
{
    Well::D2TModel d2t = *this;
    if ( !d2t.ensureValid(wd.track(),wd.info().replvel) )
	return false;

    TypeSet<float> depths;
    TypeSet<float> times;
    for ( int idx=0; idx<d2t.size(); idx++ )
    {
	const float curdah = d2t.dah( idx );
	depths += mCast(float, wd.track().getPos( curdah ).z );
	times += d2t.t( idx );
    }

    model.setModel( depths.arr(), times.arr(), depths.size() );

    return model.isOK();
}


bool Well::D2TModel::ensureValid( const Well::Track& track, float replvel )
{
    const int sz = size();
    if ( sz < 2 )
	return false;

    const float srddepth = mCast( float, -1.*SI().seismicReferenceDatum() );
    const float kbdepth = -1.f * track.getKbElev();
    const float replveldz = srddepth - kbdepth;
    const float timeatkbelev = -2.f * replveldz / replvel;
    const bool srdbelowkb = replveldz > mLocalEps;

    TypeSet<float> dahs, depths, times;
    mAllocVarLenArr( int, zidxs, sz );
    for ( int idx=0; idx<sz; idx++ )
    {
	const float curdah = dah( idx );
	dahs += curdah;
	depths += mCast( float, track.getPos( curdah ).z );
	times += value( idx );
	zidxs[idx] = idx;
    }
    sort_coupled( times.arr(), mVarLenArr(zidxs), sz );
    if ( depths[zidxs[sz-1]] < srddepth-mLocalEps || times[zidxs[sz-1]] < 0.f )
    {
	setEmpty();
	return true; //All points above SRD; D2TModel object should be empty
    }

    BendPointBasedMathFunction<float,float> dtmodel;
    dtmodel.setExtrapolateType(
	   BendPointBasedMathFunction<float,float>::ExtraPolGradient );
    const float mindepth = srdbelowkb ? srddepth + mLocalEps
				      : kbdepth + mLocalEps;
    int idah = 0;
    for ( int idx=0; idx<sz; idx++ )
    {
	dtmodel.add( depths[zidxs[idx]], times[zidxs[idx]] );
	if ( depths[zidxs[idx]] < mindepth )
	    idah++;
    }

    const float bulkshift = srdbelowkb
			  ? dtmodel.getValue( srddepth )
			  : dtmodel.getValue( kbdepth ) - timeatkbelev;
    if ( !mIsZero(bulkshift,1e-6f) )
    {
	for ( int idx=idah; idx<sz; idx++ )
	    times[zidxs[idx]] -= bulkshift;
    }

    D2TModel origmodel = *this;
    setEmpty();

    add( 0.f, timeatkbelev ); //set KB
    if ( srdbelowkb )
	add( track.getDahForTVD(srddepth,0.f), 0.f ); //set SRD

    if ( idah < sz && times[zidxs[idah]] > 1e-6f )
	add( dahs[zidxs[idah]], times[zidxs[idah]] );

    for ( int idx=idah+1; idx<sz; idx++ )
    {
	const int idx0 = zidxs[idx-1];
	const int idx1 = zidxs[idx];
	const float dh = dahs[idx1] - dahs[idx0];
	const float dt = times[idx1] - times[idx0];
	if ( dh > mLocalEps && dt > 1e-6f && times[idx1] > 1e-6f )
	    add( dahs[idx1], times[idx1] );
    }

    if ( size() < 2 )
    {
	*this = origmodel;
	return false;
    }

    return true;
}
