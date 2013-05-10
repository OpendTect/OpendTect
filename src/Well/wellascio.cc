/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Nanne Hemstra
 * DATE     : May 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "wellimpasc.h"

#include "sorting.h"
#include "tabledef.h"
#include "unitofmeasure.h"
#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "idxable.h"

//#include <iostream>
//#include <math.h>


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


bool TrackAscIO::getData( Data& wd, bool tosurf ) const
{
    if ( !getHdrVals(strm_) )
	return false;

    static const Coord3 c000( 0, 0, 0 );
    Coord3 c, prevc;
    Coord3 surfcoord;
    float dah = 0;

    const bool isxy = fd_.bodyinfos_[0]->selection_.form_ == 0;

    while ( true )
    {
	const int ret = getNextBodyVals( strm_ );
	if ( ret < 0 ) return false;
	if ( ret == 0 ) break;

	c.x = getdValue(0); c.y = getdValue(1);
	if ( !isxy && !mIsUdf(c.x) && !mIsUdf(c.y) )
	{
	    Coord wc( SI().transform( BinID( mNINT32(c.x), mNINT32(c.y) ) ) );
	    c.x = wc.x; c.y = wc.y;
	}
	if ( mIsUdf(c.x) || mIsUdf(c.y) )
	    continue;

	c.z = mCast( float, getdValue(2) );
	const float newdah = mCast( float, getdValue( 3 ) );
	const bool havez = !mIsUdf(c.z);
	const bool havedah = !mIsUdf(newdah);
	if ( !havez && !havedah )
	    continue;

	if ( wd.track().size() == 0 )
	{
	    if ( !SI().isReasonable(wd.info().surfacecoord) )
		wd.info().surfacecoord = c;
//		wd.info().SRDelev = 0;  user input required

	    surfcoord.x = wd.info().surfacecoord.x;
	    surfcoord.y = wd.info().surfacecoord.y;
	    surfcoord.z = c.z;

	    prevc = tosurf && c.z >=0 ? surfcoord : c;
	}

	if ( mIsUdf(newdah) )
	    dah += (float) c.distTo( prevc );
	else
	{
	    if ( mIsUdf(c.z) )
	    {
		float d = newdah - dah;
		const float hdist = (float)Coord(c).distTo( Coord(prevc) );
		c.z = prevc.z;
		if ( d > hdist )
		    c.z += Math::Sqrt( d*d - hdist*hdist );
	    }
	    dah = newdah;
	}

	if ( c.distTo(c000) < 1 )
	    break;

	wd.track().addPoint( c, (float) c.z, dah );
	prevc = c;
    }

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


bool MarkerSetAscIO::get( std::istream& strm, MarkerSet& ms,
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
		fd_.bodyinfos_[icol+1]->selection_.elems_[0].pos_.col
		    = icol + nmcol;
	}
    }

    while ( true )
    {
	int ret = getNextBodyVals( strm );
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
    fd->bodyinfos_ += ti;
}


void D2TModelAscIO::updateDesc( Table::FormatDesc& fd, bool withunitfld )
{
    fd.bodyinfos_.erase();
    createDescBody( &fd, withunitfld );
}


static bool getTVDD2TModel( D2TModel& d2t, TypeSet<double>& rawzvals,
       			    TypeSet<double>& rawtvals, const Data& wll )
{
    const Track& trck = wll.track();
    int inputsz = rawzvals.size();

    if ( inputsz < 2 || inputsz != rawtvals.size() )
	return false;

    // sort and remove duplicates
    mAllocVarLenIdxArr( int, idxs, inputsz );
    sort_coupled( rawzvals.arr(), mVarLenArr(idxs), inputsz );
    TypeSet<double> zvals, tvals;
    zvals += rawzvals[0];
    tvals += rawtvals[idxs[0]];
    for ( int idx=1; idx<inputsz; idx++ )
    {
	const int lastidx = zvals.size()-1;
	const bool samez = mIsEqual( rawzvals[idx], zvals[lastidx], 1e-6 );
	const bool reversedtwt = tvals[lastidx] - rawtvals[idxs[idx]] > 1e-6;
	if ( !samez && !reversedtwt )
	{
	    zvals += rawzvals[idx];
	    tvals += rawtvals[idxs[idx]];
	}
    }

    inputsz = zvals.size();
    if ( inputsz < 2 )
	return false;

    TypeSet<float> mds;
    TypeSet<double> ts;
    const double zwllhead = trck.pos(0).z;
    const double srd = wll.info().srdelev;
    const double firstz = mMAX(-1.f * srd, zwllhead );
    // no write above deepest of (well head, SRD)
    // velocity above is controled by info().replvel

    int istartz = IdxAble::getLowIdx( zvals, inputsz, firstz );
    if ( istartz < 0  )
	istartz = 0;
    else if ( istartz >= (inputsz-1) )
	istartz--;

    double curvel = ( zvals[istartz+1] - zvals[istartz] ) /
       		    ( tvals[istartz+1] - tvals[istartz] );
    mds += trck.getDahForTVD(mCast(float,firstz));
    ts  += -1.f * srd > zwllhead ? 0 : 2.f * ( zwllhead + srd ) /
       				       mCast( double, wll.info().replvel );
    // one SHOULD check here if this time corresponds to the time at the
    // same depth in the input file, i.e. is the computed replacement velocity
    // in line with the one stored in info() or input in the advanced import
    // settings window

    int prevvelidx = istartz;
    istartz++;
    for ( int idz=istartz; idz<inputsz; idz++ )
    {
	const double newvel = ( zvals[idz] - zvals[prevvelidx] ) /
	   		      ( tvals[idz] - tvals[prevvelidx] );
	if ( mIsEqual(curvel,newvel,1e-2) && (idz<inputsz-1) )
	   continue;

	const float dah = trck.getDahForTVD( mCast(float,zvals[idz]) );
	if ( !mIsUdf(dah) )
	{
	    prevvelidx = idz;
	    curvel = newvel;
	    mds += trck.getDahForTVD( mCast(float,zvals[idz]) );
	    ts += tvals[idz];
	}
    }

    const int outsz = mds.size();

    if ( outsz < 2 )
	return false;

    for ( int idx=0; idx<outsz; idx++ )
	d2t.add( mds[idx], (float)ts[idx] );

    return true;
}


bool D2TModelAscIO::get( std::istream& strm, D2TModel& d2t,
			 const Data& wll ) const
{
    d2t.setEmpty();
    if ( wll.track().isEmpty() ) return true;

    const int dpthopt = formOf( false, 0 );
    const int tmopt = formOf( false, 1 );
    const bool istvd = dpthopt > 0;
    TypeSet<double> zvals, tvals;
    while ( true )
    {
	int ret = getNextBodyVals( strm );
	if ( ret < 0 ) return false;
	if ( ret == 0 ) break;

	double zval = getdValue( 0 );
	double tval = getdValue( 1 );
	if ( mIsUdf(zval) || mIsUdf(tval) )
	    continue;
	if ( dpthopt == 2 )
	    zval -= wll.info().srdelev;
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

	    zvals += crd.z;
	}
	else
	    zvals += zval;

	tvals += tval;
    }

    return getTVDD2TModel( d2t, zvals, tvals, wll );
}


// Well::BulkTrackAscIO
BulkTrackAscIO::BulkTrackAscIO( const Table::FormatDesc& fd,
				std::istream& strm )
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
    fd->bodyinfos_ += new Table::TargetInfo( "Well ID (UWI)", Table::Optional );
    return fd;
}


bool BulkTrackAscIO::get( BufferString& wellnm, Coord3& crd, float md,
			  BufferString& uwi ) const
{
    const int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return false;

    wellnm = text( 0 );
    crd.x = getdValue( 1 );
    crd.y = getdValue( 2 );
    crd.z = getfValue( 3 );
    md = getfValue( 4 );
    uwi = text( 5 );
    return true;
}


// Well::BulkMarkerAscIO
BulkMarkerAscIO::BulkMarkerAscIO( const Table::FormatDesc& fd,
				std::istream& strm )
    : Table::AscIO(fd)
    , strm_(strm)
{}


Table::FormatDesc* BulkMarkerAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "BulkWellMarkers" );
    fd->bodyinfos_ += gtDepthTI( true );
    fd->bodyinfos_ += new Table::TargetInfo( "Marker name", Table::Required );
    fd->bodyinfos_ += new Table::TargetInfo( "Well name", Table::Optional );
    fd->bodyinfos_ += new Table::TargetInfo( "Well ID (UWI)", Table::Optional );
    return fd;
}


bool BulkMarkerAscIO::get( BufferString& wellnm, float md,
			  BufferString& uwi ) const
{
    const int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return false;

    wellnm = text( 0 );
    md = getfValue( 4 );
    uwi = text( 5 );
    return true;
}

} // namespace Well
