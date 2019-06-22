/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Aug 2003
-*/


#include "wellimpasc.h"

#include "sorting.h"
#include "staticstring.h"
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
    convs_.setNullAllowed();
    convs_.erase();

    BufferString linebuf; char wordbuf[64];
    const char* ptr;
    char section = '-';
    lfi.depthcolnr = -1;
    int colnr = 0;

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
		ptr = getNextNonBlanks( ptr, wordbuf );
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
		    char* newptr = (char*)getNextNonBlanks( newnm.getCStr(),
						    wordbuf );
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

    info = firstOcc( ptr, ':' );
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

	if ( wd_->logs().isPresent(lognm) )
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
	    wd_->logs().getLogByIdx(addstartidx+idx)
			->addValue( dah, selvals[idx] );

	nradded++;
    }

    if ( nradded == 0 )
	return "No matching log data found";

    wd_->logs().removeTopBottomUdfs();
    return 0;
}
