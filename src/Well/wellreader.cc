/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID = "$Id: wellreader.cc,v 1.1 2003-08-15 15:29:22 bert Exp $";

#include "wellreader.h"
#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "ascstream.h"
#include "filegen.h"
#include "errh.h"
#include "strmprov.h"
#include <fstream>

const char* Well::IO::sKeyWell = "Well";
const char* Well::IO::sKeyLog = "Well Log";
const char* Well::IO::sKeyMarkers = "Well Markers";
const char* Well::IO::sKeyD2T = "Detph2Time Model";
const char* Well::IO::sExtWell = ".well";
const char* Well::IO::sExtLog = ".wll";
const char* Well::IO::sExtMarkers = ".wlm";
const char* Well::IO::sExtD2T = ".wlt";


Well::IO::IO( const char* f, bool fr )
    	: basenm(f)
    	, isrdr(fr)
{
    BufferString fnm = File_getPathOnly( basenm.buf() );
    if ( strrchr(fnm.buf(),'.') )
    {
	char* ptr = const_cast<char*>( strrchr( basenm.buf(), '.' ) );
	*ptr = '\0';
    }
}


StreamData Well::IO::mkSD( const char* ext, int nr ) const
{
    StreamProvider sp( getFileName(ext,nr) );
    return isrdr ? sp.makeIStream() : sp.makeOStream();
}


const char* Well::IO::getFileName( const char* ext, int nr ) const
{
    static BufferString fnm;
    fnm = basenm;
    if ( nr )
	{ fnm += "^"; fnm += nr; }
    fnm = File_getFullPath( fnm.buf(), ext );
    return fnm;
}


Well::Reader::Reader( const char* f, Well::Data& w )
	: Well::IO(f,true)
    	, wd(w)
{
}


const char* Well::Reader::rdHdr( istream& strm, const char* fileky ) const
{
    ascistream astrm( strm, YES );
    if ( !astrm.isOfFileType(fileky) )
    {
	BufferString msg( "Opened file has type '" );
	msg += astrm.fileType(); msg += "' whereas it should be '";
	msg += fileky; msg += "'";
	ErrMsg( msg );
	return 0;
    }
    static BufferString ver; ver = astrm.version();
    return ver.buf();
}


bool Well::Reader::get() const
{
    if ( !getInfo() )
	return false;

    getLogs();
    getMarkers();
    getD2T();
    return true;
}


bool Well::Reader::getInfo() const
{
    StreamData sd = mkSD( sExtWell );
    if ( !sd.usable() ) return false;

    const bool isok = getInfo( *sd.istrm );
    sd.close();
    return isok;
}


bool Well::Reader::getInfo( istream& strm ) const
{
    const char* ver = rdHdr( strm, sKeyWell );
    if ( !ver || !*ver || !*(ver+1) )
	return false;
    if ( *ver < '2' && *(ver+2) < '6' )
	return getOldTimeWell(strm);

    return true;
}


bool Well::Reader::getOldTimeWell( istream& strm ) const
{
    ascistream astrm( strm, false );
    astrm.next();
    while ( !atEndOfSection(astrm) ) astrm.next();

    // get track
    Coord3 c3, prevc, c0;
    float z;
    float dah = 0;
    while ( strm )
    {
	strm >> c3.x >> c3.y >> c3.z;
	if ( c3.distance(c0) < 1 ) break;

	wd.track().addPoint( c3, c3.z, dah );
	if ( wd.track().nrPoints() > 1 )
	    dah += c3.distance( prevc );
	prevc = c3;
    }
    if ( wd.track().nrPoints() < 1 )
	return false;

    wd.info().surfacecoord = wd.track().pos(0);
    wd.info().setName( File_getPathOnly(basenm) );

    // create T2D
    D2TModel* d2t = new D2TModel( Well::D2TModel::sKeyTimeWell );
    wd.setD2TModel( d2t );
    for ( int idx=0; idx<wd.track().nrPoints(); idx++ )
    {
	float dah = wd.track().dah(idx);
	d2t->dah += dah;
	d2t->t += dah;
    }

    return true;
}


bool Well::Reader::getLogs() const
{
    bool rv = false;
    for ( int idx=1;  ; idx++ )
    {
	StreamData sd = mkSD( sExtLog, idx );
	if ( !sd.usable() ) break;

	if ( !addLog(*sd.istrm) )
	{
	    BufferString msg( "Could not read data from " );
	    msg += File_getFileName(basenm.buf());
	    msg += " log #";
	    msg += idx;
	    ErrMsg( msg );
	    continue;
	}

	rv = true;
    }

    return rv;
}


bool Well::Reader::addLog( istream& strm ) const
{
    return false;
}


bool Well::Reader::getMarkers() const
{
    StreamData sd = mkSD( sExtMarkers );
    if ( !sd.usable() ) return false;

    const bool isok = getMarkers( *sd.istrm );
    sd.close();
    return isok;
}


bool Well::Reader::getMarkers( istream& strm ) const
{
    return true;
}


bool Well::Reader::getD2T() const
{
    StreamData sd = mkSD( sExtD2T );
    if ( !sd.usable() ) return false;

    const bool isok = getD2T( *sd.istrm );
    sd.close();
    return isok;
}


bool Well::Reader::getD2T( istream& strm ) const
{
    return true;
}
