/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID = "$Id: wellimpasc.cc,v 1.18 2004-02-19 14:02:53 bert Exp $";

#include "wellimpasc.h"
#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
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
	    surfcoord = c;
	    if ( SI().isReasonable( wd.info().surfacecoord ) )
	    {
		surfcoord.x = wd.info().surfacecoord.x;
		surfcoord.y = wd.info().surfacecoord.y;
	    }
	    surfcoord.z = -wd.info().surfaceelev;
	    prevc = tosurf ? surfcoord : c;
	}

	if ( *ptr )
	{
	    getNextWord( ptr, valbuf );
	    dah = atof(valbuf) * zfac;
	}
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

    if ( !wd.d2TModel() )
	wd.setD2TModel( new Well::D2TModel );
    Well::D2TModel& d2t = *wd.d2TModel();
    d2t.erase();

    const float zfac = zinfeet ? 0.3048 : 1;
    float z, val, prevdah = mUndefValue;
    while ( *sd.istrm )
    {
	*sd.istrm >> z >> val;
	z *= zfac;
	if ( !*sd.istrm ) break;
	if ( istvd )
	{
	    z = wd.track().getDahForTVD( z, prevdah );
	    if ( mIsUndefined(z) ) continue;
	    prevdah = z;
	}
	d2t.add( z, val );
    }

    sd.close();
    return d2t.size() ? 0 : "No valid Depth/Time points found";
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

const char* Well::AscImporter::getLogInfo( istream& strm,
					   LasFileInfo& lfi ) const
{
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
	    {
		lfi.zrg.start = atof(val2);
		const UnitOfMeasure* mu = UnitOfMeasure::getGuessed( val1 );
		if ( mu ) lfi.zrg.start = mu->internalValue( lfi.zrg.start );
	    }
	    if ( mIsKey("STOP") )
	    {
		lfi.zrg.start = atof(val2);
		const UnitOfMeasure* mu = UnitOfMeasure::getGuessed( val1 );
		if ( mu ) lfi.zrg.start = mu->internalValue( lfi.zrg.start );
	    }
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


const char* Well::AscImporter::getLogs( istream& strm,
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
    bool havestart = !mIsUndefined(lfi.zrg.start);
    bool havestop = !mIsUndefined(lfi.zrg.stop);
    TypeSet<float> vals;

    while ( 1 )
    {
	strm >> dpth;
	if ( strm.fail() || strm.eof() ) break;
	dpth = mIS_ZERO(dpth-lfi.undefval) ? mUndefValue
					   : convs[0]->internalValue( dpth );
	if ( havestop && dpth > lfi.zrg.stop ) break;
	bool douse = !mIsUndefined(dpth)
	          && (!havestart || dpth >= lfi.zrg.start);
	if ( mIS_ZERO(prevdpth-dpth) )
	    douse = false;
	else
	    prevdpth = dpth;

	vals.erase();
	for ( int ilog=0; ilog<inplfi.lognms.size(); ilog++ )
	{
	    strm >> val;
	    if ( !douse || !issel[ilog] ) continue;

	    val = mIS_ZERO(val-lfi.undefval) ? mUndefValue
		: (useconvs_ ? convs[ilog+1]->internalValue( val ) : val);
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
