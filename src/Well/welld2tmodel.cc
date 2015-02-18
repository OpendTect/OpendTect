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
#include "statparallelcalc.h"
#include "statruncalc.h"
#include "stratlevel.h"
#include "survinfo.h"
#include "tabledef.h"
#include "unitofmeasure.h"
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

    const double srddepth = -1.f * SI().seismicReferenceDatum();
    const double kb  = mCast(double,track.getKbElev());
    const double veld = mCast(double,vel);
    const double bulkshift = mIsUdf( replvel ) ? 0. : ( kb+srddepth ) *
			     ( (2. / veld) - (2. / mCast(double,replvel) ) );

    int idahofminz = 0;
    int idahofmaxz = 0;
    double tvdmin = track.pos(0).z;
    double tvdmax = track.pos(0).z;

    for ( int idx=1; idx<track.size(); idx++ )
    {
	const double zpostrack = track.pos(idx).z;
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

    const float lastdah = track.dah( idahofmaxz );

    add( firstdah, mCast(float,2.*( tvdmin-srddepth )/veld + bulkshift) );
    add( lastdah, mCast(float,2.*( tvdmax-srddepth )/veld + bulkshift) );
}


#define mLocalEps	1e-2f

bool Well::D2TModel::getTimeDepthModel( const Well::Data& wd,
					TimeDepthModel& model ) const
{
    Well::D2TModel d2t = *this;
    if ( !d2t.ensureValid(wd) )
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
    if ( track.isEmpty() )
	return false;

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

    if ( srdbelowkb )
	add( track.getDahForTVD(srddepth,0.f), 0.f ); //set SRD
    else
	add( 0.f, timeatkbelev );

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


static int sortAndEnsureUniqueTZPairs( TypeSet<double>& zvals,
				       TypeSet<double>& tvals )
{
    if ( zvals.isEmpty() || zvals.size() != tvals.size() )
	return 0;

    TypeSet<double> rawzvals, rawtvals;
    rawzvals = zvals;
    rawtvals = tvals;
    const int inputsz = rawzvals.size();
    mAllocVarLenIdxArr( int, idxs, inputsz );
    sort_coupled( rawzvals.arr(), mVarLenArr(idxs), inputsz );

    zvals.setEmpty();
    tvals.setEmpty();
    zvals += rawzvals[0];
    tvals += rawtvals[idxs[0]];
    for ( int idx=1; idx<inputsz; idx++ )
    {
	const int lastidx = zvals.size()-1;
	const bool samez = mIsEqual( rawzvals[idx], zvals[lastidx], mDefEps );
	const bool reversedtwt = rawtvals[idxs[idx]] < tvals[lastidx] - mDefEps;
	if ( samez || reversedtwt )
	    continue;

	zvals += rawzvals[idx];
	tvals += rawtvals[idxs[idx]];
    }

    return zvals.size();
}


static double getVreplFromFile( const TypeSet<double>& zvals,
				const TypeSet<double>& tvals, double wllheadz )
{
    if ( zvals.size() != tvals.size() )
	return mUdf(double);

    const double srddepth = -1. * SI().seismicReferenceDatum();
    const Interval<double> vrepldepthrg( srddepth, wllheadz );
    TypeSet<double> vels, thicknesses;
    for ( int idz=1; idz<zvals.size(); idz++ )
    {
	Interval<double> velrg( zvals[idz-1], zvals[idz] );
	if ( !velrg.overlaps(vrepldepthrg) )
	    continue;

	velrg.limitTo( vrepldepthrg );
	const double thickness = velrg.width();
	if ( thickness < mDefEps )
	    continue;

	vels += ( tvals[idz] - tvals[idz-1] ) / ( tvals[idz] - zvals[idz-1] );
	thicknesses += thickness;
    }

    if ( vels.isEmpty() )
	return mUdf(double);

    Stats::ParallelCalc<double> velocitycalc( Stats::CalcSetup(true),
					      vels.arr(), vels.size(),
					      thicknesses.arr() );
    velocitycalc.execute();
    const double avgslowness = velocitycalc.average();
    return mIsUdf(avgslowness) || mIsZero(avgslowness,mDefEps)
	   ? mUdf(double) : 2. / avgslowness;
}


static double getDatumTwtFromFile( const TypeSet<double>& zvals,
				   const TypeSet<double>& tvals, double targetz)
{
    if ( zvals.size() != tvals.size() )
	return mUdf(double);

    BendPointBasedMathFunction<double,double> tdcurve(
	  BendPointBasedMathFunction<double,double>::Linear,
	  BendPointBasedMathFunction<double,double>::None );

    for ( int idz=0; idz<zvals.size(); idz++ )
	tdcurve.add( zvals[idz], tvals[idz] );

    return tdcurve.getValue( targetz );
}


static bool removePairsAtOrAboveDatum( TypeSet<double>& zvals,
				       TypeSet<double>& tvals, double wllheadz )
{
    if ( zvals.size() != tvals.size() )
	return false;

    const double srddepth = -1. * SI().seismicReferenceDatum();
    double originz = wllheadz < srddepth ? srddepth : wllheadz;
    originz += mDefEps;
    bool needremove = false;
    int idz=0;
    const int sz = zvals.size();
    while( true && idz < sz )
    {
	if ( zvals[idz] > originz )
	    break;

	needremove = true;
	idz++;
    }

    if ( needremove )
    {
	idz--;
	zvals.removeRange( 0, idz );
	tvals.removeRange( 0, idz );
    }

    return zvals.size() > 1;
}


static void removeDuplicatedVelocities( TypeSet<double>& zvals,
					TypeSet<double>& tvals )
{
    const int sz = zvals.size();
    if ( sz < 3 || tvals.size() != sz )
	return;

    double prevvel = ( zvals[sz-1]-zvals[sz-2] ) / ( tvals[sz-1]-tvals[sz-2] );
    for ( int idz=sz-2; idz>0; idz-- )
    {
	const double curvel = ( zvals[idz] - zvals[idz-1] ) /
			      ( tvals[idz] - tvals[idz-1] );
	if ( !mIsEqual(curvel,prevvel,mDefEps) )
	{
	    prevvel = curvel;
	    continue;
	}

	zvals.removeSingle(idz);
	tvals.removeSingle(idz);
    }
}


static bool truncateToTD( TypeSet<double>& zvals,
			  TypeSet<double>& tvals, double zstop )
{
    zstop += mDefEps;
    const int sz = zvals.size();
    if ( sz < 3 || tvals.size() != sz )
	return false;

    if ( zvals[0] > zstop )
    {
	zvals.setEmpty();
	tvals.setEmpty();
    }
    else
    {
	for ( int idz=1; idz<sz; idz++ )
	{
	    if ( zvals[idz] < zstop )
		continue;

	    const double vel = ( zvals[idz] - zvals[idz-1] ) /
			       ( tvals[idz] - tvals[idz-1] );
	    zvals[idz] = zstop - mDefEps;
	    tvals[idz] = tvals[idz-1] + ( zstop - zvals[idz-1]) / vel;
	    if ( idz+1 <= sz-1 )
	    {
		zvals.removeRange(idz+1,sz-1);
		tvals.removeRange(idz+1,sz-1);
	    }
	    break;
	}
    }

    return zvals.size() > 1;
}


#define mScaledValue(s,uom) ( uom ? uom->userValue(s) : s )
static void checkReplacementVelocity( Well::Info& info, double vreplinfile,
				      BufferString& msg )
{
    if ( mIsUdf(vreplinfile) )
	return;

    FixedString replvelbl( Well::Info::sKeyreplvel() );
    if ( !mIsEqual((float)vreplinfile,info.replvel,mDefEpsF) )
    {
	if ( mIsEqual(info.replvel,Well::getDefaultVelocity(),mDefEpsF) )
	{
	    info.replvel = mCast(float,vreplinfile);
	}
	else
	{
	    msg.set( "Input error with the " ).add( replvelbl ).addNewLine();
	    const UnitOfMeasure* uomdepth = UnitOfMeasure::surveyDefDepthUnit();
	    const BufferString veluomlbl(
		    UnitOfMeasure::surveyDefDepthUnitAnnot(true,false), "/s" );

	    msg.add( "Your time-depth model suggests a " );
	    msg.add( replvelbl ).add( " of " );
	    msg.add( toString(mScaledValue(vreplinfile,uomdepth), 2 ) );
	    msg.add( veluomlbl ).addNewLine().add( "but the " );
	    msg.add( replvelbl ).add( " was set to: " );
	    msg.add( toString(mScaledValue(info.replvel,uomdepth), 2) );
	    msg.add( veluomlbl );
	}
    }
}


static void shiftTimesIfNecessary( TypeSet<double>& tvals, double wllheadz,
				   double vrepl, double origintwtinfile,
				   BufferString& msg )
{
    if ( mIsUdf(origintwtinfile) )
	return;

    const double srddepth = -1. * SI().seismicReferenceDatum();
    const double origintwt = wllheadz < srddepth
			? 0.f : 2.f * ( srddepth - wllheadz) / vrepl;
    const double timeshift = origintwtinfile - origintwt;
    if ( mIsZero(timeshift,mDefEps) )
	return;

    msg.set( "Error with the input time-depth model:" );
    msg.addNewLine().add( "It does not honour TWT(Z=SRD) = 0." );
    const UnitOfMeasure* uomz = UoMR().get( "Milliseconds" );
    msg.addNewLine().add( "OpendTect WILL correct for this error by applying a "
			   "time shift of: " );
    msg.add( toString(mScaledValue(timeshift,uomz),2) ).add( uomz->symbol() );
    msg.addNewLine();
    msg.add( "The resulting travel-times will differ from the file" );

    for ( int idz=0; idz<tvals.size(); idz++ )
	tvals[idz] += timeshift;
}


static void convertDepthsToMD( const Well::Track& track,
			       TypeSet<double>& zvals )
{
    float prevdah = 0.f;
    for ( int idz=0; idz<zvals.size(); idz++ )
    {
	const float depth = mCast( float, zvals[idz] );
	float dah = track.getDahForTVD( depth, prevdah );
	if ( mIsUdf(dah) )
	    dah = track.getDahForTVD( depth );

	zvals[idz] = mCast( double, dah );
	if ( !mIsUdf(dah) )
	    prevdah = dah;
    }
}


#undef mErrRet
#define mErrRet(s) { errmsg = s; return false; }
#define mNewLn(s) { s.addNewLine(); }

static bool getTVDD2TModel( Well::D2TModel& d2t, const Well::Data& wll,
			    TypeSet<double>& zvals, TypeSet<double>& tvals,
			    BufferString& errmsg, BufferString& warnmsg )
{
    int inputsz = zvals.size();
    if ( inputsz < 2 || inputsz != tvals.size() )
	mErrRet( "Input file does not contain at least two valid rows" );

    inputsz = sortAndEnsureUniqueTZPairs( zvals, tvals );
    if ( inputsz < 2 )
    {
	mErrRet( "Input file does not contain at least two valid rows"
		 "after resorting and removal of duplicated positions" );
    }

    const Well::Track& track = wll.track();
    if ( track.isEmpty() )
	mErrRet( "Cannot get the time-depth model with an empty track" )

    const double zwllhead = track.getPos( 0.f ).z;
    const double vreplfile = getVreplFromFile( zvals, tvals, zwllhead );
    Well::Info& wllinfo = const_cast<Well::Info&>( wll.info() );
    checkReplacementVelocity( wllinfo, vreplfile, warnmsg );

    const double srddepth = -1. * SI().seismicReferenceDatum();
    const bool kbabovesrd = zwllhead < srddepth;
    const double originz = kbabovesrd ? srddepth : zwllhead;
    const double origintwtinfile = getDatumTwtFromFile( zvals, tvals, originz );
    //before any data gets removed

    if ( !removePairsAtOrAboveDatum(zvals,tvals,zwllhead) )
	mErrRet( "Input file has not enough data points below the datum" )

    const Interval<double> trackrg = track.zRangeD();
    if ( !truncateToTD(zvals,tvals,trackrg.stop) )
	mErrRet( "Input file has not enough data points above TD" )

    removeDuplicatedVelocities( zvals, tvals );
    const double replveld = mCast( double, wllinfo.replvel );
    shiftTimesIfNecessary( tvals, zwllhead, mCast(double,replveld),
			   origintwtinfile, warnmsg );

    if ( trackrg.includes(originz,false) )
    {
	zvals.insert( 0, originz );
	tvals.insert( 0, kbabovesrd ?
			 0.f : 2. * ( zwllhead-srddepth ) / replveld );
    }
    convertDepthsToMD( track, zvals );

    d2t.setEmpty();
    for ( int idx=0; idx<zvals.size(); idx++ )
	d2t.add( mCast(float,zvals[idx]), mCast(float,tvals[idx]) );

    return true;
}


bool Well::D2TModel::ensureValid( const Well::Data& wll,
				  TypeSet<double>* depths,
				  TypeSet<double>* times,
				  BufferString* emsg, BufferString* wmsg )
{
    const int sz = depths ? depths->size() : size();
    if ( sz < 2 || depths != times )
	return false;

    BufferString* errmsg = emsg ? emsg : new BufferString;
    BufferString* warnmsg = wmsg ? wmsg : new BufferString;
    const Well::Track& track = wll.track();

    TypeSet<double>* zvals = depths ? depths : new TypeSet<double>;
    TypeSet<double>* tvals = times ? times : new TypeSet<double>;
    for ( int idx=0; idx<sz; idx++ )
    {
	const float curdah = dah_[idx];

	*zvals += track.getPos( curdah ).z;
	*tvals += mCast( double, t_[idx] );
    }

    const bool success =
	      getTVDD2TModel( *this, wll, *zvals, *tvals, *errmsg, *warnmsg );

    if ( !depths ) delete zvals;
    if ( !times ) delete tvals;
    if ( !emsg ) delete errmsg;
    if ( !warnmsg) delete warnmsg;

    return success;
}

