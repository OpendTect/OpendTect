/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID = "$Id: wellimpasc.cc,v 1.24 2004-06-16 08:32:32 nanne Exp $";

#include "wellimpasc.h"
#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "filegen.h"
#include "strmprov.h"
#include "unitofmeasure.h"
#include "survinfo.h"
#include <iostream>

inline static StreamData getSD( const char* fnm )
{
    StreamProvider sp( fnm );
    StreamData sd = sp.makeIStream();
    return sd;
}


Well::AscImporter::~AscImporter()
{
    unitmeasstrs.deepErase();
}


#define mOpenFile(fnm) \
	StreamProvider sp( fnm ); \
	StreamData sd = sp.makeIStream(); \
	if ( !sd.usable() ) \
	    return "Cannot open input file"

const char* Well::AscImporter::getTrack( const char* fnm, bool tosurf, 
					 bool zinfeet )
{
    mOpenFile( fnm );

    Coord3 c, c0, prevc;
    Coord3 surfcoord;
    float dah = 0;
    const float zfac = zinfeet ? 0.3048 : 1;
    char buf[1024]; char valbuf[256];
    while ( *sd.istrm )
    {
	sd.istrm->getline( buf, 1024 );
	const char* ptr = getNextWord( buf, valbuf );
	c.x = atof( valbuf );
	ptr = getNextWord( ptr, valbuf );
	c.y = atof( valbuf );
	if ( !*ptr ) break;
	ptr = getNextWord( ptr, valbuf );
	c.z = atof( valbuf );
	c.z *= zfac;
	if ( c.distance(c0) < 1 ) break;

	if ( wd.track().size() == 0 )
	{
	    if ( !SI().isReasonable(wd.info().surfacecoord) )
		wd.info().surfacecoord = c;
	    if ( mIsUndefined(wd.info().surfaceelev) )
		wd.info().surfaceelev = -c.z;

	    surfcoord.x = wd.info().surfacecoord.x;
	    surfcoord.y = wd.info().surfacecoord.y;
	    surfcoord.z = -wd.info().surfaceelev;

	    prevc = tosurf ? surfcoord : c;
	}

	getNextWord( ptr, valbuf );
	float newdah = *ptr ? atof(valbuf) * zfac : 0;

	if ( wd.track().size() == 0 || !mIS_ZERO(newdah) )
	    dah = newdah;
	else
	    dah += c.distance( prevc );

	wd.track().addPoint( c, c.z, dah );
	prevc = c;
    }

    sd.close();
    return wd.track().size() ? 0 : "No valid well track points found";
}


const char* Well::AscImporter::getD2T( const char* fnm, bool istvd, 
				       bool zinfeet )
{
    mOpenFile( fnm );
    std::istream& strm = *sd.istrm;

    if ( !wd.d2TModel() )
	wd.setD2TModel( new Well::D2TModel );
    Well::D2TModel& d2t = *wd.d2TModel();
    d2t.erase();

    const float zfac = zinfeet ? 0.3048 : 1;
    float z, val, prevdah = mUndefValue;
    bool firstpos = true;
    bool t_in_ms = false;
    TypeSet<float> tms; TypeSet<float> dahs;
    while ( strm )
    {
	strm >> z >> val;
	if ( !strm ) break;
	if ( mIsUndefined(z) ) continue;
	z *= zfac;

	if ( istvd )
	{
	    z = wd.track().getDahForTVD( z, prevdah );
	    if ( mIsUndefined(z) ) continue;
	    prevdah = z;
	}

	tms += val;
	dahs += z;
    }
    sd.close();
    if ( !tms.size() ) return "No valid Depth/Time points found";

    const bool t_in_sec = tms[tms.size()-1] < 2 * SI().zRange(false).stop;

    for ( int idx=0; idx<tms.size(); idx++ )
	d2t.add( dahs[idx], t_in_sec ? tms[idx] : tms[idx] * 0.001 );

    return 0;
}


const char* Well::AscImporter::getMarkers( const char* fnm, bool istvd, 
					   bool zinfeet )
{
    mOpenFile( fnm );
    std::istream& strm = *sd.istrm;
    const float zfac = zinfeet ? 0.3048 : 1;
    float z, prevdah = mUndefValue;
#   define mBufSz 128
    char buf[mBufSz];
    while ( strm )
    {
	strm >> z;
	if ( !strm ) break;
	strm.getline( buf, mBufSz );
	char* ptr = buf; skipLeadingBlanks(ptr); removeTrailingBlanks(ptr);
	if ( mIsUndefined(z) || !*ptr ) continue;
	z *= zfac;

	if ( istvd )
	{
	    z = wd.track().getDahForTVD( z, prevdah );
	    if ( mIsUndefined(z) ) continue;
	    prevdah = z;
	}

	Well::Marker* newmrk = new Well::Marker( ptr );
	newmrk->dah = z;
	wd.markers() += newmrk;
    }
    sd.close();

    return 0;
}



const char* Well::AscImporter::getLogInfo( const char* fnm,
					   LasFileInfo& lfi ) const
{
    mOpenFile( fnm );
    const char* res = getLogInfo( *sd.istrm, lfi );
    sd.close();
    return res;
}

#define mIsKey(s) caseInsensitiveEqual(keyw,s,0)

const char* Well::AscImporter::getLogInfo( std::istream& strm,
					   LasFileInfo& lfi ) const
{
    convs.allowNull();
    convs.erase();

    char linebuf[1024]; char wordbuf[64];
    const char* ptr;
    char section = '-';
    bool hasdepthlog = false;

    while ( !strm.eof() )
    {
	strm.getline( linebuf, 1024 );
	ptr = linebuf; skipLeadingBlanks(ptr);
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
	    int qnr = 0;
	    skipLeadingBlanks(ptr);
	    while ( *ptr )
	    {
		ptr = getNextWord( ptr, wordbuf );
		skipLeadingBlanks(ptr);
		char* unstr = strchr( wordbuf, '(' );
		if ( unstr )
		{
		    *unstr++ = '\0';
		    char* closeparptr = strchr( unstr, ')' );
		    if ( closeparptr ) *closeparptr = '\0';
		}
		if ( qnr == 0 )
		    hasdepthlog = matchStringCI("dept",wordbuf);
		else
		{
		    if ( qnr == 1 && ( !strcmp(wordbuf,"in:")
				    || isdigit(wordbuf[0]) || wordbuf[0] == '+'
				    || wordbuf[1] == '-' || wordbuf[2] == '.' ))
			return "Invalid LAS-like file";

		    lfi.lognms += new BufferString( wordbuf );
		}
		convs += UnitOfMeasure::getGuessed( unstr );
		unitmeasstrs.add( unstr );
		qnr++;
	    }
	    break;
	}

	char* keyw = const_cast<char*>(ptr);
	char* val1; char* val2; char* info;
	parseHeader( keyw, val1, val2, info );

	switch ( section )
	{
	case 'C':
	    if ( mIsKey("depth") && !hasdepthlog )
		hasdepthlog = true;
	    else
	    {
		if ( !info || !*info )
		    info = keyw;
		if ( *info >= '0' && *info <= '9' )
		{
		    const char* newptr = getNextWord( info, wordbuf );
		    if ( newptr && *newptr ) skipLeadingBlanks(newptr);
		    if ( newptr && *newptr ) info = (char*)newptr;
		}
		lfi.lognms += new BufferString( info );
	    }
	    convs += UnitOfMeasure::getGuessed( val1 );
	    unitmeasstrs.add( val1 );
	break;
	case 'W':
	    if ( mIsKey("STRT") )
		lfi.zrg.start = atof(val2);
	    if ( mIsKey("STOP") )
		lfi.zrg.stop = atof(val2);
	    if ( mIsKey("NULL") )
		lfi.undefval = atof( val1 );
	    if ( mIsKey("WELL") )
	    {
		lfi.wellnm = val1;
		if ( val2 ) { lfi.wellnm += " "; lfi.wellnm += val2; }
	    }
	break;
	default:
	break;
	}
    }

    if ( convs[0] ) lfi.zunitstr = convs[0]->symbol();
    const char* ret = strm.good() ? 0 : "Only header found; No data";

    if ( !lfi.lognms.size() )
	ret = "No logs present";
    else if ( !hasdepthlog )
	ret = "'DEPTH' not present";

    return ret;
}


void Well::AscImporter::parseHeader( char* startptr, char*& val1, char*& val2,
				     char*& info ) const
{
    val1 = 0; val2 = 0; info = "";
    char* ptr = strchr( startptr, '.' );
    if ( ptr ) *ptr++ = '\0';
    removeTrailingBlanks( startptr );
    if ( !ptr ) return;

    info = strchr( ptr, ':' );
    if ( info )
    {
	*info++ = '\0';
	skipLeadingBlanks(info); removeTrailingBlanks(info);
    }

    skipLeadingBlanks( ptr );
    val1 = ptr;
    while ( *ptr && !isspace(*ptr) ) ptr++;
    val2 = ptr;
    if ( *ptr )
    {
	*val2++ = '\0';
	skipLeadingBlanks(val2); removeTrailingBlanks(val2);
    }
}


const char* Well::AscImporter::getLogs( const char* fnm, const LasFileInfo& lfi,
					bool istvd )
{
    mOpenFile( fnm );
    const char* res = getLogs( *sd.istrm, lfi, istvd );
    sd.close();
    return res;
}


const char* Well::AscImporter::getLogs( std::istream& strm,
					const LasFileInfo& lfi, bool istvd )
{
    LasFileInfo inplfi;
    const char* res = getLogInfo( strm, inplfi );
    if ( res )
	return res;
    if ( lfi.lognms.size() == 0 )
	return "No logs selected";

    const int prevnrlogs = wd.logs().size();
    BoolTypeSet issel( inplfi.lognms.size(), false );
    for ( int idx=0; idx<inplfi.lognms.size(); idx++ )
    {
	int globidx = idx + 1;
	if ( indexOf(lfi.lognms,*inplfi.lognms[idx]) >= 0 )
	{
	    issel[idx] = true;
	    Well::Log* newlog = new Well::Log( inplfi.lognms[idx]->buf() );
	    BufferString unlbl;
	    if ( convs[globidx] )
	    {
		if ( useconvs_ )
		    unlbl = "Converted to SI from ";
		unlbl += unitmeasstrs.get( globidx );
	    }
	    newlog->setUnitMeasLabel( unlbl );
	    wd.logs().add( newlog );
	}
    }

    float val; double dpth, prevdpth = -1e30;
    Interval<float> reqzrg;
    assign( reqzrg, lfi.zrg );
    bool havestart = !mIsUndefined(reqzrg.start);
    bool havestop = !mIsUndefined(reqzrg.stop);
    if ( convs[0] )
    {
	reqzrg.start = convs[0]->internalValue( reqzrg.start );
	reqzrg.stop = convs[0]->internalValue( reqzrg.stop );
    }

    TypeSet<float> vals;
    while ( 1 )
    {
	strm >> dpth;
	if ( strm.fail() || strm.eof() ) break;
	if ( mIS_ZERO(dpth-lfi.undefval) )
	    dpth = mUndefValue;
	else if ( convs[0] )
	    dpth = convs[0]->internalValue( dpth );

#define mIsZero(x) ( (x) < (1e-5) && (x) > (-1e-5) )

	bool atstop = mIsZero(reqzrg.stop-dpth);
	if ( havestop && !atstop && dpth > reqzrg.stop ) break;

	bool atstart = mIsZero(reqzrg.start-dpth);
	bool douse = !mIsUndefined(dpth)
	          && (!havestart || atstart || dpth >= reqzrg.start);
	if ( mIS_ZERO(prevdpth-dpth) )
	    douse = false;
	else
	    prevdpth = dpth;

	vals.erase();
	for ( int ilog=0; ilog<inplfi.lognms.size(); ilog++ )
	{
	    strm >> val;
	    if ( !douse || !issel[ilog] ) continue;

	    if ( mIS_ZERO(val-lfi.undefval) )
		val = mUndefValue;
	    else if ( useconvs_ && convs[ilog+1] )
		val = convs[ilog+1]->internalValue( val );

	    vals += val;
	}
	if ( !vals.size() ) continue;

	const float z = istvd ? wd.track().getDahForTVD( dpth, prevdpth )
	    		      : dpth;
	for ( int idx=0; idx<vals.size(); idx++ )
	    wd.logs().getLog(prevnrlogs+idx).addValue( z, vals[idx] );
    }

    wd.logs().updateDahIntvs();
    return 0;
}
