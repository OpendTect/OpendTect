/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/


#include "wellimpasc.h"

#include "sorting.h"
#include "strmprov.h"
#include "tabledef.h"
#include "unitofmeasure.h"
#include "varlenarray.h"
#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "idxable.h"
#include "od_istream.h"

#include <math.h>


const char* Well::LASImporter::fileFilter()
{
#ifdef __win__
    return "LAS files (*.las *.dat)";
#else
    return "LAS files (*.las *.LAS *.dat *.DAT)";
#endif
}


static bool convToDah( const Well::Track& trck, float& val,
			float prevdah=mUdf(float) )
{
    val = trck.getDahForTVD( val, prevdah );
    return !mIsUdf(val);
}


Well::LASImporter::~LASImporter()
{
    unitmeasstrs_.erase();
}


#define mOpenFile(fnm) \
    od_istream strm( fnm ); \
    if ( !strm.isOK() ) \
    { \
	mDeclStaticString(ret); \
	ret = "Cannot open input file"; strm.addErrMsgTo( ret ); \
	return ret.buf(); \
    }


const char* Well::LASImporter::getLogInfo( const char* fnm,
					   FileInfo& lfi ) const
{
    mOpenFile( fnm );
    const char* res = getLogInfo( strm, lfi );
    return res;
}


#define mIsKey(s) caseInsensitiveEqual(keyw,s,0)
#define mErrRet(s) { lfi.depthcolnr = -1; return s; }

const char* Well::LASImporter::getLogInfo( od_istream& strm,
					   FileInfo& lfi ) const
{
    convs_.allowNull();
    convs_.erase();

    BufferString linebuf; char wordbuf[64];
    const char* ptr;
    char section = '-';
    lfi.depthcolnr = -1;
    int colnr = 0;
    LatLong ll;

    while ( strm.isOK() )
    {
	strm.getLine( linebuf );
	ptr = linebuf.buf(); mSkipBlanks(ptr);
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
		char* unstr = firstOcc( wordbuf, '(' );
		if ( unstr )
		{
		    *unstr++ = '\0';
		    char* closeparptr = firstOcc( unstr, ')' );
		    if ( closeparptr ) *closeparptr = '\0';
		}
		if ( lfi.depthcolnr<0 &&
			FixedString(wordbuf).startsWith("dept",CaseInsensitive))
		    lfi.depthcolnr = colnr;
		else
		{
		    if ( colnr == 1 && ( FixedString(wordbuf)=="in:"
				    || iswdigit(wordbuf[0]) || wordbuf[0] == '+'
				    || wordbuf[1] == '-' || wordbuf[2] == '.' ))
			mErrRet( "Invalid LAS-like file" )

		    lfi.lognms += new BufferString( wordbuf );
		    lfi.logcurves.add( wordbuf );
		    lfi.logunits.add( unstr );
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

	    if ( lfi.depthcolnr < 0 &&
		 (mIsKey("dept") || mIsKey("depth") || mIsKey("md")) )
		lfi.depthcolnr = colnr;
	    else
	    {
		BufferString curve( keyw );
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
		if ( lognm.startsWith("Name = ") )
		{
		    // Possible HRS 'encoding'. Awful for user display.
		    BufferString newnm = lognm.buf() + 7;
		    char* newptr = newnm.find( " - Type = " );
		    if ( !newptr )
			lognm = newnm;
		    else
		    {
			*newptr = '\0'; lognm = newnm; lognm += " (";
			newptr += 10; lognm += newptr; lognm += ")";
		    }
		}
		if ( lognm.isEmpty() ||
		     lognm.startsWith("Run",CaseInsensitive) )
		    lognm = keyw;

		lfi.logcurves.add( curve );
		lfi.lognms.add( lognm );
		lfi.logunits.add( val1 );
	    }

	    colnr++;
	    convs_ += UnitOfMeasure::getGuessed( val1 );
	    unitmeasstrs_.add( val1 );

	break;
	case 'W':	// ~Well Information Block
	case 'P':	// ~Parameter Information Block
	    if ( mIsKey("STRT") )
		lfi.zrg.start = toFloat(val2);
	    if ( mIsKey("STOP") )
		lfi.zrg.stop = toFloat(val2);
	    if ( mIsKey("NULL") )
		lfi.undefval = toFloat(val1);
	    if ( mIsKey("COMP") )
	    {
		lfi.comp_ = val1;
		if ( val2 && *val2 ) { lfi.comp_ += " "; lfi.comp_ += val2; }
	    }
	    if ( mIsKey("WELL") )
	    {
		lfi.wellnm = val1;
		if ( val2 && *val2 ) { lfi.wellnm += " "; lfi.wellnm += val2; }
	    }
	    if ( (mIsKey("EKB") || mIsKey("EDF")) && mIsUdf(lfi.kbelev_))
		lfi.kbelev_ = toDouble(val2);
	    if ( mIsKey("EGL") )
		lfi.glelev_ = toDouble(val2);
	    // TODO: Country and State are two different things. Need to split
	    if ( (mIsKey("CTRY") || mIsKey("STAT")) && lfi.state_.isEmpty() &&
		 (val1 && *val1) )
	    {
		lfi.state_ = val1;
		if ( val2 && *val2 ) { lfi.state_ += " "; lfi.state_ += val2; }
	    }
	    if ( (mIsKey("CNTY") || mIsKey("PROV")) && lfi.county_.isEmpty() &&
		 (val1 && *val1) )
	    {
		lfi.county_ = val1;
		if ( val2 && *val2 )
		{
		    lfi.county_ += " "; lfi.county_ += val2;
		}
	    }
	    if ( mIsKey("LOC") )
		parseLocation( val1, val2, lfi.loc_ );
	    if ( mIsKey("UWI") )
	    {
		lfi.uwi = val1;
		if ( val2 && *val2 ) { lfi.uwi += " "; lfi.uwi += val2; }
	    }
	    if ( mIsKey("API") && lfi.uwi.isEmpty() )
		lfi.uwi = val1;
	    if ( mIsKey("SRVC") )
	    {
		lfi.srvc_ = val1;
		if ( val2 && *val2 ) { lfi.srvc_ += " "; lfi.srvc_ += val2; }
	    }
	    if ( mIsKey("XCOORD") || mIsKey("XWELL") || mIsKey("X") )
	    {
		// TODO: use UOM
		BufferString locx = val2 && *val2 ? val2 : val1;
		lfi.loc_.x = toDouble( locx, mUdf(double) );
	    }
	    if ( mIsKey("YCOORD") || mIsKey("YWELL") || mIsKey("Y") )
	    {
		// TODO: use UOM
		BufferString locy = val2 && *val2 ? val2 : val1;
		lfi.loc_.y = toDouble( locy, mUdf(double) );
	    }
	    if ( mIsKey("LATI") || mIsKey("LAT") )
	    {
		BufferString lat = val2 && *val2 ? val2 : val1;
		if ( LatLong::isDMSString(lat) )
		    lat = BufferString( val1, " ", val2 );

		ll.setFromString( lat, true );
	    }
	    if ( mIsKey("LONG") || mIsKey("LON") )
	    {
		BufferString lon = val2 && *val2 ? val2 : val1;
		if ( LatLong::isDMSString(lon) )
		    lon = BufferString( val1, " ", val2 );

		ll.setFromString( lon, false );
	    }
	break;
	default:
	break;
	}
    }

    if ( lfi.loc_.isUdf() && ll.isDefined() )
	lfi.loc_ = LatLong::transform( ll );

    if ( convs_.isEmpty() )
	mErrRet( "Could not find any valid log in file" )
    if ( lfi.depthcolnr < 0 )
	mErrRet( "Could not find a depth column ('DEPT' or 'DEPTH')")

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
	if ( !mIsUdf(lfi.kbelev_) )
	    lfi.kbelev_ = unmeas->internalValue(lfi.kbelev_);
	if ( !mIsUdf(lfi.glelev_) )
	    lfi.glelev_ = unmeas->internalValue(lfi.glelev_);
	//TODO: Position unit conversion ?
    }

    if ( !strm.isOK() )
	mErrRet( "Only header found; No data" )
    else if ( lfi.lognms.size() < 1 )
	mErrRet( "No logs present" )

    return 0;
}


void Well::LASImporter::parseHeader( char* startptr, char*& val1, char*& val2,
				     char*& info ) const
{
    val1 = 0; val2 = 0; info = 0;
    char* ptr = firstOcc( startptr, '.' );
    if ( ptr ) *ptr++ = '\0';
    removeTrailingBlanks( startptr );
    if ( !ptr ) return;

    info = firstOcc( ptr, ':' ); //TODO: handle DATE
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


void Well::LASImporter::parseLocation( const char* startptr1,
				       const char* startptr2, Coord& ret )
{
    const BufferString locstr( startptr1, startptr2 );
    if ( locstr.isEmpty() || locstr.contains("UNKNOWN") )
	return;

    const char* startptr = locstr.str();
    mSkipBlanks(startptr)
    while ( *startptr && !( (*startptr >= '0' && *startptr <= '9') ||
			     *startptr == '-' || *startptr == '+' ||
			     *startptr == 'E' || *startptr == 'N' ||
			     *startptr == '\'' || *startptr == '"' ) )
    {
	startptr++;
	mSkipBlanks(startptr)
    }
    if ( !startptr )
	return;

    BufferString word( 64, true );
    char* wordbuf = word.getCStr();
    startptr = (char*)getNextWord( startptr, wordbuf );
    if ( !startptr )
	return;

    Coord pos;
    LatLong ll;
    bool islatlong = locstr.contains('N') || locstr.contains('S') ||
		     locstr.contains('E') || locstr.contains('W');
    if ( isNumberString(wordbuf) )
    {
	const double posd = toDouble(wordbuf);
	if ( Math::Abs(posd) < 360. )
	{
	    ll.lat_ = posd;
	    islatlong = true;
	}
	else
	    pos.x = posd;
    }
    else if ( islatlong )
	ll.setFromString( wordbuf, true );

    while ( *startptr && !( (*startptr >= '0' && *startptr <= '9') ||
			     *startptr == '-' || *startptr == '+' ||
			     *startptr == 'E' || *startptr == 'N' ||
			     *startptr == '\'' || *startptr == '"' ) )
    {
	startptr++;
	mSkipBlanks(startptr)
    }
    if ( !startptr )
	return;

    startptr = (char*)getNextWord( startptr, wordbuf );
    if ( isNumberString(wordbuf) )
    {
	const double posd = toDouble(wordbuf);
	if ( Math::Abs(posd) < 361. )
	{
	    ll.lng_ = posd;
	    islatlong = true;
	}
	else
	    pos.y = posd;
    }
    else if ( islatlong )
	ll.setFromString( wordbuf, false );

    if ( islatlong )
    {
	pos = LatLong::transform( ll );
	if ( !pos.isDefined() )
	{
	    pos.x = ll.lat_;
	    pos.y = ll.lng_;
	}
    }

    if ( pos.isDefined() )
	ret = pos;
}


void Well::LASImporter::copyInfo( const FileInfo& inf, bool& changed )
{
    if ( !wd_ )
	return;

    Well::Info& winf = wd_->info();
    if ( !inf.state_.isEmpty() && winf.state.isEmpty() )
    {
	winf.state = inf.state_;
	changed = true;
    }
    if ( !inf.county_.isEmpty() && winf.county.isEmpty() )
    {
	winf.county = inf.county_;
	changed = true;
    }
    if ( !inf.srvc_.isEmpty() && winf.oper.isEmpty() )
    {
	winf.oper = inf.srvc_;
	changed = true;
    }
    if ( !mIsUdf(inf.glelev_) && !mIsZero(inf.glelev_,1e-2f) &&
	  mIsUdf(winf.groundelev) )
    {
	winf.groundelev = inf.glelev_;
	changed = true;
    }
}


void Well::LASImporter::adjustTrack( const Interval<float>& zrg, bool istvdss,
				     bool& changed )
{
    if ( !wd_ )
	return;

    Well::Track& track = wd_->track();
    if ( ( istvdss && track.zRange().includes(zrg)) ||
	 (!istvdss && track.dahRange().includes(zrg)) )
	return;

    const float firsttrackz = istvdss ? track.zRange().start
				      : track.dahRange().start;
    const float lasttrackz = istvdss ? track.zRange().stop
				     : track.dahRange().stop;
    if ( firsttrackz > zrg.start )
    {
	const Coord3 surfloc = track.pos(0);
	const bool inserted = track.insertAtDah( 0.f, -1.f*track.getKbElev() );
	if ( inserted )
	{
	    const int idx = track.indexOf( 0.f );
	    if ( idx > -1 )
		const_cast<Coord3&>( track.pos(idx) ).coord() = surfloc.coord();
	    changed = true;
	}
    }

    if ( lasttrackz < zrg.stop )
    {
	Coord3 lastpos = track.pos( track.size()-1 );
	const float lastmd = track.td();
	const double zdiff = zrg.stop - (istvdss ? lasttrackz : lastmd);
	lastpos.z += zdiff;
	track.addPoint( lastpos, istvdss ? lastmd + zdiff : zrg.stop );
	changed = true;
    }
}


const char* Well::LASImporter::getLogs( const char* fnm, const FileInfo& lfi,
					bool istvd, bool usecurvenms )
{
    mOpenFile( fnm );
    const char* res = getLogs( strm, lfi, istvd, usecurvenms );
    return res;
}


const char* Well::LASImporter::getLogs( od_istream& strm, const FileInfo& lfi,
					bool istvd, bool usecurvenms )
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
    BoolTypeSet issel( inplfi.size(), false );

    const BufferStringSet& lognms =
		usecurvenms ? inplfi.logcurves : inplfi.lognms;
    for ( int idx=0; idx<lognms.size(); idx++ )
    {
	const int colnr = idx + (idx >= lfi.depthcolnr ? 1 : 0);
	const BufferString& lognm = lognms.get(idx);
	const bool ispresent = lfi.lognms.isPresent( lognm );
	if ( !ispresent )
	    continue;
	if ( wd_->logs().getLog(lognm) )
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
			inplfi.size()+1 );
}


const char* Well::LASImporter::getLogData( od_istream& strm,
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
	    if ( strm.isBad() || (icol<totalcols-1 && !strm.isOK()) )
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
