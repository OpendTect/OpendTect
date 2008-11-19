/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID = "$Id: wellreader.cc,v 1.27 2008-11-19 20:24:24 cvsbert Exp $";

#include "wellreader.h"
#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "ascstream.h"
#include "filegen.h"
#include "filepath.h"
#include "errh.h"
#include "strmprov.h"
#include "keystrs.h"
#include "separstr.h"
#include "iopar.h"
#include "ptrman.h"
#include "bufstringset.h"
#include <iostream>

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
    FilePath fp( basenm );
    fp.setExtension( 0, true );
    const_cast<BufferString&>(basenm) = fp.fullPath();
}


StreamData Well::IO::mkSD( const char* ext, int nr ) const
{
    StreamProvider sp( getFileName(ext,nr) );
    return isrdr ? sp.makeIStream() : sp.makeOStream();
}


const char* Well::IO::getFileName( const char* ext, int nr ) const
{
    return mkFileName( basenm, ext, nr );
}


const char* Well::IO::mkFileName( const char* bnm, const char* ext, int nr )
{
    static BufferString fnm;
    fnm = bnm;
    if ( nr )
	{ fnm += "^"; fnm += nr; }
    fnm += ext;
    return fnm;
}


bool Well::IO::removeAll( const char* ext ) const
{
    for ( int idx=1; ; idx++ )
    {
	BufferString fnm( getFileName(ext,idx) );
	if ( !File_exists(fnm) )
	    break;
	else if ( !File_remove(fnm,mFile_NotRecursive) )
	    return false;
    }
    return true;
}


Well::Reader::Reader( const char* f, Well::Data& w )
	: Well::IO(f,true)
    	, wd(w)
{
}


const char* Well::Reader::rdHdr( std::istream& strm, const char* fileky ) const
{
    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType(fileky) )
    {
	BufferString msg( "Opened file has type '" );
	msg += astrm.fileType(); msg += "' whereas it should be '";
	msg += fileky; msg += "'";
	ErrMsg( msg );
	return 0;
    }

    static BufferString hdrln; hdrln = astrm.headerStartLine();
    return hdrln.buf();
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


bool Well::Reader::getInfo( std::istream& strm ) const
{
    const char* hdrln = rdHdr( strm, sKeyWell );
    if ( !hdrln )
	return false;
    bool badhdr = *hdrln != 'd';
    if ( !badhdr )
    {
	if ( *(hdrln+1) == 'G' )
	    return getOldTimeWell(strm);
	else if ( *(hdrln+1) != 'T' )
	    badhdr = true;
    }
    if ( badhdr )
	{ ErrMsg("Bad file header for main well file"); return false; }

    ascistream astrm( strm, false );
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
	    wd.info().surfaceelev = astrm.getFValue();
    }

    return getTrack( strm );
}


bool Well::Reader::getOldTimeWell( std::istream& strm ) const
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
	if ( !strm || c3.distTo(c0) < 1 ) break;

	if ( !wd.track().isEmpty() )
	    dah += c3.distTo( prevc );
	wd.track().addPoint( c3, c3.z, dah );
	prevc = c3;
    }
    if ( wd.track().size() < 1 )
	return false;

    wd.info().surfacecoord = wd.track().pos(0);
    wd.info().setName( FilePath(basenm).fileName() );

    // create T2D
    D2TModel* d2t = new D2TModel( Well::D2TModel::sKeyTimeWell );
    wd.setD2TModel( d2t );
    for ( int idx=0; idx<wd.track().size(); idx++ )
	d2t->add( wd.track().dah(idx), wd.track().pos(idx).z );

    return true;
}


bool Well::Reader::getTrack( std::istream& strm ) const
{
    Coord3 c, c0; float dah;
    while ( strm )
    {
	strm >> c.x >> c.y >> c.z >> dah;
	if ( !strm || c.distTo(c0) < 1 ) break;
	wd.track().addPoint( c, c.z, dah );
    }
    return wd.track().size();
}


void Well::Reader::getLogInfo( BufferStringSet& strs ) const
{
    for ( int idx=1;  ; idx++ )
    {
	StreamData sd = mkSD( sExtLog, idx );
	if ( !sd.usable() ) break;

	if ( rdHdr(*sd.istrm,sKeyLog) )
	{
	    PtrMan<Well::Log> log = rdLogHdr( *sd.istrm, idx-1 );
	    strs.add( log->name() );
	}
	sd.close();
    }
}


Interval<float> Well::Reader::getLogDahRange( const char* nm ) const
{
    Interval<float> ret( mUdf(float), mUdf(float) );
    if ( !nm || !*nm ) return ret;

    for ( int idx=1;  ; idx++ )
    {
	StreamData sd = mkSD( sExtLog, idx );
	if ( !sd.usable() ) break;
	std::istream& strm = *sd.istrm;

	if ( !rdHdr(strm,sKeyLog) )
	    { sd.close(); continue; }
	PtrMan<Well::Log> log = rdLogHdr( strm, idx-1 );
	if ( log->name() != nm )
	    { sd.close(); continue; }

	float dah, val;
	strm >> dah >> val;
	if ( !strm )
	    { sd.close(); break; }

	ret.start = dah;
	while ( strm )
	    strm >> dah >> val;
	ret.stop = dah;
	sd.close(); break;
    }

    return ret;
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
	    msg += FilePath(basenm).fileName();
	    msg += " log #";
	    msg += idx;
	    ErrMsg( msg );
	    continue;
	}

	rv = true;
	sd.close();
    }

    return rv;
}


Well::Log* Well::Reader::rdLogHdr( std::istream& strm, int idx ) const
{
    Well::Log* newlog = new Well::Log;
    ascistream astrm( strm, false );
    while ( !atEndOfSection(astrm.next()) )
    {
	if ( astrm.hasKeyword(sKey::Name) )
	    newlog->setName( astrm.value() );
	if ( astrm.hasKeyword(Well::Log::sKeyUnitLbl) )
	    newlog->setUnitMeasLabel( astrm.value() );
    }
    if ( newlog->name().isEmpty() )
    {
	BufferString nm( "[" ); nm += idx+1; nm += "]";
	newlog->setName( nm );
    }
    return newlog;
}


bool Well::Reader::addLog( std::istream& strm ) const
{
    if ( !rdHdr(strm,sKeyLog) )
	return false;

    Well::Log* newlog = rdLogHdr( strm, wd.logs().size() );

    float dah, val;
    while ( strm )
    {
	strm >> dah >> val;
	if ( !strm ) break;

	newlog->addValue( dah, val );
    }

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


bool Well::Reader::getMarkers( std::istream& strm ) const
{
    if ( !rdHdr(strm,sKeyMarkers) )
	return false;

    ascistream astrm( strm, false );
    IOPar iopar( astrm );
    if ( iopar.isEmpty() ) return false;

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
	    wm->istop_ = *fms[0] == 'T';
	    wm->desc_ = fms[1];
	}
	key = IOPar::compKey( basekey, sKey::Color );
	if ( iopar.get(key,bs) )
	    wm->color_.use( bs.buf() );
	key = IOPar::compKey( basekey, Well::Marker::sKeyDah );
	if ( !iopar.get(key,bs) )
	    { delete wm; continue; }
	wm->dah_ = atof( bs.buf() );
	key = IOPar::compKey( basekey, sKey::StratRef );
	iopar.get( key, wm->stratlevelid_ );
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


bool Well::Reader::getD2T( std::istream& strm ) const
{
    if ( !rdHdr(strm,sKeyD2T) )
	return false;

    ascistream astrm( strm, false );
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
