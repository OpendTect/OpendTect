/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/


#include "welld2tmodel.h"

#include "ailayer.h"
#include "idxable.h"
#include "iopar.h"
#include "mathfunc.h"
#include "statparallelcalc.h"
#include "statruncalc.h"
#include "stratlevel.h"
#include "tabledef.h"
#include "timedepthmodel.h"
#include "unitofmeasure.h"
#include "welldata.h"
#include "welltrack.h"

const char* Well::D2TModel::sKeyTimeWell()	{ return "=Time"; }
const char* Well::D2TModel::sKeyDataSrc()	{ return "Data source"; }


Well::D2TModel& Well::D2TModel::operator =( const Well::D2TModel& d2t )
{
    if ( &d2t != this )
    {
	DahObj::operator=( d2t );
	t_ = d2t.t_;
	desc = d2t.desc;
	datasource = d2t.datasource;
    }
    return *this;
}


#define mDefEpsZ 1e-2
#define mDefEpsT 1e-5
#define mDefEpsV 1e-3

bool Well::D2TModel::operator ==( const Well::D2TModel& d2t ) const
{
    if ( &d2t == this )
	return true;

    if ( d2t.size() != size() )
	return false;

    for ( int idx=0; idx<size(); idx++ )
    {
	if ( !mIsEqual(d2t.dah(idx),dah_[idx],mDefEpsZ) ||
	     !mIsEqual(d2t.t(idx),t_[idx],mDefEpsT) )
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


mDefParallelCalc6Pars(Dah2Twt,
		  od_static_tr("Dah2Twt", "Dah to TWT conversion"),
		      const float*, daharr, const Well::Track&, track,
		      const Well::D2TModel&, d2t, const UnitOfMeasure*, dah_uom,
		      float*, twtarr, const UnitOfMeasure*, twt_uom)
mDefParallelCalcBody(
const UnitOfMeasure* zsuom = UnitOfMeasure::surveyDefDepthStorageUnit();
const UnitOfMeasure* tsuom = nullptr;
,
const float dah =  getConvertedValue( daharr_[idx], dah_uom_, zsuom );
const float twt = mIsUdf(dah) ? mUdf(float) : d2t_.getTime( dah, track_ );
twtarr_[idx] = getConvertedValue( twt, tsuom, twt_uom_ );
,
)


void Well::D2TModel::getAllTime( int sz, const float* dah, const Track& track,
				 const UnitOfMeasure* in, float* twt,
				 const UnitOfMeasure* out ) const
{
    Dah2Twt converter( sz, dah, track, *this, in, twt, out );
    converter.execute();
}


Interval<float> Well::D2TModel::getTimeRange( const Interval<float>& dahrg,
					      const Track& track,
					      const UnitOfMeasure* in_uom,
					 const UnitOfMeasure* out_uom ) const
{
    const UnitOfMeasure* zsuom = UnitOfMeasure::surveyDefDepthStorageUnit();
    const UnitOfMeasure* ztsuom = nullptr;
    const float start = getConvertedValue( dahrg.start, in_uom, zsuom );
    const float stop = getConvertedValue( dahrg.stop, in_uom, zsuom );
    const float twtbeg = getConvertedValue( getTime(start, track), ztsuom,
					    out_uom );
    const float twtend = getConvertedValue( getTime(stop, track), ztsuom,
					    out_uom );
    return Interval<float>( twtbeg, twtend );
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

    if ( idah < 0 )
	return 0;

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
    uiString msg;
    if ( !d2t.ensureValid(wd,msg) )
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
	const bool samez = mIsEqual( rawzvals[idx], zvals[lastidx], mDefEpsZ );
	const bool reversedtwt = rawtvals[idxs[idx]] < tvals[lastidx]-mDefEpsT;
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
	if ( thickness < mDefEpsZ )
	    continue;

	const double vel = ( tvals[idz] - tvals[idz-1] ) /
			   ( zvals[idz] - zvals[idz-1] );

	if ( !validVelocityRange().includes(1./vel,false) )
	    continue;

	vels += vel;
	thicknesses += thickness;
    }

    if ( vels.isEmpty() )
    {
	if ( srddepth < wllheadz && !zvals.isEmpty() &&
	     !tvals.isEmpty() && tvals[0] > mDefEpsT )
	    return 2. * ( zvals[0] - srddepth ) / tvals[0];
	else
	    return mUdf(double);
    }

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
    originz += mDefEpsZ;
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

    return zvals.size() > 0;
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
	if ( !mIsEqual(curvel,prevvel,mDefEpsV) )
	{
	    prevvel = curvel;
	    continue;
	}

	zvals.removeSingle(idz);
	tvals.removeSingle(idz);
    }
}


static bool truncateToTD( TypeSet<double>& zvals,
			  TypeSet<double>& tvals, double tddepth )
{
    tddepth += mDefEpsZ;
    const int sz = zvals.size();
    if ( sz < 1 )
	return false;

    if ( zvals[0] > tddepth || tvals.size() != sz )
    {
	zvals.setEmpty();
	tvals.setEmpty();
	return false;
    }

    for ( int idz=1; idz<sz; idz++ )
    {
	if ( zvals[idz] < tddepth )
	    continue;

	const double vel = ( zvals[idz] - zvals[idz-1] ) /
			   ( tvals[idz] - tvals[idz-1] );
	zvals[idz] = tddepth - mDefEpsZ;
	tvals[idz] = tvals[idz-1] + ( tddepth - zvals[idz-1]) / vel;
	if ( idz+1 <= sz-1 )
	{
	    zvals.removeRange(idz+1,sz-1);
	    tvals.removeRange(idz+1,sz-1);
	}

	break;
    }

    return zvals.size() > 0;
}


#define mScaledValue(s,uom) ( uom ? uom->userValue(s) : s )
void Well::D2TModel::checkReplacementVelocity( Well::Info& info,
					       double vreplinfile,
					       uiString& msg )
{
    if ( mIsUdf(vreplinfile) )
	return;

    uiString replvelbl = Well::Info::sReplVel();
    if ( !mIsEqual((float)vreplinfile,info.replvel_,mDefEpsV) )
    {
	if ( mIsEqual(info.replvel_,Well::getDefaultVelocity(),mDefEpsV) )
	{
	    info.replvel_ = mCast(float,vreplinfile);
	}
	else
	{
	    const UnitOfMeasure* uomvel = UnitOfMeasure::surveyDefVelUnit();
	    const uiString veluomlbl(
		    UnitOfMeasure::surveyDefVelUnitAnnot(true,false) );
	    const BufferString fileval =
			       toString(mScaledValue(vreplinfile,uomvel), 2 );
	    msg = tr("Input error with the %1\n"
		  "Your time-depth model suggests a %1 of %2%4\n "
		  "but the %1 was set to: %3%4\n"
		  "Velocity information from file was overruled.");
	    msg.arg( replvelbl ).arg( fileval )
	       .arg( toString(mScaledValue(info.replvel_,uomvel), 2) )
	       .arg( veluomlbl );
	}
    }
}


void Well::D2TModel::shiftTimesIfNecessary( TypeSet<double>& tvals,
				   double wllheadz,
				   double vrepl, double origintwtinfile,
				   uiString& msg )
{
    if ( mIsUdf(origintwtinfile) )
	return;

    const double srddepth = -1. * SI().seismicReferenceDatum();
    const double origintwt = wllheadz < srddepth
			   ? 0.f : 2.f * ( wllheadz - srddepth ) / vrepl;
    const double timeshift = origintwt - origintwtinfile;
    if ( mIsZero(timeshift,mDefEpsT) )
	return;

    msg = tr("Error in input time-depth model, "
	     "it does not honor TWT=0 at SRD.");
    const UnitOfMeasure* uomz = UnitOfMeasure::surveyDefTimeUnit();
    msg.append(
	tr( "\nOpendTect will correct for this error by applying\n"
		"a time shift of: %1 %2.\n"
		"The resulting travel-times will differ from the input file."))
		   .arg( toString(mScaledValue(timeshift,uomz),2) )
		   .arg(UnitOfMeasure::surveyDefTimeUnitAnnot(true,false) );

    for ( int idz=0; idz<tvals.size(); idz++ )
	tvals[idz] += timeshift;
}


void Well::D2TModel::convertDepthsToMD( const Well::Track& track,
			       const TypeSet<double>& zvals,
			       TypeSet<float>& dahs )
{
    dahs.setSize( zvals.size(), mUdf(float) );
    float prevdah = 0.f;
    for ( int idz=0; idz<zvals.size(); idz++ )
    {
	const float depth = mCast( float, zvals[idz] );
	float dah = track.getDahForTVD( depth, prevdah );
	if ( mIsUdf(dah) )
	    dah = track.getDahForTVD( depth );

	dahs[idz] = dah;
	if ( !mIsUdf(dah) )
	    prevdah = dah;
    }
}


#undef mErrRet
#define mErrRet(s) { errmsg = s; return false; }
#define mNewLn(s) { s.addNewLine(); }

bool Well::D2TModel::getTVDD2TModel( Well::D2TModel& d2t, const Well::Data& wll,
			    TypeSet<double>& zvals, TypeSet<double>& tvals,
			    uiString& errmsg, uiString& warnmsg )
{
    int inputsz = zvals.size();
    if ( inputsz < 2 || inputsz != tvals.size() )
	mErrRet( tr("Input file does not contain at least two valid rows.") );

    inputsz = sortAndEnsureUniqueTZPairs( zvals, tvals );
    if ( inputsz < 2 )
    {
	mErrRet( tr("Input file does not contain at least two valid rows "
		 "after resorting and removal of duplicated positions.") );
    }

    const Well::Track& track = wll.track();
    if ( track.isEmpty() )
	mErrRet( tr("Cannot get the time-depth model with an empty track.") )

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
	mErrRet( tr("Input file has not enough data points below the datum.") )

    const Interval<double> trackrg = track.zRangeD();
    if ( !truncateToTD(zvals,tvals,trackrg.stop) )
	mErrRet( tr("Input file has not enough data points above TD.") )

    removeDuplicatedVelocities( zvals, tvals );
    const double replveld( wllinfo.replvel_ );
    shiftTimesIfNecessary( tvals, zwllhead, mCast(double,replveld),
			   origintwtinfile, warnmsg );

    if ( trackrg.includes(originz,false) )
    {
	zvals.insert( 0, originz );
	tvals.insert( 0, kbabovesrd ?
			 0.f : 2. * ( zwllhead-srddepth ) / replveld );
    }

    TypeSet<float> dahs;
    convertDepthsToMD( track, zvals, dahs );

    d2t.setEmpty();
    for ( int idx=0; idx<zvals.size(); idx++ )
	d2t.add( dahs[idx], mCast(float,tvals[idx]) );
    d2t.updateDahRange();
    return true;
}


bool Well::D2TModel::ensureValid( const Well::Data& wll, uiString& msg,
				  TypeSet<double>* depths,
				  TypeSet<double>* times )
{
    const bool externalvals = depths && times;
    const int sz = externalvals ? depths->size() : size();
    if ( sz < 2 || (depths && !times) || (!depths && times) )
	{ msg = tr("Input model invalid."); return false; }

    const Well::Track& track = wll.track();
    TypeSet<double>* zvals = depths ? depths : new TypeSet<double>;
    TypeSet<double>* tvals = times ? times : new TypeSet<double>;
    if ( !externalvals )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    const float curdah = dah_[idx];
	    *zvals += track.getPos( curdah ).z;
	    *tvals += mCast( double, t_[idx] );
	}
    }

    const bool isok = getTVDD2TModel( *this, wll, *zvals, *tvals, msg, msg );
    if ( !externalvals )
	{ delete zvals; delete tvals; }

    return isok;
}
