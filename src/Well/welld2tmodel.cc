/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Aug 2003
-*/


#include "welld2tmodel.h"

#include "elasticmodel.h"
#include "idxable.h"
#include "iopar.h"
#include "mathfunc.h"
#include "statparallelcalc.h"
#include "statruncalc.h"
#include "stratlevel.h"
#include "tabledef.h"
#include "unitofmeasure.h"
#include "velocitycalc.h"
#include "welldata.h"
#include "wellinfo.h"
#include "welltrack.h"

const char* Well::D2TModel::sKeyTimeWell()	{ return "=Time"; }
const char* Well::D2TModel::sKeyDataSrc()	{ return "Data source"; }

static const Well::D2TModel::ZType z_eps = Well::DahObj::dahEps();
static const Well::D2TModel::TWTType t_eps = 1e-5f;
static const Well::D2TModel::VelType v_eps = 1e-3f;

mDefineInstanceCreatedNotifierAccess(Well::D2TModel);

Well::D2TModel::D2TModel( const char* nm )
    : DahObj(nm)
{
    mTriggerInstanceCreatedNotifier();
}


Well::D2TModel::D2TModel( const D2TModel& oth )
    : DahObj(oth)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


Well::D2TModel::~D2TModel()
{
    sendDelNotif();
}


mImplMonitorableAssignment( Well::D2TModel, Well::DahObj )

void Well::D2TModel::copyClassData( const D2TModel& oth )
{
    times_ = oth.times_;
    desc_ = oth.desc_;
    datasource_ = oth.datasource_;
}


Monitorable::ChangeType Well::D2TModel::compareClassData(
					const D2TModel& oth ) const
{
    if ( times_ != oth.times_ )
	return cEntireObjectChange();

    mDeliverSingCondMonitorableCompare(
	desc_ == oth.desc_ && datasource_ == oth.datasource_,
	cParsChange() );
}


void Well::D2TModel::fillHdrPar( IOPar& iop ) const
{
    putNameInPar( iop );
    mLock4Read();
    iop.set( sKey::Desc(), desc_ );
    iop.set( sKeyDataSrc(), datasource_ );
}


void Well::D2TModel::useHdrPar( const IOPar& iop )
{
    getNameFromPar( iop );
    mLock4Write();
    iop.get( sKey::Desc(), desc_ );
    iop.get( sKeyDataSrc(), datasource_ );
    mSendEntireObjChgNotif();
}


void Well::D2TModel::getData( ZSetType& zs, TWTSetType& vals ) const
{
    mLock4Read();
    zs = dahs_;
    vals = times_;
}


void Well::D2TModel::setData( const ZSetType& zs, const TWTSetType& vals )
{
    mLock4Write();
    if ( doSetData( zs, vals, times_ ) )
	mSendEntireObjChgNotif();
}


bool Well::D2TModel::doSet( idx_type idx, ValueType twt )
{
    if ( mIsEqual(times_[idx],twt,t_eps) )
	return false;
    times_[idx] = twt;
    return true;
}


#define mGetReadAndTrackLock() mLock4Read(); MonitorLock mltrack( track )


Well::D2TModel::TWTType Well::D2TModel::getTime( ZType dh,
						 const Track& track ) const
{
    TWTType twt = mUdf(TWTType);
    Interval<double> depths( mUdf(double), mUdf(double) );
    Interval<TWTType> times( mUdf(TWTType), mUdf(TWTType) );
    mGetReadAndTrackLock();

    if ( !gtVelocityBoundsForDah(dh,track,depths,times) )
	return twt;

    double reqz = track.getPos(dh).z_;
    const double curvel = getVelocityForDah( dh, track );
    twt = times.start + 2.f* (TWTType)( ( reqz - depths.start ) / curvel );

    return twt;
}


Well::D2TModel::ZType Well::D2TModel::getDepth( TWTType twt,
						const Track& track ) const
{
    mGetReadAndTrackLock();
    return gtDepth( twt, track );
}


Well::D2TModel::ZType Well::D2TModel::gtDepth( TWTType twt,
						const Track& track ) const
{
    TWTIntvType times( mUdf(TWTType), mUdf(TWTType) );
    Interval<double> depths( mUdf(double), mUdf(double) );

    if ( !gtVelocityBoundsForTwt(twt,track,depths,times) )
	return mUdf(TWTType);

    const double curvel = gtVelocityForTwt( twt, track );
    const double depth = depths.start +
	( ( (double)twt - (double)times.start ) * curvel ) / 2.f;

    return (TWTType)depth;
}


Well::D2TModel::ZType Well::D2TModel::getDah( TWTType twt,
						const Track& track ) const
{
    mGetReadAndTrackLock();
    const ZType depth = gtDepth( twt, track );
    if ( mIsUdf(depth) )
        return mUdf(ZType);

    return track.getDahForTVD( depth );
}


double Well::D2TModel::getVelocityForDah( ZType dh, const Track& track ) const
{
    double velocity = mUdf(double);
    Interval<double> depths( mUdf(double), mUdf(double) );
    Interval<TWTType> times( mUdf(TWTType), mUdf(TWTType) );
    mGetReadAndTrackLock();

    if ( !gtVelocityBoundsForDah(dh,track,depths,times) )
	return velocity;

    velocity = 2.f * depths.width() / (double)times.width();

    return velocity;
}


double Well::D2TModel::getVelocityForDepth( ZType dpt,
					    const Track& track ) const
{
    const ZType dahval = track.getDahForTVD( dpt );
    return getVelocityForDah( dahval, track );
}


double Well::D2TModel::getVelocityForTwt( TWTType twt,
					  const Track& track ) const
{
    mGetReadAndTrackLock();
    return gtVelocityForTwt( twt, track );
}


double Well::D2TModel::gtVelocityForTwt( TWTType twt, const Track& track ) const
{
    double velocity = mUdf(double);
    Interval<double> depths( mUdf(double), mUdf(double) );
    TWTIntvType times( mUdf(TWTType), mUdf(TWTType) );

    mGetReadAndTrackLock();
    if ( !gtVelocityBoundsForTwt(twt,track,depths,times) )
	return velocity;

    velocity = 2.f * depths.width() / (double)times.width();

    return velocity;
}


bool Well::D2TModel::gtVelocityBoundsForDah( ZType dh, const Track& track,
					Interval<double>& depths,
					TWTIntvType& times ) const
{
    const int idah = gtVelocityIdx( dh, track );
    if ( idah <= 0 )
	return false;

    depths.start = track.getPos(dahs_[idah-1]).z_;
    depths.stop = track.getPos(dahs_[idah]).z_;
    times.start = times_[idah-1];
    times.stop = times_[idah];

    if ( depths.isUdf() )
        return false;

    bool reversedz = times.isRev() || depths.isRev();
    bool sametwt = mIsZero(times.width(),1e-6f) || mIsZero(depths.width(),1e-6);

    if ( reversedz || sametwt )
	return getOldVelocityBoundsForDah(dh,track,depths,times);

    return true;
}


bool Well::D2TModel::gtVelocityBoundsForTwt( TWTType twt, const Track& track,
					      Interval<double>& depths,
					      TWTIntvType& times ) const
{
    const int idah = gtVelocityIdx( twt, track, false );
    if ( idah <= 0 )
	return false;

    depths.start = track.getPos(dahs_[idah-1]).z_;
    depths.stop = track.getPos(dahs_[idah]).z_;
    if ( depths.isUdf() )
	return false;

    times.start = times_[idah-1];
    times.stop = times_[idah];

    bool reversedz = times.isRev() || depths.isRev();
    bool sametwt = mIsZero(times.width(),1e-6f) || mIsZero(depths.width(),1e-6);

    if ( reversedz || sametwt )
	return getOldVelocityBoundsForTwt(twt,track,depths,times);

    return true;
}


int Well::D2TModel::gtVelocityIdx( ZType pos, const Track& track,
				    bool posisdah ) const
{
    const int dtsize = size();
    const int tsize = times_.size();
    if ( dtsize != tsize )
	return -1;

    int idah = posisdah
	     ? IdxAble::getUpperIdx( dahs_, dtsize, pos )
	     : IdxAble::getUpperIdx( times_, dtsize, pos );

    if ( idah >= (dtsize-1) && posisdah )
    {
	int idx = 0;
	const int trcksz = track.size();
	const TypeSet<Coord3> allpos = track.getAllPos();
	TypeSet<double> trckposz;
	for ( int idz=0; idz<trcksz; idz++ )
	    trckposz += allpos[idz].z_;

	const double reqz = track.getPos( pos ).z_;
	idx = IdxAble::getUpperIdx( trckposz, trcksz, reqz );
	if ( idx >= trcksz ) idx--;
	const double dhtop = (double)track.dahByIdx(idx-1);
	const double dhbase = (double)track.dahByIdx(idx);
	const double fraction = ( reqz - track.valueByIdx(idx-1) ) /
			    ( track.valueByIdx(idx) - track.valueByIdx(idx-1) );
	const ZType reqdh = (ZType)(dhtop + (fraction * (dhbase-dhtop)));
	idah = IdxAble::getUpperIdx( dahs_, dtsize, reqdh );
    }

    return idah >= dtsize ? dtsize-1 : idah;
}


bool Well::D2TModel::getOldVelocityBoundsForDah( ZType dh, const Track& track,
						 Interval<double>& depths,
						 TWTIntvType& times ) const
{
    const int dtsize = size();
    const int idah = gtVelocityIdx( dh, track );
    if ( idah <= 0 )
	return false;

    depths.start = track.getPos( dahs_[idah-1] ).z_;
    times.start = times_[idah-1];
    depths.stop = mUdf(double);
    times.stop = mUdf(TWTType);

    for ( int idx=idah; idx<dtsize; idx++ )
    {
	const double curz = track.getPos( dahs_[idx] ).z_;
	const TWTType curtwt = times_[idx];
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
	times.start = mUdf(TWTType);
	for ( int idx=idah-2; idx>=0; idx-- )
	{
	    const double curz = track.getPos( dahs_[idx] ).z_;
	    const TWTType curtwt = times_[idx];
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


bool Well::D2TModel::getOldVelocityBoundsForTwt( TWTType twt,
			const Track& track, Interval<double>& depths,
			TWTIntvType& times ) const
{
    const int dtsize = size();
    const int idah = gtVelocityIdx( twt, track, false );
    if ( idah <= 0 )
	return false;

    depths.start = track.getPos( dahs_[idah-1] ).z_;
    times.start = times_[idah-1];
    depths.stop = mUdf(double);
    times.stop = mUdf(TWTType);

    for ( int idx=idah; idx<dtsize; idx++ )
    {
	const double curz = track.getPos( dahs_[idx] ).z_;
	const TWTType curtwt = times_[idx];
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
	times.start = mUdf(TWTType);
	for ( int idx=idah-2; idx>=0; idx-- )
	{
	    const double curz = track.getPos( dahs_[idx] ).z_;
	    const TWTType curtwt = times_[idx];
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


void Well::D2TModel::makeFromTrack( const Track& track, VelType vel,
				    VelType replvel )
{
    setEmpty();
    if ( track.isEmpty() )
	return;

    const double srddepth = -1.f * SI().seismicReferenceDatum();
    const double kb  = (double)track.getKbElev();
    const double veld = (double)vel;
    const double bulkshift = mIsUdf( replvel ) ? 0. : ( kb+srddepth ) *
			     ( (2. / veld) - (2. / (double)replvel ) );

    PointID idofminz = PointID::getInvalid();
    PointID idofmaxz = PointID::getInvalid();

    TrackIter it( track );
    if ( !it.next() )
	return;

    double tvdmin = it.pos().z_;
    idofminz = it.ID();
    double tvdmax = tvdmin;
    idofmaxz = idofminz;

    while ( it.next() )
    {
	const double zpostrack = it.pos().z_;
	if ( zpostrack > tvdmax )
	{
	    tvdmax = zpostrack;
	    idofmaxz = it.ID();
	}
	else if ( zpostrack < tvdmin )
	{
	    tvdmin = zpostrack;
	    idofminz = it.ID();
	}
    }

    if ( tvdmax < srddepth ) // whole track above SRD !
	return;

    const ZType lastdah = track.dah( idofmaxz );
    ZType firstdah = track.dah( idofminz );
    if ( tvdmin < srddepth ) // no write above SRD
    {
	tvdmin = srddepth;
	firstdah = track.getDahForTVD( tvdmin );
    }
    setValueAt( firstdah, (TWTType)( 2.*(tvdmin-srddepth)/veld + bulkshift) );
    setValueAt( lastdah, (TWTType)( 2.*(tvdmax-srddepth)/veld + bulkshift) );
}


#define mLocalEps	1e-2f

bool Well::D2TModel::getTimeDepthModel( const Well::Data& wd,
					TimeDepthModel& model ) const
{
    Well::D2TModel d2t = *this;
    uiString msg;
    if ( !d2t.ensureValid(wd,msg) )
	return false;

    TypeSet<double> depths, times;
    D2TModelIter iter( d2t );
    while ( iter.next() )
    {
	const ZType curdah = iter.dah();
	depths += wd.track().getPos( curdah ).z_;
	times += iter.t();
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
	const bool samez = mIsEqual( rawzvals[idx], zvals[lastidx], z_eps );
	const bool reversedtwt = rawtvals[idxs[idx]] < tvals[lastidx]-t_eps;
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
    const Interval<double> vreplzrg( srddepth, wllheadz );
    TypeSet<double> slownesses, thicknesses;
    for ( int idz=1; idz<zvals.size(); idz++ )
    {
	Interval<double> zrg( zvals[idz-1], zvals[idz] );
	if ( !zrg.overlaps(vreplzrg) )
	    continue;

	zrg.limitTo( vreplzrg );
	const double thickness = zrg.width();
	if ( thickness < z_eps )
	    continue;

	const double slowness = ( tvals[idz] - tvals[idz-1] ) /
				( zvals[idz] - zvals[idz-1] );

	if ( !validVelocityRange().includes(1./slowness,false) )
	    continue;

	slownesses += slowness;
	thicknesses += thickness;
    }

    if ( slownesses.isEmpty() )
    {
	if ( srddepth < wllheadz && !zvals.isEmpty() &&
	     !tvals.isEmpty() && tvals[0] > t_eps )
	    return 2. * ( zvals[0] - srddepth ) / tvals[0];
	else
	    return mUdf(double);
    }

    Stats::ParallelCalc<double> velocitycalc( Stats::CalcSetup(true),
					      slownesses.arr(),
					      slownesses.size(),
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
    originz += z_eps;
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
	if ( !mIsEqual(curvel,prevvel,v_eps) )
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
    tddepth += z_eps;
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
	zvals[idz] = tddepth - z_eps;
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
    const Well::Info::VelType replvel = info.replacementVelocity();
    if ( !mIsEqual((VelType)vreplinfile,replvel,v_eps) )
    {
	if ( mIsEqual(replvel,Well::getDefaultVelocity(),v_eps) )
	    info.setReplacementVelocity( (Well::Info::VelType)vreplinfile );
	else
	{
	    const UnitOfMeasure* uomvel = UnitOfMeasure::surveyDefVelUnit();
	    const uiString veluomlbl(
		    UnitOfMeasure::surveyDefVelUnitAnnot(true) );
	    const double fileval = mScaledValue( vreplinfile, uomvel );
	    const double replval = mScaledValue( replvel, uomvel );
	    msg = tr("Input error with the %1\n"
		  "Your time-depth model suggests a %1 of %2%4\n "
		  "but the %1 was set to: %3\n"
		  "Velocity information from input was overruled.");
	    msg.arg( replvelbl ).arg( fileval )
		.arg( replval ).withUnit( veluomlbl );
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
    if ( mIsZero(timeshift,t_eps) )
	return;

    msg = tr("The input time-depth model does not honor TWT=0 at SRD.");
    const UnitOfMeasure* uomz = UnitOfMeasure::surveyDefTimeUnit();
    const double usrtshftval = mScaledValue( timeshift, uomz );
    msg.appendPhrase(
	tr( "OpendTect will correct for this error by applying\n"
		  "a time shift of %1").arg( usrtshftval )
		  .withUnit(UnitOfMeasure::surveyDefTimeUnitAnnot(true)) );
    msg.appendPhrase(tr("The resulting travel-times will differ from "
		    "the input file") );

    for ( int idz=0; idz<tvals.size(); idz++ )
	tvals[idz] += timeshift;
}


void Well::D2TModel::convertDepthsToMD( const Well::Track& track,
			       const TypeSet<double>& zvals,
			       ZSetType& dahs )
{
    dahs.setSize( zvals.size(), mUdf(ZType) );
    ZType prevdah = 0.f;
    for ( int idz=0; idz<zvals.size(); idz++ )
    {
	const ZType depth = (ZType)zvals[idz];
	ZType dah = track.getDahForTVD( depth, prevdah );
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
	mErrRet( tr("Input data does not contain at least two valid rows") );

    inputsz = sortAndEnsureUniqueTZPairs( zvals, tvals );
    if ( inputsz < 2 )
    {
	mErrRet( tr("Input data does not contain at least two valid rows"
		 " after re-sorting and removal of duplicated positions") );
    }

    const Well::Track& track = wll.track();
    if ( track.isEmpty() )
	mErrRet( tr("Cannot get the time-depth model with an empty track") )

    const double zwllhead = track.getPos( 0.f ).z_;
    const double vreplfile = getVreplFromFile( zvals, tvals, zwllhead );
    Well::Info& wllinfo = const_cast<Well::Info&>( wll.info() );
    checkReplacementVelocity( wllinfo, vreplfile, warnmsg );

    const double srddepth = -1. * SI().seismicReferenceDatum();
    const bool kbabovesrd = zwllhead < srddepth;
    const double originz = kbabovesrd ? srddepth : zwllhead;
    const double origintwtinfile = getDatumTwtFromFile( zvals, tvals, originz );
    //before any data gets removed

    if ( !removePairsAtOrAboveDatum(zvals,tvals,zwllhead) )
	mErrRet( tr("Input data has not enough data points below the datum") )

    const Interval<double> trackrg = track.zRangeD();
    if ( !truncateToTD(zvals,tvals,trackrg.stop) )
	mErrRet( tr("Input data has not enough data points above TD") )

    removeDuplicatedVelocities( zvals, tvals );
    const double replveld = (double)wllinfo.replacementVelocity();
    shiftTimesIfNecessary( tvals, zwllhead, replveld, origintwtinfile, warnmsg);

    if ( trackrg.includes(originz,false) )
    {
	zvals.insert( 0, originz );
	tvals.insert( 0, kbabovesrd ?  0.f
				    : 2. * ( zwllhead-srddepth ) / replveld );
    }

    ZSetType dahs;
    convertDepthsToMD( track, zvals, dahs );

    TWTSetType times; append( times, tvals );
    d2t.setData( dahs, times );
    return true;
}


bool Well::D2TModel::ensureValid( const Well::Data& wll, uiString& msg,
				  TypeSet<double>* depths,
				  TypeSet<double>* times )
{
    const bool externalvals = depths && times;
    const int sz = externalvals ? depths->size() : size();
    if ( sz < 2 || (depths && !times) || (!depths && times) )
	{ msg = tr("Invalid Depth vs Time model"); return false; }

    const Well::Track& track = wll.track();
    TypeSet<double>* zvals = depths ? depths : new TypeSet<double>;
    TypeSet<double>* tvals = times ? times : new TypeSet<double>;
    if ( !externalvals )
    {
	for ( int idx=0; idx<sz; idx++ )
	{
	    const ZType curdah = dahs_[idx];
	    *zvals += track.getPos( curdah ).z_;
	    *tvals += (double)times_[idx];
	}
    }

    const bool isok = getTVDD2TModel( *this, wll, *zvals, *tvals, msg, msg );
    if ( !externalvals )
	{ delete zvals; delete tvals; }

    return isok;
}


bool Well::D2TModel::calibrateBy( const D2TModel& oth )
{
    if ( isEmpty() || oth.isEmpty() )
	return false;

    mLock4Read();

    typedef BendPointBasedMathFunction<ZType,TWTType> BPFn;
    BPFn func( BPFn::Linear, BPFn::None );
    bool havenonzero = false;
    size_type sz = gtSize();
    for ( size_type idx=0; idx<sz; idx++ )
    {
	const ZType dh = dahs_[idx];
	const TWTType tme = times_[idx];
	const TWTType toth = oth.valueAt( dh );
	if ( mIsUdf(toth) )
	    continue;

	const TWTType tdiff = toth - tme;
	if ( !mIsZero(tdiff,1e-6f) )
	    havenonzero = true;
	func.add( dh, tdiff );
    }
    if ( func.isEmpty() || !havenonzero )
	return false;

    mLock2Write();

    sz = gtSize();
    for ( idx_type idx=0; idx<sz; idx++ )
    {
	TWTType delta = func.getValue( dahs_[idx] );
	if ( !mIsUdf(delta) )
	    times_[idx] += delta;
    }

    mSendEntireObjChgNotif();
    return true;
}


Well::D2TModelIter::D2TModelIter( const D2TModel& mdl, bool atend )
    : DahObjIter(mdl,atend)
{
}


Well::D2TModelIter::D2TModelIter( const D2TModelIter& oth )
    : DahObjIter(oth)
{
}


const Well::D2TModel& Well::D2TModelIter::model() const
{
    return static_cast<const D2TModel&>( monitored() );
}


Well::D2TModelIter::TWTType Well::D2TModelIter::t() const
{
    return isValid() ? model().times_[ curidx_ ] : mUdf(TWTType);
}
