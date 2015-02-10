/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : May 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "wellimpasc.h"

#include "ailayer.h"
#include "mathfunc.h"
#include "sorting.h"
#include "statparallelcalc.h"
#include "statruncalc.h"
#include "tabledef.h"
#include "unitofmeasure.h"
#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "idxable.h"
#include "od_ostream.h"


static bool convToDah( const Well::Track& trck, float& val,
		       float prevdah=mUdf(float) )
{
    val = trck.getDahForTVD( val, prevdah );
    return !mIsUdf(val);
}


namespace Well
{

Table::FormatDesc* TrackAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "WellTrack" );
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true );
    fd->bodyinfos_ += Table::TargetInfo::mkDepthPosition( false );
    Table::TargetInfo* ti = new Table::TargetInfo( "MD", FloatInpSpec(),
						   Table::Optional );
    ti->setPropertyType( PropertyRef::Dist );
    ti->selection_.unit_ = UnitOfMeasure::surveyDefDepthUnit();
    fd->bodyinfos_ += ti;
    return fd;
}


bool TrackAscIO::readTrackData( TypeSet<Coord3>& pos, TypeSet<float>& mdvals,
				float& kbelevinfile ) const
{
    if ( !getHdrVals(strm_) )
	return false;

    const bool isxy = fd_.bodyinfos_[0]->selection_.form_ == 0;
    const uiString nozpts = tr("At least one point had neither Z nor MD");
    bool nozptsfound = false;

    while( true )
    {
	const int ret = getNextBodyVals( strm_ );
	if ( ret < 0 ) return false;
	if ( ret == 0 ) break;

	Coord3 curpos;
	curpos.x = getdValue(0);
	curpos.y = getdValue(1);
	if ( !isxy && !mIsUdf(curpos.x) && !mIsUdf(curpos.y) )
	{
	    Coord wc( SI().transform(
			BinID( mNINT32(curpos.x), mNINT32(curpos.y) ) ) );
	    curpos.x = wc.x; curpos.y = wc.y;
	}
	if ( mIsUdf(curpos.x) || mIsUdf(curpos.y) )
	    continue;

	curpos.z = getdValue(2);
	const float dah = getfValue(3);
	if ( mIsUdf(curpos.z) && mIsUdf(dah) )
	{
	    if ( !nozptsfound )
		warnmsg_.append( nozpts, true );
	    else
		nozptsfound = true;

	    continue;
	}

	pos += curpos;
	mdvals += dah;
	if ( mIsUdf(kbelevinfile) && !mIsUdf(curpos.z) && !mIsUdf(dah) )
	    kbelevinfile = dah - mCast(float,curpos.z);
    }

    return !pos.isEmpty();
}


#define mErrRet(s) { errmsg_ = s; return false; }
#define mScaledValue(s,uom) ( uom ? uom->userValue(s) : s )
bool TrackAscIO::computeMissingValues( TypeSet<Coord3>& pos,
				       TypeSet<float>& mdvals,
				       float& kbelevinfile ) const
{
    if ( pos.isEmpty() || mdvals.isEmpty() || pos.size() != mdvals.size() )
	return false;

    Coord3 prevpos = pos[0];
    float prevdah = mdvals[0];
    if ( mIsUdf(prevpos.z) && mIsUdf(prevdah) )
	return false;

    if ( mIsUdf(kbelevinfile) )
	kbelevinfile = 0.f;

    if ( mIsUdf(prevpos.z) )
    {
	prevpos.z = mCast( double, prevdah - kbelevinfile );
	pos[0].z = prevpos.z;
    }

    if ( mIsUdf(prevdah) )
    {
	prevdah = mCast( float, prevpos.z ) + kbelevinfile;
	mdvals[0] = prevdah;
    }

    const UnitOfMeasure* uom = UnitOfMeasure::surveyDefDepthUnit();
    const uiString uomlbl(UnitOfMeasure::surveyDefDepthUnitAnnot(true,false) );
    for ( int idz=1; idz<pos.size(); idz++ )
    {
	Coord3& curpos = pos[idz];
	float& dah = mdvals[idz];
	if ( mIsUdf(curpos) && mIsUdf(dah) )
	    return false;
	else if ( mIsUdf(curpos) )
	{
	    const double dist = mCast( double, dah - prevdah );
	    const double hdist = Coord(curpos).distTo( Coord(prevpos) );
	    if ( dist < hdist )
	    {
		const BufferString val = toString( mScaledValue( dah, uom), 2 );
		mErrRet( tr("Impossible MD to TVD transformation for MD=%1%2" )
			 .arg(val)
			 .arg(uomlbl) )
	    }

	    curpos.z = prevpos.z + Math::Sqrt( dist*dist - hdist*hdist );
	}
	else if ( mIsUdf(dah) )
	{
	    const double dist = curpos.distTo( prevpos );
	    if ( dist < 0. )
	    {
		const BufferString val = toString(mScaledValue(curpos.z,uom),2);
		mErrRet( tr( "Impossible TVD to MD transformation for Z=%1%2" )
			  .arg(val)
			  .arg(uomlbl) )
	    }

	    dah = prevdah + mCast(float, dist );
	}
	prevpos = curpos;
	prevdah = dah;
    }

    return true;
}


static void adjustKBIfNecessary( TypeSet<Coord3>& pos, float kbelevinfile,
				 float kbelev )
{
    if ( mIsUdf(kbelev) || mIsUdf(kbelevinfile) )
	return;

    const float kbshift = kbelev - kbelevinfile;
    for ( int idz=0; idz<pos.size(); idz++ )
    {
	if ( !mIsUdf(pos[idz].z) )
	    pos[idz].z -= kbshift;
    }
}


static void addOriginIfNecessary( TypeSet<Coord3>& pos, TypeSet<float>& mdvals )
{
    if ( mdvals.isEmpty() || mdvals[0] < mDefEpsF )
	return;

    Coord3 surfloc = pos[0];
    surfloc.z = mCast(float, surfloc.z ) - mdvals[0];

    pos.insert( 0, surfloc );
    mdvals.insert( 0, 0.f );
}


static void adjustToTDIfNecessary( TypeSet<Coord3>& pos,
				   TypeSet<float>& mdvals, float td )
{
    if ( mIsUdf(td) || pos.size() != mdvals.size() )
	return;

    for ( int idz=pos.size()-1; idz>=0; idz-- )
    {
	if ( mdvals[idz] <= td )
	    break;

	mdvals.removeSingle( idz );
	pos.removeSingle( idz );
    }

    const int sz = pos.size();
    if ( mIsEqual(mdvals[sz-1],td,mDefEpsF) )
	return;

    Coord3 tdpos = pos[sz-1];
    tdpos.z += mCast(double,td) - mCast(double,mdvals[sz-1]);

    pos += tdpos;
    mdvals += td;
}


bool TrackAscIO::adjustSurfaceLocation( TypeSet<Coord3>& pos,
					Coord& surfacecoord ) const
{
    if ( pos.isEmpty() )
	return true;

    if ( mIsZero(surfacecoord.x,mDefEps) && mIsZero(surfacecoord.y,mDefEps) )
    {
	if ( mIsZero(pos[0].x,mDefEps) && mIsZero(pos[0].y,mDefEps) )
	    mErrRet( tr("Relative easting/northing found\n"
			"Please enter a valid surface coordinate in"
			" the advanced dialog") )

	surfacecoord = Coord( pos[0].x, pos[0].y );
	return true;
    }

    const double xshift = surfacecoord.x - pos[0].x;
    const double yshift = surfacecoord.y - pos[0].y;
    for ( int idz=0; idz<pos.size(); idz++ )
    {
	pos[idz].x += xshift;
	pos[idz].y += yshift;
    }

    return true;
}


bool TrackAscIO::getData( Data& wd, float kbelev, float td ) const
{
    TypeSet<Coord3> pos;
    TypeSet<float> mdvals;
    float kbelevinfile = mUdf(float);
    if ( !readTrackData(pos,mdvals,kbelevinfile) )
	return false;

    if ( mIsUdf(kbelev) && mIsUdf(kbelevinfile) )
	mErrRet( tr( "%1 was not provided and cannot be computed" )
		     .arg(Well::Info::sKeykbelev()) )

    if ( !computeMissingValues(pos,mdvals,kbelevinfile) )
	return false;

    adjustKBIfNecessary( pos, kbelevinfile, kbelev );
    addOriginIfNecessary( pos, mdvals );
    adjustToTDIfNecessary( pos, mdvals, td );
    if ( !adjustSurfaceLocation(pos,wd.info().surfacecoord) )
	return false;

    if ( pos.size() < 2 || mdvals.size() < 2 )
	mErrRet( tr("Insufficent data for importing the track") )

    wd.track().setEmpty();
    for ( int idz=0; idz<pos.size(); idz++ )
	wd.track().addPoint( pos[idz], mdvals[idz] );

    return !wd.track().isEmpty();
}


Table::TargetInfo* gtDepthTI( bool withuns )
{
    Table::TargetInfo* ti = new Table::TargetInfo( "Depth", FloatInpSpec(),
						   Table::Required );
    if ( withuns )
    {
	ti->setPropertyType( PropertyRef::Dist );
	ti->selection_.unit_ = UnitOfMeasure::surveyDefDepthUnit();
    }

    ti->form(0).setName( "MD" );
    ti->add( new Table::TargetInfo::Form( "TVDSS", FloatInpSpec() ) );
    return ti;
}


Table::FormatDesc* MarkerSetAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "MarkerSet" );

    fd->bodyinfos_ += gtDepthTI( true );

#define mAddNmSpec(nm,typ) \
    fd->bodyinfos_ += new Table::TargetInfo(nm,StringInpSpec(),Table::typ)
    mAddNmSpec( "Marker name", Required );
    mAddNmSpec( "Nm p2", Hidden );
    mAddNmSpec( "Nm p3", Hidden );
    mAddNmSpec( "Nm p4", Hidden );

    return fd;
}


bool MarkerSetAscIO::get( od_istream& strm, MarkerSet& ms,
				const Track& trck ) const
{
    ms.erase();

    const int dpthcol = columnOf( false, 0, 0 );
    const int nmcol = columnOf( false, 1, 0 );
    if ( nmcol > dpthcol )
    {
	// We'll assume that the name occupies up to 4 words
	for ( int icol=1; icol<4; icol++ )
	{
	    if ( fd_.bodyinfos_[icol+1]->selection_.elems_.isEmpty() )
		fd_.bodyinfos_[icol+1]->selection_.elems_ +=
		  Table::TargetInfo::Selection::Elem( RowCol(0,nmcol+icol), 0 );
	    else
		fd_.bodyinfos_[icol+1]->selection_.elems_[0].pos_.col()
		    = icol + nmcol;
	}
    }

    while( true )
    {
	const int ret = getNextBodyVals( strm );
	if ( ret < 0 ) return false;
	if ( ret == 0 ) break;

	float dah = mCast( float, getdValue( 0 ) );
	BufferString namepart = text( 1 );
	if ( mIsUdf(dah) || namepart.isEmpty() )
	    continue;
	if ( formOf(false,0) == 1 && !convToDah(trck,dah) )
	    continue;

	BufferString fullnm( namepart );
	for ( int icol=2; icol<5; icol++ )
	{
	    if ( icol == dpthcol ) break;
	    namepart = text( icol );
	    if ( namepart.isEmpty() ) break;

	    fullnm += " "; fullnm += namepart;
	}

	ms += new Marker( fullnm, dah );
    }

    return true;
}


Table::FormatDesc* D2TModelAscIO::getDesc( bool withunitfld )
{
    Table::FormatDesc* fd = new Table::FormatDesc( "DepthTimeModel" );
    fd->headerinfos_ +=
	new Table::TargetInfo( "Undefined Value",
				StringInpSpec(sKey::FloatUdf()),
				Table::Required );
    createDescBody( fd, withunitfld );
    return fd;
}


void D2TModelAscIO::createDescBody( Table::FormatDesc* fd,
					  bool withunits )
{
    Table::TargetInfo* ti = gtDepthTI( withunits );
    ti->add( new Table::TargetInfo::Form( "TVD rel SRD", FloatInpSpec() ) );
    ti->add( new Table::TargetInfo::Form( "TVD rel KB", FloatInpSpec() ) );
    ti->add( new Table::TargetInfo::Form( "TVD rel GL", FloatInpSpec() ) );
    fd->bodyinfos_ += ti;

    ti = new Table::TargetInfo( "Time", FloatInpSpec(), Table::Required,
				PropertyRef::Time );
    ti->form(0).setName( "TWT" );
    ti->add( new Table::TargetInfo::Form( "One-way TT", FloatInpSpec() ) );
    ti->selection_.unit_ = UoMR().get( "Milliseconds" );
    fd->bodyinfos_ += ti;
}


void D2TModelAscIO::updateDesc( Table::FormatDesc& fd, bool withunitfld )
{
    fd.bodyinfos_.erase();
    createDescBody( &fd, withunitfld );
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


static void checkReplacementVelocity( Well::Info& info, double vreplinfile,
				      uiString& msg )
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
	    const UnitOfMeasure* uomvel = UnitOfMeasure::surveyDefVelUnit();
	    const uiString veluomlbl(
		    UnitOfMeasure::surveyDefVelUnitAnnot(true,false) );
	    const BufferString fileval =
			       toString(mScaledValue(vreplinfile,uomvel), 2 );
	    msg = "Input error with the %1\n"
		  "Your time-depth model suggests a %1 of %2%4\n "
		  "but the %1 was set to: %3%4\n"
		  "Velocity information from file was overruled.";
	    msg.arg( replvelbl ).arg( fileval )
	       .arg( toString(mScaledValue(info.replvel,uomvel), 2) )
	       .arg( veluomlbl );
	}
    }
}


static void shiftTimesIfNecessary( TypeSet<double>& tvals, double wllheadz,
				   double vrepl, double origintwtinfile,
				   uiString& msg )
{
    if ( mIsUdf(origintwtinfile) )
	return;

    const double srddepth = -1. * SI().seismicReferenceDatum();
    const double origintwt = wllheadz < srddepth
			? 0.f : 2.f * ( srddepth - wllheadz) / vrepl;
    const double timeshift = origintwtinfile - origintwt;
    if ( mIsZero(timeshift,mDefEps) )
	return;

    msg = "Error with the input time-depth model:\n"
	  "It does not honour TWT(Z=SRD) = 0.";
    const UnitOfMeasure* uomz = UnitOfMeasure::surveyDefTimeUnit();
    msg.append(
	uiString( "\nOpendTect WILL correct for this error by applying a "
		  "time shift of: %1%2\n"
		  "The resulting travel-times will differ from the file")
		   .arg( toString(mScaledValue(timeshift,uomz),2) )
		   .arg(UnitOfMeasure::surveyDefTimeUnitAnnot(true,false) ) );

    for ( int idz=0; idz<tvals.size(); idz++ )
	tvals[idz] += timeshift;
}


static void convertDepthsToMD( const Track& track, TypeSet<double>& zvals )
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

static bool getTVDD2TModel( D2TModel& d2t, TypeSet<double>& zvals,
			    TypeSet<double>& tvals, const Data& wll,
			    uiString& errmsg, uiString& warnmsg )
{
    const Track& track = wll.track();
    int inputsz = zvals.size();

    if ( inputsz < 2 || inputsz != tvals.size() )
	mErrRet( "Input file does not contain at least two valid rows" );

    inputsz = sortAndEnsureUniqueTZPairs( zvals, tvals );
    if ( inputsz < 2 )
    {
	mErrRet( "Input file does not contain at least two valid rows"
		 "after resorting and removal of duplicated positions" );
    }

    const Interval<float> trackrg = track.zRange();
    const double zwllhead = mCast( double, trackrg.start );
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

    const double zstop = mCast( double, trackrg.stop );
    if ( !truncateToTD(zvals,tvals,zstop) )
	mErrRet( "Input file has not enough data points above TD" )

    removeDuplicatedVelocities( zvals, tvals );
    const double replvel = mCast( double, wll.info().replvel );
    shiftTimesIfNecessary( tvals, zwllhead, replvel, origintwtinfile, warnmsg );

    zvals.insert( 0, originz );
    tvals.insert( 0, kbabovesrd ? 0.f : 2. * ( zwllhead-srddepth ) / replvel );
    convertDepthsToMD( track, zvals );

    d2t.setEmpty();
    for ( int idx=0; idx<zvals.size(); idx++ )
	d2t.add( mCast(float,zvals[idx]), mCast(float,tvals[idx]) );

    return true;
}


bool D2TModelAscIO::get( od_istream& strm, D2TModel& d2t,
			 const Data& wll ) const
{
    if ( wll.track().isEmpty() ) return true;

    const int dpthopt = formOf( false, 0 );
    const int tmopt = formOf( false, 1 );
    const bool istvd = dpthopt > 0;
    TypeSet<double> zvals, tvals;
    while( true )
    {
	const int ret = getNextBodyVals( strm );
	if ( ret < 0 ) return false;
	if ( ret == 0 ) break;

	double zval = getdValue( 0 );
	double tval = getdValue( 1 );
	if ( mIsUdf(zval) || mIsUdf(tval) )
	    continue;
	if ( dpthopt == 2 )
	    zval -= SI().seismicReferenceDatum();
	if ( dpthopt == 3 )
	    zval -= wll.track().getKbElev();
	if ( dpthopt == 4 )
	    zval -= wll.info().groundelev;
	if ( tmopt == 1 )
	    tval *= 2;

	if ( !istvd )
	{
	    const Coord3 crd = wll.track().getPos( mCast(float,zval) );
	    if ( mIsUdf(crd) )
		continue;

	    zvals += crd.z; tvals += tval;
	}
	else
	{
	    zvals += zval;
	    tvals += tval;
	}
    }

    return getTVDD2TModel( d2t, zvals, tvals, wll, errmsg_, warnmsg_ );
}


// Well::BulkTrackAscIO
BulkTrackAscIO::BulkTrackAscIO( const Table::FormatDesc& fd,
				od_istream& strm )
    : Table::AscIO(fd)
    , strm_(strm)
{}


Table::FormatDesc* BulkTrackAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "BulkWellTrack" );
    fd->bodyinfos_ += new Table::TargetInfo( "Well name", Table::Required );
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true );
    fd->bodyinfos_ += Table::TargetInfo::mkDepthPosition( true );
    fd->bodyinfos_ +=
	new Table::TargetInfo( "MD", FloatInpSpec(), Table::Optional );
    fd->bodyinfos_ += new Table::TargetInfo( Well::Info::sKeyuwid(),
					     Table::Optional );
    return fd;
}


bool BulkTrackAscIO::get( BufferString& wellnm, Coord3& crd, float& md,
			  BufferString& uwi ) const
{
    const int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return false;

    wellnm = text( 0 );
    crd.x = getdValue( 1 );
    crd.y = getdValue( 2 );
    crd.z = getdValue( 3 );
    md = getfValue( 4 );
    uwi = text( 5 );
    return true;
}


Table::TargetInfo* gtWellNameTI()
{
    Table::TargetInfo* ti = new Table::TargetInfo( "Well identifier",
						   Table::Required );
    ti->form(0).setName( "Name" );
    ti->add( new Table::TargetInfo::Form("UWI",StringInpSpec()) );
    return ti;
}


// Well::BulkMarkerAscIO
BulkMarkerAscIO::BulkMarkerAscIO( const Table::FormatDesc& fd,
				od_istream& strm )
    : Table::AscIO(fd)
    , strm_(strm)
{}


Table::FormatDesc* BulkMarkerAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "BulkMarkerSet" );
    fd->bodyinfos_ += gtWellNameTI();
    fd->bodyinfos_ += gtDepthTI( true );
    fd->bodyinfos_ += new Table::TargetInfo( "Marker name", Table::Required );
    return fd;
}


bool BulkMarkerAscIO::get( BufferString& wellnm,
			   float& md, BufferString& markernm ) const
{
    const int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return false;

    wellnm = text( 0 );
    md = getfValue( 1 );
    markernm = text( 2 );
    return true;
}


bool BulkMarkerAscIO::identifierIsUWI() const
{ return formOf( false, 0 ) == 1; }


// Well::BulkD2TModelAscIO
BulkD2TModelAscIO::BulkD2TModelAscIO( const Table::FormatDesc& fd,
				      od_istream& strm )
    : Table::AscIO(fd)
    , strm_(strm)
{}


Table::FormatDesc* BulkD2TModelAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "BulkDepthTimeModel" );
    fd->bodyinfos_ += gtWellNameTI();

    Table::TargetInfo* ti = gtDepthTI( true );
    ti->add( new Table::TargetInfo::Form( "TVD rel SRD", FloatInpSpec() ) );
    ti->add( new Table::TargetInfo::Form( "TVD rel KB", FloatInpSpec() ) );
    ti->add( new Table::TargetInfo::Form( "TVD rel GL", FloatInpSpec() ) );
    fd->bodyinfos_ += ti;

    ti = new Table::TargetInfo( "Time", FloatInpSpec(), Table::Required,
				PropertyRef::Time );
    ti->form(0).setName( "TWT" );
    ti->add( new Table::TargetInfo::Form( "One-way TT", FloatInpSpec() ) );
    ti->selection_.unit_ = UoMR().get( "Milliseconds" );
    fd->bodyinfos_ += ti;
    return fd;
}


bool BulkD2TModelAscIO::get( BufferString& wellnm,
			     float& md, float& twt ) const
{
    const int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return false;

    wellnm = text( 0 );
    md = getfValue( 1 );
    twt = getfValue( 2 );
    return true;
}


bool BulkD2TModelAscIO::identifierIsUWI() const
{ return formOf( false, 0 ) == 1; }

} // namespace Well
