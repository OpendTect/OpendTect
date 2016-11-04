/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : May 2013
-*/


#include "wellimpasc.h"

#include "idxable.h"
#include "ioobj.h"
#include "mathfunc.h"
#include "od_ostream.h"
#include "sorting.h"
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
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true );
    Table::TargetInfo* zti = Table::TargetInfo::mkDepthPosition( false );
    zti->setName( "Z (TVDSS)" );
    fd->bodyinfos_ += zti;
    Table::TargetInfo* ti = new Table::TargetInfo( "MD", FloatInpSpec(),
						   Table::Optional );
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

    const bool isxy = fd_.bodyinfos_[0]->selection_.form_ == 0;
    const uiString nozpts = tr("At least one point had neither Z nor MD");
    bool nozptsfound = false;

    while( true )
    {
	const int ret = getNextBodyVals( strm_ );
	if ( ret < 0 ) return false;
	if ( ret == 0 ) break;

	Coord3 curpos;
	curpos.x_ = getDValue(0);
	curpos.y_ = getDValue(1);
	if ( !isxy && !mIsUdf(curpos.x_) && !mIsUdf(curpos.y_) )
	{
	    Coord wc( SI().transform(
			BinID( mNINT32(curpos.x_), mNINT32(curpos.y_) ) ) );
	    curpos.x_ = wc.x_; curpos.y_ = wc.y_;
	}
	if ( mIsUdf(curpos.x_) || mIsUdf(curpos.y_) )
	    continue;

	curpos.z_ = getDValue(2);
	const double dah = getDValue(3);
	if ( mIsUdf(curpos.z_) && mIsUdf(dah) )
	{
	    if ( !nozptsfound )
		warnmsg_.append( nozpts, true );
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
    const uiString uomlbl(UnitOfMeasure::surveyDefDepthUnitAnnot(true,false) );
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
		const BufferString val = toString( mScaledValue( dah, uom), 2 );
		mErrRet( tr("Impossible MD to TVD transformation for MD=%1%2" )
			 .arg(val)
			 .arg(uomlbl) )
	    }

	    curpos.z_ = prevpos.z_ + Math::Sqrt( dist*dist - hdist*hdist );
	}
	else if ( mIsUdf(dah) )
	{
	    const double dist = curpos.distTo<double>( prevpos );
	    if ( dist < 0. )
	    {
		const BufferString val =
				toString(mScaledValue(curpos.z_,uom),2);
		mErrRet( tr( "Impossible TVD to MD transformation for Z=%1%2" )
			  .arg(val)
			  .arg(uomlbl) )
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
	mErrRet( tr( "%1 was not provided and cannot be computed" )
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


Table::FormatDesc* Well::MarkerSetAscIO::getDesc()
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
	new Table::TargetInfo( "Undefined Value",
				StringInpSpec(sKey::FloatUdf()),
				Table::Required );
    createDescBody( fd, withunitfld );
    return fd;
}


void Well::D2TModelAscIO::createDescBody( Table::FormatDesc* fd,
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

    return d2t.ensureValid( wll, errmsg_, &zvals, &tvals );
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
    fd->bodyinfos_ += new Table::TargetInfo( "Well name", Table::Required );
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true );
    Table::TargetInfo* zti = Table::TargetInfo::mkDepthPosition( true );
    zti->setName( "Z (TVDSS)" );
    fd->bodyinfos_ += zti;
    Table::TargetInfo* mdti =
	new Table::TargetInfo( "MD", FloatInpSpec(), Table::Optional,
				PropertyRef::Dist );
    mdti->selection_.unit_ = UnitOfMeasure::surveyDefDepthUnit();
    fd->bodyinfos_ += mdti;
    fd->bodyinfos_ += new Table::TargetInfo( Well::Info::sKeyUwid(),
					     Table::Optional );
    return fd;
}


bool Well::BulkTrackAscIO::get( BufferString& wellnm, Coord3& crd, float& md,
				BufferString& uwi ) const
{
    const int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return false;

    wellnm = text( 0 );
    crd.x_ = getDValue( 1 );
    crd.y_ = getDValue( 2 );
    crd.z_ = getDValue( 3 );
    md = getFValue( 4 );
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
    fd->bodyinfos_ += new Table::TargetInfo( "Marker name", Table::Required );
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


bool Well::BulkD2TModelAscIO::get( BufferString& wellnm, float& md, float& twt )
{
    const int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return false;

    wellnm = text( 0 );
    md = getFValue( 1 );
    twt = getFValue( 2 );
    if ( mIsUdf(md) || mIsUdf(twt) )
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

    const int dpthopt = formOf( false, 0 );
    const int tmopt = formOf( false, 1 );

    if ( dpthopt == 0 )
	md = mCast(float,wellsdata_[wellidx]->track().getPos(md).z_);
    if ( dpthopt == 2 )
	md -= SI().seismicReferenceDatum();
    if ( dpthopt == 3 )
	md -= wellsdata_[wellidx]->track().getKbElev();
    if ( dpthopt == 4 )
	md -= wellsdata_[wellidx]->info().groundElevation();
    if ( tmopt == 1 )
	twt *= 2;

    return true;
}


bool Well::BulkD2TModelAscIO::identifierIsUWI() const
{
    return formOf( false, 0 ) == 1;
}
