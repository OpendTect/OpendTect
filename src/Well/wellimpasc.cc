/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID = "$Id: wellimpasc.cc,v 1.1 2003-08-22 16:40:34 bert Exp $";

#include "wellimpasc.h"
#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "filegen.h"
#include "strmprov.h"
#include <fstream>

inline static StreamData getSD( const char* fnm )
{
    StreamProvider sp( fnm );
    StreamData sd = sp.makeIStream();
    return sd;
}


#define mOpenFile(fnm) \
	StreamProvider sp( fnm ); \
	StreamData sd = sp.makeIStream(); \
	if ( !sd.usable() ) \
	    return "Cannot open input file"
#define strm (*sd.istrm)

const char* Well::AscImporter::getTrack( const char* fnm, bool tosurf ) const
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

    return wd.track().size() ? 0 : "No valid well track points found";
}


const char* Well::AscImporter::getD2T( const char* fnm, bool istvd ) const
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

    return d2t.size() ? 0 : "No valid Depth/Time points found";
}
