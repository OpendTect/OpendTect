/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID mUnusedVar = "$Id: wellimpasc.cc,v 1.92 2012-08-08 05:03:40 cvsaneesh Exp $";

#include "wellimpasc.h"
#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "strmprov.h"
#include "unitofmeasure.h"
#include "tabledef.h"
#include "varlenarray.h"
#include "sorting.h"
#include <iostream>
#include <math.h>


static bool convToDah( const Well::Track& trck, float& val,
			float prevdah=mUdf(float) )
{
    val = trck.getDahForTVD( val, prevdah );
    return !mIsUdf(val);
}


inline static StreamData getSD( const char* fnm )
{
    StreamProvider sp( fnm );
    StreamData sd = sp.makeIStream();
    return sd;
}


Well::LASImporter::~LASImporter()
{
    unitmeasstrs_.erase();
}


#define mOpenFile(fnm) \
	StreamProvider sp( fnm ); \
	StreamData sd = sp.makeIStream(); \
	if ( !sd.usable() ) \
	    return "Cannot open input file"


const char* Well::LASImporter::getLogInfo( const char* fnm,
					   FileInfo& lfi ) const
{
    mOpenFile( fnm );
    const char* res = getLogInfo( *sd.istrm, lfi );
    sd.close();
    return res;
}

#define mIsKey(s) caseInsensitiveEqual(keyw,s,0)
#define mErrRet(s) { lfi.depthcolnr = -1; return s; }

const char* Well::LASImporter::getLogInfo( std::istream& strm,
					   FileInfo& lfi ) const
{
    convs_.allowNull();
    convs_.erase();

    char linebuf[4096]; char wordbuf[64];
    const char* ptr;
    char section = '-';
    lfi.depthcolnr = -1;
    int colnr = 0;

    while ( strm )
    {
	strm.getline( linebuf, 4096 );
	ptr = linebuf; mSkipBlanks(ptr);
	if ( *ptr == '#' || *ptr == '\0' ) continue;

	if ( *ptr == '~' )
	{
	    section = *(++ptr);
	    if ( section == 'A' ) break;
	    continue;
	}
	else if ( section == '-' )
	{
	    // This is not LAS really, just one line of header and then go
	    mSkipBlanks(ptr);
	    while ( *ptr )
	    {
		ptr = getNextWord( ptr, wordbuf );
		mSkipBlanks(ptr);
		char* unstr = strchr( wordbuf, '(' );
		if ( unstr )
		{
		    *unstr++ = '\0';
		    char* closeparptr = strchr( unstr, ')' );
		    if ( closeparptr ) *closeparptr = '\0';
		}
		if ( lfi.depthcolnr < 0 && matchStringCI("dept",wordbuf) )
		    lfi.depthcolnr = colnr;
		else
		{
		    if ( colnr == 1 && ( !strcmp(wordbuf,"in:")
				    || isdigit(wordbuf[0]) || wordbuf[0] == '+'
				    || wordbuf[1] == '-' || wordbuf[2] == '.' ))
			mErrRet( "Invalid LAS-like file" )

		    lfi.lognms += new BufferString( wordbuf );
		}
		convs_ += UnitOfMeasure::getGuessed( unstr );
		unitmeasstrs_.add( unstr );
		colnr++;
	    }
	    break;
	}

	char* keyw = const_cast<char*>(ptr);
	char* val1; char* val2; char* info;
	parseHeader( keyw, val1, val2, info );

	switch ( section )
	{
	case 'C':

	    if ( lfi.depthcolnr < 0 && (mIsKey("dept") || mIsKey("depth")) )
		lfi.depthcolnr = colnr;
	    else
	    {
		BufferString lognm( info );
		if ( *lognm.buf() >= '0' && *lognm.buf() <= '9' )
		{
		    // Leading curve number. Remove it.
		    BufferString newnm( lognm );
		    char* newptr = (char*)getNextWord( newnm.buf(), wordbuf );
		    if ( newptr && *newptr )
			{ mSkipBlanks(newptr); }
		    if ( newptr && *newptr )
			lognm = newptr;
		}
		if ( matchString("Name = ",lognm.buf()) )
		{
		    // Possible HRS 'encoding'. Awful for user display.
		    BufferString newnm =  lognm.buf() + 7;
		    char* newptr = strstr( newnm.buf(), " - Type = " );
		    if ( !newptr )
			lognm = newnm;
		    else
		    {
			*newptr = '\0'; lognm = newnm; lognm += " (";
			newptr += 10; lognm += newptr; lognm += ")";
		    }
		}
		if ( lognm.isEmpty() )
		    lognm = keyw;
		lfi.lognms += new BufferString( lognm );
	    }

	    colnr++;
	    convs_ += UnitOfMeasure::getGuessed( val1 );
	    unitmeasstrs_.add( val1 );

	break;
	case 'W':
	    if ( mIsKey("STRT") )
		lfi.zrg.start = toFloat(val2);
	    if ( mIsKey("STOP") )
		lfi.zrg.stop = toFloat(val2);
	    if ( mIsKey("NULL") )
		lfi.undefval = toFloat( val1 );
	    if ( mIsKey("WELL") )
	    {
		lfi.wellnm = val1;
		if ( val2 && *val2 ) { lfi.wellnm += " "; lfi.wellnm += val2; }
	    }
	    if ( mIsKey("UWI") || mIsKey("API") )
		lfi.uwi = val1;
	break;
	default:
	break;
	}
    }

    if ( convs_.isEmpty() )
	mErrRet( "Could not find any valid log in file" )
    if ( lfi.depthcolnr < 0 )
	mErrRet( "Could not find a depth column ('DEPT' or 'DEPTH')" )

    lfi.revz = lfi.zrg.start > lfi.zrg.stop;
    lfi.zrg.sort();
    const UnitOfMeasure* unmeas = convs_[lfi.depthcolnr];
    if ( unmeas )
    {
	lfi.zunitstr = unmeas->symbol();
	if ( !mIsUdf(lfi.zrg.start) )
	    lfi.zrg.start = unmeas->internalValue(lfi.zrg.start);
	if ( !mIsUdf(lfi.zrg.stop) )
	    lfi.zrg.stop = unmeas->internalValue(lfi.zrg.stop);
    }

    if ( !strm.good() )
	mErrRet( "Only header found; No data" )
    else if ( lfi.lognms.size() < 1 )
	mErrRet( "No logs present" )

    return 0;
}


void Well::LASImporter::parseHeader( char* startptr, char*& val1, char*& val2,
				     char*& info ) const
{
    val1 = 0; val2 = 0; info = 0;
    char* ptr = strchr( startptr, '.' );
    if ( ptr ) *ptr++ = '\0';
    removeTrailingBlanks( startptr );
    if ( !ptr ) return;

    info = strchr( ptr, ':' );
    if ( info )
    {
	*info++ = '\0';
	mTrimBlanks(info);
    }

    mSkipBlanks( ptr );
    val1 = ptr;
    mSkipNonBlanks( ptr );
    val2 = ptr;
    if ( *ptr )
    {
	*val2++ = '\0';
	mTrimBlanks(val2);
    }
}


const char* Well::LASImporter::getLogs( const char* fnm, const FileInfo& lfi,
					bool istvd )
{
    mOpenFile( fnm );
    const char* res = getLogs( *sd.istrm, lfi, istvd );
    sd.close();
    return res;
}


const char* Well::LASImporter::getLogs( std::istream& strm,
					const FileInfo& lfi, bool istvd )
{
    FileInfo inplfi;
    const char* res = getLogInfo( strm, inplfi );
    if ( res )
	return res;
    if ( lfi.lognms.size() == 0 )
	return "No logs selected";
    if ( inplfi.depthcolnr < 0 )
	return "Input file is invalid";

    if ( lfi.depthcolnr < 0 )
	const_cast<FileInfo&>(lfi).depthcolnr = inplfi.depthcolnr;
    const int addstartidx = wd_->logs().size();
    BoolTypeSet issel( inplfi.lognms.size(), false );

    for ( int idx=0; idx<inplfi.lognms.size(); idx++ )
    {
	const int colnr = idx + (idx >= lfi.depthcolnr ? 1 : 0);
	const BufferString& lognm = inplfi.lognms.get(idx);
	const bool ispresent = indexOf( lfi.lognms, lognm ) >= 0;
	if ( !ispresent )
	    continue;
	if ( wd_->logs().getLog( lognm ) )
	{
	    BufferString msg( lognm );
	    msg += " already exists, will be ignored.";
	    pErrMsg( msg );
	    continue;
	}

	issel[idx] = true;
	Well::Log* newlog = new Well::Log( lognm );
	BufferString unlbl;
	if ( convs_[colnr] )
	{
	    if ( useconvs_ )
		unlbl = "Converted to SI from ";
	    unlbl += unitmeasstrs_.get( colnr );
	}
	newlog->setUnitMeasLabel( unlbl );
	wd_->logs().add( newlog );
    }

    return getLogData( strm, issel, lfi, istvd, addstartidx,
	    		inplfi.lognms.size() + 1 );
}


const char* Well::LASImporter::getLogData( std::istream& strm,
	const BoolTypeSet& issel, const FileInfo& lfi,
	bool istvd, int addstartidx, int totalcols )
{
    Interval<float> reqzrg( Interval<float>().setFrom( lfi.zrg ) );
    const bool havestart = !mIsUdf(reqzrg.start);
    const bool havestop = !mIsUdf(reqzrg.stop);
    if ( havestart && havestop )
	reqzrg.sort();

    float prevdpth = mUdf(float), prevdah = mUdf(float);
    int nradded = 0;
    while ( true )
    {
	TypeSet<float> vals;
	bool atend = false;
	float val;
	for ( int icol=0; icol<totalcols; icol++ )
	{
	    strm >> val;
	    if ( strm.fail() || (icol<totalcols-1 && strm.eof()) )
		{ atend = true; break; }
	    if ( mIsEqual(val,lfi.undefval,mDefEps) )
		val = mUdf(float);
	    else if ( useconvs_ && convs_[icol] )
		val = convs_[icol]->internalValue( val );
	    vals += val;
	}
	if ( atend )
	    break;

	float dpth = vals[ lfi.depthcolnr ];
	if ( mIsUdf(dpth) )
	    continue;

	if ( convs_[lfi.depthcolnr] )
	    dpth = convs_[lfi.depthcolnr]->internalValue( dpth );

	const bool afterstop = havestop && dpth > reqzrg.stop + mDefEps;
	const bool beforestart = havestart && dpth < reqzrg.start - mDefEps;
	if ( (lfi.revz && beforestart) || (!lfi.revz && afterstop) )
	    break;
	if ( beforestart || afterstop || mIsEqual(prevdpth,dpth,mDefEps) )
	    continue;
	prevdpth = dpth;

	TypeSet<float> selvals;
	for ( int icol=0; icol<totalcols; icol++ )
	{
	    if ( icol == lfi.depthcolnr )
		continue;

	    const int valnr = icol - (icol > lfi.depthcolnr ? 1 : 0);
	    if ( icol != lfi.depthcolnr && issel[valnr] )
		selvals += vals[icol];
	}
	if ( selvals.isEmpty() ) continue;

	float dah = dpth;
	if ( istvd && !convToDah(wd_->track(),dah,prevdah) )
	    continue;
	prevdah = dah;

	for ( int idx=0; idx<selvals.size(); idx++ )
	    wd_->logs().getLog(addstartidx+idx).addValue( dah, selvals[idx] );

	nradded++;
    }

    if ( nradded == 0 )
	return "No matching log data found";

    wd_->logs().updateDahIntvs();
    wd_->logs().removeTopBottomUdfs();
    return 0;
}


Table::FormatDesc* Well::TrackAscIO::getDesc()
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


bool Well::TrackAscIO::getData( Well::Data& wd, bool tosurf ) const
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

	c.z = getfValue(2);
	const float newdah = getfValue( 3 );
	const bool havez = !mIsUdf(c.z);
	const bool havedah = !mIsUdf(newdah);
	if ( !havez && !havedah )
	    continue;

	if ( wd.track().size() == 0 )
	{
	    if ( !SI().isReasonable(wd.info().surfacecoord) )
		wd.info().surfacecoord = c;
	    if ( mIsUdf(wd.info().surfaceelev) )
		wd.info().surfaceelev = -c.z;

	    surfcoord.x = wd.info().surfacecoord.x;
	    surfcoord.y = wd.info().surfacecoord.y;
	    surfcoord.z = -wd.info().surfaceelev;

	    prevc = tosurf && c.z >=0 ? surfcoord : c;
	}

	if ( mIsUdf(newdah) )
	    dah += c.distTo( prevc );
	else
	{
	    if ( mIsUdf(c.z) )
	    {
		float d = newdah - dah;
		const float hdist = (float)Coord(c).distTo( Coord(prevc) );
		c.z = prevc.z;
		if ( d > hdist )
		    c.z += sqrt( d*d - hdist*hdist );
	    }
	    dah = newdah;
	}

	if ( c.distTo(c000) < 1 )
	    break;

	wd.track().addPoint( c, c.z, dah );
	prevc = c;
    }

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


bool Well::MarkerSetAscIO::get( std::istream& strm, Well::MarkerSet& ms,
       				const Well::Track& trck ) const
{
    ms.erase();

    const int dpthcol = columnOf( false, 0, 0 );
    const int nmcol = columnOf( false, 1, 0 );
    if ( nmcol > dpthcol )
    {
	// We'll assume that the name occupies up to 4 words
	for ( int icol=nmcol+1; icol<5; icol++ )
	{
	    if ( icol == dpthcol ) break;

	    if ( fd_.bodyinfos_[icol-nmcol+1]->selection_.elems_.isEmpty() )
		fd_.bodyinfos_[icol-nmcol+1]->selection_.elems_ +=
		    Table::TargetInfo::Selection::Elem( RowCol(0,icol), 0 );
	    else
		fd_.bodyinfos_[icol-nmcol+1]->selection_.elems_[0].pos_.col
		    = icol;
	}
    }

    while ( true )
    {
	int ret = getNextBodyVals( strm );
	if ( ret < 0 ) return false;
	if ( ret == 0 ) break;

	float dah = getfValue( 0 );
	BufferString namepart = text( 1 );
	if ( mIsUdf(dah) || namepart.isEmpty() )
	    continue;
	if ( formOf(false,0) == 1 && !convToDah(trck,dah) )
	    continue;

	BufferString fullnm( namepart );
	for ( int icol=nmcol+1; ; icol++ )
	{
	    if ( icol == dpthcol ) break;
	    namepart = text( icol );
	    if ( namepart.isEmpty() ) break;

	    fullnm += " "; fullnm += namepart;
	}
	ms += new Well::Marker( fullnm, dah );
    }

    return true;
}


Table::FormatDesc* Well::D2TModelAscIO::getDesc( bool withunitfld )
{
    Table::FormatDesc* fd = new Table::FormatDesc( "DepthTimeModel" );
    fd->headerinfos_ +=
	new Table::TargetInfo( "Undefined Value", StringInpSpec(sKey::FloatUdf()),
				Table::Required );
    createDescBody( fd, withunitfld );
    return fd;
}


void Well::D2TModelAscIO::createDescBody( Table::FormatDesc* fd,
					  bool withunits )
{
    Table::TargetInfo* ti = gtDepthTI( withunits );
    ti->add( new Table::TargetInfo::Form( "TVD rel SRD", FloatInpSpec() ) );
    fd->bodyinfos_ += ti;

    ti = new Table::TargetInfo( "Time", FloatInpSpec(), Table::Required,
	    			PropertyRef::Time );
    ti->form(0).setName( "TWT" );
    ti->add( new Table::TargetInfo::Form( "One-way TT", FloatInpSpec() ) );
    fd->bodyinfos_ += ti;
}


void Well::D2TModelAscIO::updateDesc( Table::FormatDesc& fd, bool withunitfld )
{
    fd.bodyinfos_.erase();
    createDescBody( &fd, withunitfld );
}


static bool getTVDD2TModel( Well::D2TModel& d2t,
		const TypeSet<float>& zvals, const TypeSet<float>& tvals,
		const Well::Track& trck )
{
    const float eps = 1e-4;
    TypeSet<float> mds, ts;
    for ( int iz=0; iz<zvals.size(); iz++ )
    {
	const float targettvd = zvals[iz];
	const float curt = tvals[iz];

	// find MD intervals including this TVD. Can be multiple.
	float prevtvd = trck.pos(0).z;
	for ( int idah=1; idah<trck.size(); idah++ )
	{
	    const float tvd = trck.pos(idah).z;
	    const float tvddist = tvd - prevtvd;
	    if ( mIsZero(tvddist,eps) )
		continue;

	    if ( targettvd > prevtvd-eps && targettvd < tvd+eps )
	    {
		const float relpos = (targettvd - prevtvd) / tvddist;
		mds += relpos*trck.dah(idah) + (1-relpos)*trck.dah(idah-1);
		ts += curt;
	    }
	    prevtvd = tvd;
	}
    }

    const int sz = mds.size();
    mAllocVarLenIdxArr( int, idxs, sz );
    sort_coupled( mds.arr(), idxs.ptr(), sz );
    for ( int idx=0; idx<sz; idx++ )
	d2t.add( mds[idx], ts[ idxs[idx] ] );

    return true;
}


bool Well::D2TModelAscIO::get( std::istream& strm, Well::D2TModel& d2t,
       				const Well::Data& wll ) const
{
    d2t.erase();
    if ( wll.track().isEmpty() ) return true;

    const int dpthopt = formOf( false, 0 );
    const int tmopt = formOf( false, 1 );
    const bool istvd = dpthopt > 0;
    TypeSet<float> zvals, tvals;
    while ( true )
    {
	int ret = getNextBodyVals( strm );
	if ( ret < 0 ) return false;
	if ( ret == 0 ) break;

	float zval = getfValue( 0 );
	float tval = getfValue( 1 );
	if ( mIsUdf(zval) || mIsUdf(tval) )
	    continue;
	if ( dpthopt == 2 )
	    zval -= wll.info().surfaceelev;
	if ( tmopt == 1 )
	    tval *= 2;

	if ( !istvd )
	    d2t.add( zval, tval );
	else
	    { zvals += zval; tvals += tval; }
    }

    return istvd ? getTVDD2TModel( d2t, zvals, tvals, wll.track() ) : true;
}
