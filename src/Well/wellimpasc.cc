/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID = "$Id: wellimpasc.cc,v 1.2 2003-08-25 10:31:12 bert Exp $";

#include "wellimpasc.h"
#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "filegen.h"
#include "strmprov.h"
#include "unitscale.h"
#include <fstream>

inline static StreamData getSD( const char* fnm )
{
    StreamProvider sp( fnm );
    StreamData sd = sp.makeIStream();
    return sd;
}


Well::AscImporter::~AscImporter()
{
    deepErase( convs );
}


#define mOpenFile(fnm) \
	StreamProvider sp( fnm ); \
	sd = sp.makeIStream(); \
	if ( !sd.usable() ) \
	    return "Cannot open input file"
#define strm (*sd.istrm)

const char* Well::AscImporter::getTrack( const char* fnm, bool tosurf )
{
    mOpenFile( fnm );

    Coord3 c, c0, prevc;
    float dah = 0;
    while ( strm )
    {
	strm >> c.x >> c.y >> c.z;
	if ( !strm || c.distance(c0) < 1 ) break;

	if ( wd.track().size() == 0 )
	    prevc = tosurf ?
		    Coord3(wd.info().surfacecoord,wd.info().surfaceelev) : c;
	dah += c.distance( prevc );

	wd.track().addPoint( c, c.z, dah );
    }

    sd.close();
    return wd.track().size() ? 0 : "No valid well track points found";
}


const char* Well::AscImporter::getD2T( const char* fnm, bool istvd )
{
    mOpenFile( fnm );

    if ( !wd.d2TModel() )
	wd.setD2TModel( new Well::D2TModel );
    Well::D2TModel& d2t = *wd.d2TModel();
    d2t.erase();

    float z, val, prevdah = mUndefValue;
    while ( strm )
    {
	strm >> z >> val;
	if ( !strm ) break;
	if ( istvd )
	{
	    z = wd.track().getDahForTVD( z, prevdah );
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
    return gtLogInfo( fnm, lfi, true );
}

#define mIsKey(s) caseInsensitiveEqual(keyw,s,0)

const char* Well::AscImporter::gtLogInfo( const char* fnm, LasFileInfo& lfi,
					   bool closestrm ) const
{
    deepErase( convs );
    wrap = false;
    mOpenFile( fnm );

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
		if ( qnr )
		{
		    if ( qnr == 1 && (
			    !strcmp(wordbuf,"in:")
			    || isdigit(wordbuf[0])
			    || wordbuf[0] == '+'
			    || wordbuf[1] == '-'
			    || wordbuf[2] == '.'
				) )
			if ( closestrm ) sd.close();
			return "Invalid LAS-like file";

		    lfi.lognms += new BufferString( wordbuf );
		}
		convs += MeasureUnit::getGuessed( unstr );
		qnr++;
	    }
	    break;
	}

	char* keyw = const_cast<char*>(ptr);
	char* val1; char* val2; char* info;
	parseHeader( keyw, val1, val2, info );

	switch ( section )
	{
	case 'V':
	    if ( mIsKey("WRAP") )
		wrap = yesNoFromString( val1 );
	break;
	case 'C':
	    if ( mIsKey("DEPTH") )
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
	    convs += MeasureUnit::getGuessed( val1 );
	break;
	case 'W':
	    if ( mIsKey("STRT") )
	    {
		MeasureUnit* mu = MeasureUnit::getGuessed( val1 );
		lfi.zrg.start = mu->toSI( atof(val2) );
		delete mu;
	    }
	    if ( mIsKey("STOP") )
	    {
		MeasureUnit* mu = MeasureUnit::getGuessed( val1 );
		lfi.zrg.stop = mu->toSI( atof(val2) );
		delete mu;
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
    if ( closestrm ) sd.close();

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
    LasFileInfo inplfi;
    const char* res = gtLogInfo( fnm, inplfi, false );
    if ( res )
	{ sd.close(); return res; }


    return 0;
}
