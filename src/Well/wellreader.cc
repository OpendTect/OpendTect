/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID = "$Id: wellreader.cc,v 1.5 2003-08-22 16:40:34 bert Exp $";

#include "wellreader.h"
#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "ascstream.h"
#include "filegen.h"
#include "errh.h"
#include "strmprov.h"
#include "keystrs.h"
#include "separstr.h"
#include "iopar.h"
#include <fstream>

const char* Well::IO::sKeyWell = "Well";
const char* Well::IO::sKeyLog = "Well Log";
const char* Well::IO::sKeyMarkers = "Well Markers";
const char* Well::IO::sKeyD2T = "Depth2Time Model";
const char* Well::IO::sExtWell = ".well";
const char* Well::IO::sExtLog = ".wll";
const char* Well::IO::sExtMarkers = ".wlm";
const char* Well::IO::sExtD2T = ".wlt";


Well::IO::IO( const char* f, bool fr )
    	: basenm(f)
    	, isrdr(fr)
{
    BufferString fnm = File_getFileName( basenm.buf() );
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
    fnm += ext;
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
    wd.setD2TModel( 0 );
    if ( !getInfo() )
	return false;
    else if ( wd.d2TModel() )
	return true;

    getLogs();
    getMarkers();
    getD2T();
    return true;
}


bool Well::Reader::getInfo() const
{
    StreamData sd = mkSD( sExtWell );
    if ( !sd.usable() ) return false;

    wd.info().setName( getFileName(sExtWell) );
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

    ascistream astrm( strm, NO );
    while ( !atEndOfSection(astrm.next()) )
    {
	if ( astrm.hasKeyword(Well::Info::sKeyuwid) )
	    wd.info().uwid = astrm.value();
	else if ( astrm.hasKeyword(Well::Info::sKeyoper) )
	    wd.info().oper = astrm.value();
	else if ( astrm.hasKeyword(Well::Info::sKeystate) )
	    wd.info().state = astrm.value();
	else if ( astrm.hasKeyword(Well::Info::sKeycounty) )
	    wd.info().county = astrm.value();
	else if ( astrm.hasKeyword(Well::Info::sKeycoord) )
	    wd.info().surfacecoord.use( astrm.value() );
	else if ( astrm.hasKeyword(Well::Info::sKeyelev) )
	    wd.info().surfaceelev = astrm.getValue();
    }

    return getTrack( strm );
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
	if ( !strm || c3.distance(c0) < 1 ) break;

	if ( wd.track().size() > 0 )
	    dah += c3.distance( prevc );
	wd.track().addPoint( c3, c3.z, dah );
	prevc = c3;
    }
    if ( wd.track().size() < 1 )
	return false;

    wd.info().surfacecoord = wd.track().pos(0);
    wd.info().setName( File_getPathOnly(basenm) );

    // create T2D
    D2TModel* d2t = new D2TModel( Well::D2TModel::sKeyTimeWell );
    wd.setD2TModel( d2t );
    for ( int idx=0; idx<wd.track().size(); idx++ )
    {
	float dah = wd.track().dah(idx);
	d2t->add( wd.track().dah(idx), wd.track().pos(idx).z );
    }

    return true;
}


bool Well::Reader::getTrack( istream& strm ) const
{
    Coord3 c, c0; float dah;
    while ( strm )
    {
	strm >> dah >> c.x >> c.y >> c.z;
	if ( !strm || c.distance(c0) < 1 ) break;
	wd.track().addPoint( c, c.z, dah );
    }
    return wd.track().size();
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


Well::Log* Well::Reader::rdLogHdr( istream& strm, int idx ) const
{
    Well::Log* newlog = new Well::Log;
    ascistream astrm( strm, NO );
    while ( !atEndOfSection(astrm.next()) )
    {
	if ( astrm.hasKeyword(sKey::Name) )
	    newlog->setName( astrm.value() );
    }
    if ( newlog->name() == "" )
    {
	BufferString nm( "[" ); nm += idx+1; nm += "]";
	newlog->setName( nm );
    }
    return newlog;
}


bool Well::Reader::addLog( istream& strm ) const
{
    const char* ver = rdHdr( strm, sKeyLog );
    if ( !ver ) return false;

    Well::Log* newlog = rdLogHdr( strm, wd.logs().size() );

    float dah, val;
    while ( strm )
    {
	strm >> dah >> val;
	if ( !strm ) break;

	/* Useful for import, not here
	if ( mIsUndefined(dah) || mIS_ZERO(dah-prevdah)
	|| ( !newlog->nrValues() && mIsUndefined(val) ) )
	    continue;
	*/

	newlog->addValue( dah, val );
    }

    /* Useful for import, not here
    for ( int idx=newlog->nrValues()-1; idx>=0; idx-- )
    {
	dah = newlog->dah(idx);
	val = newlog->value(idx);
	if ( mIsUndefined(val) || (mIS_ZERO(dah) && mIS_ZERO(val)) )
	    newlog->removeValue(idx);
    }
    */

    wd.logs().add( newlog );
    return true;
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
    if ( !rdHdr(strm,sKeyMarkers) ) return false;

    ascistream astrm( strm, NO );
    IOPar iopar( astrm, false );
    if ( !iopar.size() ) return false;

    BufferString bs;
    for ( int idx=1;  ; idx++ )
    {
	BufferString basekey; basekey += idx;
	BufferString key = IOPar::compKey( basekey, sKey::Name );
	if ( !iopar.get(key,bs) ) break;

	Well::Marker* wm = new Well::Marker( bs );
	key = IOPar::compKey( basekey, sKey::Desc );
	if ( iopar.get(key,bs) )
	{
	    FileMultiString fms( bs );
	    wm->istop = *fms[0] == 'T';
	    wm->desc = fms[1];
	}
	key = IOPar::compKey( basekey, sKey::Color );
	if ( iopar.get(key,bs) )
	    wm->color.use( bs.buf() );
	key = IOPar::compKey( basekey, Well::Marker::sKeyDah );
	if ( !iopar.get(key,bs) )
	    { delete wm; continue; }
	wm->dah = atof( bs.buf() );
	wd.markers() += wm;
    }

    return wd.markers().size();
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
    if ( !rdHdr(strm,sKeyD2T) ) return false;

    ascistream astrm( strm, NO );
    Well::D2TModel* d2t = new Well::D2TModel;
    while ( !atEndOfSection(astrm.next()) )
    {
	if ( astrm.hasKeyword(sKey::Name) )
	    d2t->setName( astrm.value() );
	else if ( astrm.hasKeyword(sKey::Desc) )
	    d2t->desc = astrm.value();
	else if ( astrm.hasKeyword(Well::D2TModel::sKeyDataSrc) )
	    d2t->datasource = astrm.value();
    }

    float dah, val;
    while ( strm )
    {
	strm >> dah >> val;
	if ( !strm ) break;
	d2t->add( dah, val );
    }
    if ( d2t->size() < 2 )
	{ delete d2t; d2t = 0; }

    wd.setD2TModel( d2t );
    return d2t ? true : false;
}
