/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : May 2013
-*/


#include "wellimpasc.h"

#include "coordsystem.h"
#include "directionalsurvey.h"
#include "idxable.h"
#include "ioobj.h"
#include "mathfunc.h"
#include "od_ostream.h"
#include "sorting.h"
#include "keystrs.h"
#include "tabledef.h"
#include "unitofmeasure.h"
#include "welldata.h"
#include "wellinfo.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "wellmanager.h"


static bool convToDah( const Well::Track& trck, float& val,
		       float prevdah=mUdf(float) )
{
    val = trck.getDahForTVD( val, prevdah );
    return !mIsUdf(val);
}


Table::FormatDesc* Well::TrackAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "WellTrack" );
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true, false, true );
    Table::TargetInfo* zti = Table::TargetInfo::mkDepthPosition( false );
    zti->setName( "Z (TVDSS)" );
    fd->bodyinfos_ += zti;
    Table::TargetInfo* ti = new Table::TargetInfo( uiStrings::sMD(),
					FloatInpSpec(), Table::Optional );
    ti->setPropertyType( PropertyRef::Dist );
    ti->selection_.unit_ = UnitOfMeasure::surveyDefDepthUnit();
    fd->bodyinfos_ += ti;
    return fd;
}


bool Well::TrackAscIO::readTrackData( TypeSet<Coord3>& pos,
		    TypeSet<double>& mdvals, double& kbelevinfile ) const
{
    if ( !getHdrVals(strm_) )
	return false;

    const uiString nozpts = tr("At least one point had neither Z nor MD");
    bool nozptsfound = false;

    while( true )
    {
	const int ret = getNextBodyVals( strm_ );
	if ( ret < 0 ) return false;
	if ( ret == 0 ) break;

	Coord3 curpos( getPos3D(0,1,2) );
	if ( curpos.getXY().isUdf() )
	    continue;
	const double dah = getDValue(3);
	if ( mIsUdf(curpos.z_) && mIsUdf(dah) )
	{
	    if ( !nozptsfound )
		warnmsg_.appendPhrase( nozpts );
	    else
		nozptsfound = true;

	    continue;
	}

	pos += curpos;
	mdvals += dah;
	if ( mIsUdf(kbelevinfile) && !mIsUdf(curpos.z_) && !mIsUdf(dah) )
	    kbelevinfile = dah - curpos.z_;
    }

    return !pos.isEmpty();
}


#define mErrRet(s) { errmsg_ = s; return false; }
#define mScaledValue(s,uom) ( uom ? uom->userValue(s) : s )

bool Well::TrackAscIO::computeMissingValues( TypeSet<Coord3>& pos,
					     TypeSet<double>& mdvals,
					     double& kbelevinfile ) const
{
    if ( pos.isEmpty() || mdvals.isEmpty() || pos.size() != mdvals.size() )
	return false;

    Coord3 prevpos = pos[0];
    double prevdah = mdvals[0];
    if ( mIsUdf(prevpos.z_) && mIsUdf(prevdah) )
	return false;

    if ( mIsUdf(kbelevinfile) )
	kbelevinfile = 0.;

    if ( mIsUdf(prevpos.z_) )
    {
	prevpos.z_ = prevdah - kbelevinfile;
	pos[0].z_ = prevpos.z_;
    }

    if ( mIsUdf(prevdah) )
    {
	prevdah = prevpos.z_ + kbelevinfile;
	mdvals[0] = prevdah;
    }

    const UnitOfMeasure* uom = UnitOfMeasure::surveyDefDepthUnit();
    const uiString uomlbl(UnitOfMeasure::surveyDefDepthUnitAnnot(true) );
    for ( int idz=1; idz<pos.size(); idz++ )
    {
	Coord3& curpos = pos[idz];
	double& dah = mdvals[idz];
	if ( mIsUdf(curpos) && mIsUdf(dah) )
	    return false;
	else if ( mIsUdf(curpos) )
	{
	    const double dist = dah - prevdah;
	    const double hdist = curpos.xyDistTo<double>( prevpos );
	    if ( dist < hdist )
	    {
		const double usrval = mScaledValue( dah, uom );
		mErrRet( tr("Impossible MD to TVD transformation for MD=%1" )
			 .arg(usrval).withUnit(uomlbl) )
	    }

	    curpos.z_ = prevpos.z_ + Math::Sqrt( dist*dist - hdist*hdist );
	}
	else if ( mIsUdf(dah) )
	{
	    const double dist = curpos.distTo<double>( prevpos );
	    if ( dist < 0. )
	    {
		const double usrval = mScaledValue( curpos.z_, uom );
		mErrRet( tr("Impossible TVD to MD transformation for Z=%1")
			  .arg(usrval).withUnit(uomlbl) )
	    }

	    dah = prevdah + dist;
	}
	prevpos = curpos;
	prevdah = dah;
    }

    return true;
}


static void adjustKBIfNecessary( TypeSet<Coord3>& pos, double kbelevinfile,
				 double kbelev )
{
    if ( mIsUdf(kbelev) || mIsUdf(kbelevinfile) )
	return;

    const double kbshift = kbelev - kbelevinfile;
    for ( int idz=0; idz<pos.size(); idz++ )
    {
	if ( !mIsUdf(pos[idz].z_) )
	    pos[idz].z_ -= kbshift;
    }
}


#define mDefEpsZ 1e-3

static void addOriginIfNecessary( TypeSet<Coord3>& pos, TypeSet<double>& mdvals)
{
    if ( mdvals.isEmpty() || mdvals[0] < mDefEpsZ )
	return;

    Coord3 surfloc = pos[0];
    surfloc.z_ -= mdvals[0];

    pos.insert( 0, surfloc );
    mdvals.insert( 0, 0. );
}


static void adjustToTDIfNecessary( TypeSet<Coord3>& pos,
				   TypeSet<double>& mdvals, double td )
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
    if ( mIsEqual(mdvals[sz-1],td,mDefEpsZ) )
	return;

    Coord3 tdpos = pos[sz-1];
    tdpos.z_ += td - mdvals[sz-1];

    pos += tdpos;
    mdvals += td;
}


bool Well::TrackAscIO::adjustSurfaceLocation( TypeSet<Coord3>& pos,
						Coord& surfacecoord ) const
{
    if ( pos.isEmpty() )
	return true;

    if ( mIsZero(surfacecoord.x_,mDefEps) && mIsZero(surfacecoord.y_,mDefEps) )
    {
	if ( mIsZero(pos[0].x_,mDefEps) && mIsZero(pos[0].y_,mDefEps) )
	{
	    mErrRet( tr("Relative easting/northing found\n"
			"Please enter a valid surface coordinate in"
			" the advanced dialog") )
	}

	surfacecoord = Coord( pos[0].x_, pos[0].y_ );
	return true;
    }

    const double xshift = surfacecoord.x_ - pos[0].x_;
    const double yshift = surfacecoord.y_ - pos[0].y_;
    for ( int idz=0; idz<pos.size(); idz++ )
    {
	pos[idz].x_ += xshift;
	pos[idz].y_ += yshift;
    }

    return true;
}


bool Well::TrackAscIO::getData( Data& wd, float kbelev, float td ) const
{
    TypeSet<Coord3> pos;
    TypeSet<double> mdvals;
    double kbelevinfile = mUdf(double);
    if ( !readTrackData(pos,mdvals,kbelevinfile) )
	return false;

    if ( mIsUdf(kbelev) && mIsUdf(kbelevinfile) )
	mErrRet( tr("%1 was not provided and cannot be calculated")
		     .arg(Well::Info::sKBElev()) )

    if ( !computeMissingValues(pos,mdvals,kbelevinfile) )
	return false;

    adjustKBIfNecessary( pos, kbelevinfile, mCast(double,kbelev) );
    addOriginIfNecessary( pos, mdvals );
    adjustToTDIfNecessary( pos, mdvals, mCast(double,td) );
    Coord surfcoord( wd.info().surfaceCoord() );
    if ( !adjustSurfaceLocation(pos,surfcoord) )
	return false;
    else
	wd.info().setSurfaceCoord( surfcoord );

    if ( pos.size() < 2 || mdvals.size() < 2 )
	mErrRet( tr("Insufficent data for importing the track") )

    wd.track().setEmpty();
    for ( int idz=0; idz<pos.size(); idz++ )
	wd.track().addPoint( pos[idz], mCast(float,mdvals[idz]) );

    return !wd.track().isEmpty();
}


static Table::TargetInfo* gtDepthTI( bool withuns )
{
    Table::TargetInfo* ti = new Table::TargetInfo( uiStrings::sDepth(),
					    FloatInpSpec(), Table::Required );
    if ( withuns )
    {
	ti->setPropertyType( PropertyRef::Dist );
	ti->selection_.unit_ = UnitOfMeasure::surveyDefDepthUnit();
    }

    ti->form(0).setName( sKey::MD() );
    ti->add( new Table::TargetInfo::Form( uiStrings::sTVDSS(),
							FloatInpSpec() ) );
    return ti;
}


Table::FormatDesc* Well::MarkerSetAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "MarkerSet" );

    fd->bodyinfos_ += gtDepthTI( true );

#define mAddNmSpec(nm,typ) \
    fd->bodyinfos_ += new Table::TargetInfo(nm,StringInpSpec(),Table::typ)
    mAddNmSpec( uiStrings::sMarkerNm(), Required );
    mAddNmSpec( toUiString("Nm p2"), Hidden );
    mAddNmSpec( toUiString("Nm p3"), Hidden );
    mAddNmSpec( toUiString("Nm p4"), Hidden );

    return fd;
}


bool Well::MarkerSetAscIO::get( od_istream& strm, MarkerSet& ms,
				const Track& trck ) const
{
    ms.setEmpty();

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

	float dah = mCast( float, getDValue( 0 ) );
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

	ms.add( Marker(fullnm,dah) );
    }

    return true;
}


Table::FormatDesc* Well::D2TModelAscIO::getDesc( bool withunitfld )
{
    Table::FormatDesc* fd = new Table::FormatDesc( "DepthTimeModel" );
    fd->headerinfos_ +=
	new Table::TargetInfo( uiStrings::sUndefVal(),
				StringInpSpec(sKey::FloatUdf()),
				Table::Required );
    createDescBody( fd, withunitfld );
    return fd;
}


void Well::D2TModelAscIO::createDescBody( Table::FormatDesc* fd,
					  bool withunits )
{
    Table::TargetInfo* ti = gtDepthTI( withunits );
    ti->add( new Table::TargetInfo::Form( uiStrings::sTVDRelSRD(),
							    FloatInpSpec() ) );
    ti->add( new Table::TargetInfo::Form( uiStrings::sTVDRelKB(),
							    FloatInpSpec() ) );
    ti->add( new Table::TargetInfo::Form( uiStrings::sTVDRelGL(),
							    FloatInpSpec() ) );
    fd->bodyinfos_ += ti;

    ti = new Table::TargetInfo( uiStrings::sTime(), FloatInpSpec(),
					Table::Required, PropertyRef::Time );
    ti->form(0).setName( sKey::TWT() );
    ti->add( new Table::TargetInfo::Form( uiStrings::sOneWayTT(),
							    FloatInpSpec() ) );
    ti->selection_.unit_ = UoMR().get( "Milliseconds" );
    fd->bodyinfos_ += ti;
}


void Well::D2TModelAscIO::updateDesc( Table::FormatDesc& fd, bool withunitfld )
{
    fd.bodyinfos_.erase();
    createDescBody( &fd, withunitfld );
}


bool Well::D2TModelAscIO::get( od_istream& strm, D2TModel& d2t,
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

	double zval = getDValue( 0 );
	double tval = getDValue( 1 );
	if ( mIsUdf(zval) || mIsUdf(tval) )
	    continue;
	if ( dpthopt == 2 )
	    zval -= SI().seismicReferenceDatum();
	if ( dpthopt == 3 )
	    zval -= wll.track().getKbElev();
	if ( dpthopt == 4 )
	    zval -= wll.info().groundElevation();
	if ( tmopt == 1 )
	    tval *= 2;

	if ( !istvd )
	{
	    const Coord3 crd = wll.track().getPos( mCast(float,zval) );
	    if ( mIsUdf(crd) )
		continue;

	    zvals += crd.z_;
	}
	else
	    zvals += zval;

	tvals += tval;
    }

    const bool res = d2t.ensureValid( wll, errmsg_, &zvals, &tvals );
    if ( res && !errmsg_.isEmpty() )
	warnmsg_ = errmsg_;

    return res;
}


// Well::BulkTrackAscIO
Well::BulkTrackAscIO::BulkTrackAscIO( const Table::FormatDesc& fd,
				      od_istream& strm )
    : Table::AscIO(fd)
    , strm_(strm)
{}


Table::FormatDesc* Well::BulkTrackAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "BulkWellTrack" );
    fd->bodyinfos_ +=
	new Table::TargetInfo( uiStrings::sWellName(), Table::Required );
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true, false, true );
    Table::TargetInfo* zti = Table::TargetInfo::mkDepthPosition( true );
    zti->form(0).setName( "Z (TVDSS)" );
    zti->add( new Table::TargetInfo::Form(uiStrings::sTVD(),FloatInpSpec()) );
    fd->bodyinfos_ += zti;
    Table::TargetInfo* mdti =
	new Table::TargetInfo( uiStrings::sMD(), FloatInpSpec(),
					Table::Optional, PropertyRef::Dist );
    mdti->selection_.unit_ = UnitOfMeasure::surveyDefDepthUnit();
    fd->bodyinfos_ += mdti;
    fd->bodyinfos_ +=
	new Table::TargetInfo( Well::Info::sUwid(), Table::Optional );
    return fd;
}


bool Well::BulkTrackAscIO::depthIsTVD() const
{
    return formOf( false, 2 ) == 1;
}


bool Well::BulkTrackAscIO::get( BufferString& wellnm, Coord3& crd, float& md,
				BufferString& uwi ) const
{
    const int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return false;

    wellnm = text( 0 ); // how are they determining the if its xy or inl/crl
    crd = getPos3D( 1, 2, 3 );
    md = getFValue( 4 );
    uwi = text( 5 );
    return true;
}


Table::TargetInfo* gtWellNameTI()
{
    Table::TargetInfo* ti = new Table::TargetInfo( od_static_tr("gtWellNameTI",
					"Well identifier"), Table::Required );
    ti->form(0).setName( "Name" );
    ti->add( new Table::TargetInfo::Form( od_static_tr("gtWellNameTI", "UWI",
					"Unique Well ID"), StringInpSpec()) );
    return ti;
}


// Well::BulkMarkerAscIO
Well::BulkMarkerAscIO::BulkMarkerAscIO( const Table::FormatDesc& fd,
				od_istream& strm )
    : Table::AscIO(fd)
    , strm_(strm)
{}


Table::FormatDesc* Well::BulkMarkerAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "BulkMarkerSet" );
    fd->bodyinfos_ += gtWellNameTI();
    fd->bodyinfos_ += gtDepthTI( true );
    fd->bodyinfos_ += new Table::TargetInfo( uiStrings::sMarkerNm(),
							    Table::Required );
    return fd;
}


bool Well::BulkMarkerAscIO::get( BufferString& wellnm,
			   float& md, BufferString& markernm ) const
{
    const int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return false;

    wellnm = text( 0 );
    md = getFValue( 1 );
    markernm = text( 2 );
    return true;
}


bool Well::BulkMarkerAscIO::identifierIsUWI() const
{ return formOf( false, 0 ) == 1; }


// Well::BulkD2TModelAscIO
Well::BulkD2TModelAscIO::BulkD2TModelAscIO( const Table::FormatDesc& fd,
					    od_istream& strm )
    : Table::AscIO(fd)
    , strm_(strm)
{}


Table::FormatDesc* Well::BulkD2TModelAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "BulkDepthTimeModel" );
    fd->bodyinfos_ += gtWellNameTI();

    Table::TargetInfo* ti = gtDepthTI( true );
    ti->add( new Table::TargetInfo::Form( uiStrings::sTVDRelSRD(),
							FloatInpSpec() ) );
    ti->add( new Table::TargetInfo::Form( uiStrings::sTVDRelKB(),
							    FloatInpSpec() ) );
    ti->add( new Table::TargetInfo::Form( uiStrings::sTVDRelGL(),
							    FloatInpSpec() ) );
    fd->bodyinfos_ += ti;

    ti = new Table::TargetInfo( uiStrings::sTime(), FloatInpSpec(),
					  Table::Required, PropertyRef::Time );
    ti->form(0).setName( sKey::TWT() );
    ti->add( new Table::TargetInfo::Form( uiStrings::sOneWayTT(),
							    FloatInpSpec() ) );
    ti->selection_.unit_ = UoMR().get( "Milliseconds" );
    fd->bodyinfos_ += ti;
    return fd;
}


bool Well::BulkD2TModelAscIO::get( BufferString& wellnm, float& zval, float& twt )
{
    const int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return false;

    wellnm = text( 0 );
    zval = getFValue( 1 );
    twt = getFValue( 2 );
    if ( mIsUdf(zval) || mIsUdf(twt) )
	return false;

    int wellidx = wells_.indexOf(wellnm);
    if ( wellidx < 0 )
    {
	const DBKey dbky = MGR().getIDByName( wellnm );
	if ( dbky.isInvalid() )
	    return false;
	ConstRefMan<Data> wd = MGR().fetch( dbky, LoadReqs(Trck).add(D2T) );
	if ( !wd || wd->track().isEmpty() )
	    return false;

	wellsdata_ += wd;
	wells_.add( wellnm );
	wellidx = wells_.size()-1;
    }

    const int dpthopt = formOf( false, 1 );
    const int tmopt = formOf( false, 2 );

    if ( dpthopt == 0 )
	zval = mCast(float,wellsdata_[wellidx]->track().getPos(zval).z_);
    if ( dpthopt == 2 )
	zval -= SI().seismicReferenceDatum();
    if ( dpthopt == 3 )
	zval -= wellsdata_[wellidx]->track().getKbElev();
    if ( dpthopt == 4 )
	zval -= wellsdata_[wellidx]->info().groundElevation();
    if ( tmopt == 1 )
	twt *= 2;

    return true;
}


bool Well::BulkD2TModelAscIO::identifierIsUWI() const
{
    return formOf( false, 0 ) == 1;
}


// DirectionalAscIO
Well::DirectionalAscIO::DirectionalAscIO( const Table::FormatDesc& fd,
					  od_istream& strm )
    : Table::AscIO(fd)
    , strm_(strm)
{}


Table::FormatDesc* Well::DirectionalAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Directional Survey" );
    Table::TargetInfo* ti = new Table::TargetInfo( uiStrings::sMD(),
						   DoubleInpSpec(),
						   Table::Required );
    ti->setPropertyType( PropertyRef::Dist );
    ti->selection_.unit_ = UnitOfMeasure::surveyDefDepthUnit();
    fd->bodyinfos_ += ti;

    fd->bodyinfos_ += new Table::TargetInfo( uiStrings::sInclination(),
					     DoubleInpSpec(),
					     Table::Required );
    fd->bodyinfos_ += new Table::TargetInfo( uiStrings::sAzimuth(),
					     DoubleInpSpec(),
					     Table::Required );
    return fd;
}


bool Well::DirectionalAscIO::readFile( TypeSet<double>& mdvals,
				       TypeSet<double>& incls,
				       TypeSet<double>& azis ) const
{
    if ( !getHdrVals(strm_) )
	return false;

    while( true )
    {
	const int ret = getNextBodyVals( strm_ );
	if ( ret < 0 ) return false;
	if ( ret == 0 ) break;

	const double md = getDValue(0);
	const double incl = getDValue(1);
	const double azi = getDValue(2);

	if ( mIsUdf(md) )
	    continue;

	mdvals += md;
	incls += incl;
	azis += azi;
    }

    return !mdvals.isEmpty();
}


bool Well::DirectionalAscIO::getData( Data& wd, float kb ) const
{
    TypeSet<double> mdvals;
    TypeSet<double> incls;
    TypeSet<double> azis;
    if ( !readFile(mdvals,incls,azis) )
	return false;

    if ( mdvals.size() < 2 )
	mErrRet( tr("Insufficent data for importing the track") )

    if ( mIsUdf(kb) )
	kb = 0;

    TypeSet<Coord3> track;
    Well::DirectionalSurvey dirsurvey( wd.info().surfaceCoord(), kb );
    dirsurvey.calcTrack( mdvals, incls, azis, track );

    wd.track().setEmpty();
    for ( int idz=0; idz<mdvals.size(); idz++ )
	wd.track().addPoint( track[idz], mCast(float,mdvals[idz]) );

    return !wd.track().isEmpty();
}
