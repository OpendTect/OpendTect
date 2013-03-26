/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "wellreader.h"

#include "ascstream.h"
#include "bufstringset.h"
#include "errh.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "ioobj.h"
#include "ioman.h"
#include "keystrs.h"
#include "ptrman.h"
#include "separstr.h"
#include "strmprov.h"
#include "survinfo.h"
#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "welldisp.h"

#include <iostream>

const char* Well::IO::sKeyWell()	{ return "Well"; }
const char* Well::IO::sKeyTrack()	{ return "Track"; }
const char* Well::IO::sKeyLog()		{ return "Well Log"; }
const char* Well::IO::sKeyMarkers()	{ return "Well Markers"; }
const char* Well::IO::sKeyD2T()		{ return "Depth2Time Model"; }
const char* Well::IO::sKeyDispProps()	{ return "Display Properties"; }
const char* Well::IO::sExtWell()	{ return ".well"; }
const char* Well::IO::sExtTrack()	{ return ".track"; }
const char* Well::IO::sExtLog()		{ return ".wll"; }
const char* Well::IO::sExtMarkers()	{ return ".wlm"; }
const char* Well::IO::sExtD2T()		{ return ".wlt"; }
const char* Well::IO::sExtCSMdl()	{ return ".csmdl"; }
const char* Well::IO::sExtDispProps()	{ return ".disp"; }
const char* Well::IO::sExtWellTieSetup() { return ".tie"; }


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


const char* Well::IO::getMainFileName( const IOObj& ioobj )
{
    return ioobj.fullUserExpr( true );
}


const char* Well::IO::getMainFileName( const MultiID& mid )
{
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj ) return 0;
    return getMainFileName( *ioobj );
}


bool Well::IO::removeAll( const char* ext ) const
{
    for ( int idx=1; ; idx++ )
    {
	BufferString fnm( getFileName(ext,idx) );
	if ( !File::exists(fnm) )
	    break;
	else if ( !File::remove(fnm) )
	    return false;
    }
    return true;
}


static const char* rdHdr( std::istream& strm, const char* fileky,
			  double& ver )
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

    ver = (double)astrm.majorVersion() +
	  ((double)astrm.minorVersion() / (double)10);
    static BufferString hdrln; hdrln = astrm.headerStartLine();
    return hdrln.buf();
}


Well::Reader::Reader( const char* f, Well::Data& w )
	: Well::IO(f,true)
    	, wd(w)
{
}


bool Well::Reader::get() const
{
    wd.setD2TModel( 0 );
    wd.setCheckShotModel( 0 );
    if ( !getInfo() )
	return false;
    else if ( wd.d2TModel() )
	return true;

    getLogs();
    getMarkers();
    getD2T();
    getCSMdl();
    getDispProps();
    return true;
}


bool Well::Reader::getInfo() const
{
    StreamData sd = mkSD( sExtWell() );
    if ( !sd.usable() ) return false;

    wd.info().setName( getFileName(sExtWell()) );
    const bool isok = getInfo( *sd.istrm );

    sd.close();
    return isok;
}


bool Well::Reader::getInfo( std::istream& strm ) const
{
    double version = 0.0;
    const char* hdrln = rdHdr( strm, sKeyWell(), version );
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
	if ( astrm.hasKeyword(Well::Info::sKeyuwid()) )
	    wd.info().uwid = astrm.value();
	else if ( astrm.hasKeyword(Well::Info::sKeyoper()) )
	    wd.info().oper = astrm.value();
	else if ( astrm.hasKeyword(Well::Info::sKeystate()) )
	    wd.info().state = astrm.value();
	else if ( astrm.hasKeyword(Well::Info::sKeycounty()) )
	    wd.info().county = astrm.value();
	else if ( astrm.hasKeyword(Well::Info::sKeycoord()) )
	    wd.info().surfacecoord.use( astrm.value() );
	else if ( astrm.hasKeyword(Well::Info::sKeyOldelev()) )
	{
	    const float readsurfelev = astrm.getFValue(); //needed for old files
	    wd.info().srdelev = mIsUdf(readsurfelev ) ? 0  :
	       			    -1.f * readsurfelev;
	}
	else if ( astrm.hasKeyword(Well::Info::sKeySRD()) )
	    wd.info().srdelev = astrm.getFValue();
	else if ( astrm.hasKeyword(Well::Info::sKeyreplvel()) )
	    wd.info().replvel = astrm.getFValue();
	else if ( astrm.hasKeyword(Well::Info::sKeygroundelev()) )
	    wd.info().groundelev = astrm.getFValue();
    }

    if ( !getTrack(strm) )
	return false;

    if ( SI().zInFeet() && version < 4.195 )
    {
	Well::Track& welltrack = wd.track();
	for ( int idx=0; idx<welltrack.size(); idx++ )
	{
	    Coord3 pos = welltrack.pos( idx );
	    pos.z *= mToFeetFactorF;
	    welltrack.setPoint( idx, pos, (float) pos.z );
	}
    }

    return true;
}


bool Well::Reader::getOldTimeWell( std::istream& strm ) const
{
    ascistream astrm( strm, false );
    astrm.next();
    while ( !atEndOfSection(astrm) ) astrm.next();

    // get track
    Coord3 c3, prevc, c0;
    float dah = 0;
    while ( strm )
    {
	strm >> c3.x >> c3.y >> c3.z;
	if ( !strm || c3.distTo(c0) < 1 ) break;

	if ( !wd.track().isEmpty() )
	    dah += (float) c3.distTo( prevc );
	wd.track().addPoint( c3, (float) c3.z, dah );
	prevc = c3;
    }
    if ( wd.track().size() < 1 )
	return false;

    wd.info().surfacecoord = wd.track().pos(0);
    wd.info().setName( FilePath(basenm).fileName() );

    // create T2D
    D2TModel* d2t = new D2TModel( Well::D2TModel::sKeyTimeWell() );
    wd.setD2TModel( d2t );
    for ( int idx=0; idx<wd.track().size(); idx++ )
	d2t->add( wd.track().dah(idx),(float) wd.track().pos(idx).z );

    return true;
}


bool Well::Reader::getTrack( std::istream& strm ) const
{
    Coord3 c, c0; float dah;
    while ( strm )
    {
	strm >> c.x >> c.y >> c.z >> dah;
	if ( !strm || c.distTo(c0) < 1 ) break;
	wd.track().addPoint( c, (float) c.z, dah );
    }
    if ( wd.track().isEmpty() )
	return false;

    if ( wd.info().surfacecoord == Coord(0,0) )
	wd.info().surfacecoord = wd.track().pos(0);
    return true;
}


bool Well::Reader::getTrack() const
{
    StreamData sd = mkSD( sExtTrack() );
    bool isold = false;
    if ( !sd.usable() )
    {
	sd = mkSD( ".well" );
	if ( !sd.usable() )
	    return false;
	isold = true;
    }

    ascistream astrm( *sd.istrm );
    const double version = (double)astrm.majorVersion() +
			   ((double)astrm.minorVersion()/(double)10);
    if ( isold )
	{ IOPar dum; dum.getFrom( astrm ); }

    const bool isok = getTrack( *sd.istrm );
    if ( SI().zInFeet() && version < 4.195 )
    {
	Well::Track& welltrack = wd.track();
	for ( int idx=0; idx<welltrack.size(); idx++ )
	{
	    Coord3 pos = welltrack.pos( idx );
	    pos.z *= mToFeetFactorF;
	    welltrack.setPoint( idx, pos, (float) pos.z );
	}
    }

    sd.close();
    return isok;
}


void Well::Reader::getLogInfo( BufferStringSet& strs ) const
{
    for ( int idx=1;  ; idx++ )
    {
	StreamData sd = mkSD( sExtLog(), idx );
	if ( !sd.usable() ) break;

	double version = 0.0;
	if ( rdHdr(*sd.istrm,sKeyLog(),version) )
	{
	    int bintyp = 0;
	    PtrMan<Well::Log> log = rdLogHdr( *sd.istrm, bintyp, idx-1 );
	    if ( strs.isPresent( log->name() ) )
	    {
		BufferString msg(log->name());
		msg += " already present in the list, won't be read";
		pErrMsg( msg );
	    }
	    else
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
	StreamData sd = mkSD( sExtLog(), idx );
	if ( !sd.usable() ) break;
	std::istream& strm = *sd.istrm;

	double version = 0.0;
	if ( !rdHdr(strm,sKeyLog(),version) )
	    { sd.close(); continue; }

	int bintype = 0;
	PtrMan<Well::Log> log = rdLogHdr( strm, bintype, wd.logs().size() );
	if ( log->name() != nm )
	    { sd.close(); continue; }

	readLogData( *log, strm, bintype );
	sd.close();
	
	const bool valinmtr = SI().zInFeet() && (version < 4.195);

	ret.start = valinmtr ? (log->dah(0) * mToFeetFactorF) : log->dah(0);
	ret.stop = valinmtr ? (log->dah(log->size()-1) * mToFeetFactorF )
	    		    : log->dah( log->size()-1 );
	break;
    }

    return ret;
}


Interval<float> Well::Reader::getAllLogsDahRange() const
{
    Interval<float> ret( mUdf(float), mUdf(float) );
    BufferStringSet lognms; getLogInfo( lognms );

    int ilog = 0;
    for ( ; mIsUdf(ret.start) && ilog<lognms.size(); ilog++ )
	ret = getLogDahRange( lognms.get(ilog) );

    for ( ; ilog<lognms.size(); ilog++ )
    {
	Interval<float> dahrg = getLogDahRange( lognms.get(ilog) );
	if ( mIsUdf(dahrg.start) ) continue;
	ret.include( dahrg );
    }
    return ret;
}


bool Well::Reader::getLogs() const
{
    bool rv = false;
    wd.logs().setEmpty();
    for ( int idx=1;  ; idx++ )
    {
	StreamData sd = mkSD( sExtLog(), idx );
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


Well::Log* Well::Reader::rdLogHdr( std::istream& strm, int& bintype, int idx )
{
    Well::Log* newlog = new Well::Log;
    ascistream astrm( strm, false );
    bool havehdrinfo = false;
    bintype = 0;
    while ( !atEndOfSection(astrm.next()) )
    {
	if ( astrm.hasKeyword(sKey::Name()) )
	    newlog->setName( astrm.value() );
	if ( astrm.hasKeyword(Well::Log::sKeyUnitLbl()) )
	    newlog->setUnitMeasLabel( astrm.value() );
	if ( astrm.hasKeyword(Well::Log::sKeyHdrInfo()) )
	    havehdrinfo = astrm.getYN();
	if ( astrm.hasKeyword(Well::Log::sKeyStorage()) )
	    bintype = *astrm.value() == 'B' ? 1
		    : (*astrm.value() == 'S' ? -1 : 0);
    }
    if ( newlog->name().isEmpty() )
    {
	BufferString nm( "[" ); nm += idx+1; nm += "]";
	newlog->setName( nm );
    }

    if ( havehdrinfo )
	newlog->pars().getFrom( astrm );

    return newlog;
}


bool Well::Reader::addLog( std::istream& strm ) const
{
    double version = 0.0;
    if ( !rdHdr(strm,sKeyLog(),version) )
	return false;

    int bintype = 0;
    Well::Log* newlog = rdLogHdr( strm, bintype, wd.logs().size() );
    if ( !newlog )
	return false;

    readLogData( *newlog, strm, bintype );
    
    if ( SI().zInFeet() && version < 4.195 )
    {
	for ( int idx=0; idx<newlog->size(); idx++ )
	    newlog->dahArr()[idx] = newlog->dah(idx) * mToFeetFactorF;
    }

    wd.logs().add( newlog );
    return true;
}


void Well::Reader::readLogData( Well::Log& wl, std::istream& strm,
       				int bintype ) const
{

    float v[2];
    while ( strm )
    {
	if ( !bintype )
	    strm >> v[0] >> v[1];
	else
	{
	    strm.read( (char*)v, 2 * sizeof(float) );
	    if ( (bintype > 0) != __islittle__ )
	    {
		SwapBytes( v, sizeof(float) );
		SwapBytes( v+1, sizeof(float) );
	    }
	}
	if ( !strm ) break;

	wl.addValue( v[0], v[1] );
    }
}


bool Well::Reader::getMarkers() const
{
    StreamData sd = mkSD( sExtMarkers() );
    if ( !sd.usable() ) return false;

    const bool isok = getMarkers( *sd.istrm );
    sd.close();
    return isok;
}


bool Well::Reader::getMarkers( std::istream& strm ) const
{
    double version = 0.0;
    if ( !rdHdr(strm,sKeyMarkers(),version) )
	return false;

    ascistream astrm( strm, false );
    
    IOPar iopar( astrm );
    if ( iopar.isEmpty() ) return false;

    wd.markers().erase();
    BufferString bs;
    for ( int idx=1;  ; idx++ )
    {
	BufferString basekey; basekey += idx;
	BufferString key = IOPar::compKey( basekey, sKey::Name() );
	if ( !iopar.get(key,bs) ) break;

	Well::Marker* wm = new Well::Marker( bs );

	key = IOPar::compKey( basekey, Well::Marker::sKeyDah() );
	if ( !iopar.get(key,bs) )
	    { delete wm; continue; }
	float val = toFloat( bs.buf() );
	wm->setDah( (SI().zInFeet() && version<4.195) ? (val*mToFeetFactorF)
						      : val ); 
	key = IOPar::compKey( basekey, sKey::StratRef() );
	int lvlid = -1; iopar.get( key, lvlid );
	wm->setLevelID( lvlid );

	key = IOPar::compKey( basekey, sKey::Color() );
	if ( iopar.get(key,bs) )
	{
	    Color col( wm->color() );
	    col.use( bs.buf() );
	    wm->setColor( col );
	}

	wd.markers() += wm;
    }

    return wd.markers().size();
}


bool Well::Reader::getD2T() const	{ return doGetD2T( false ); }
bool Well::Reader::getCSMdl() const	{ return doGetD2T( true ); }
bool Well::Reader::doGetD2T( bool csmdl ) const
{
    StreamData sd = mkSD( csmdl ? sExtCSMdl() : sExtD2T() );
    if ( !sd.usable() ) return false;

    const bool isok = doGetD2T( *sd.istrm, csmdl );
    sd.close();
    return isok;
}


bool Well::Reader::getD2T( std::istream& strm ) const
{ return doGetD2T(strm,false); }
bool Well::Reader::getCSMdl( std::istream& strm ) const
{ return doGetD2T(strm,true); }
bool Well::Reader::doGetD2T( std::istream& strm, bool csmdl ) const
{
    double version = 0.0;
    if ( !rdHdr(strm,sKeyD2T(),version) )
	return false;

    ascistream astrm( strm, false );
    Well::D2TModel* d2t = new Well::D2TModel;
    while ( !atEndOfSection(astrm.next()) )
    {
	if ( astrm.hasKeyword(sKey::Name()) )
	    d2t->setName( astrm.value() );
	else if ( astrm.hasKeyword(sKey::Desc()) )
	    d2t->desc = astrm.value();
	else if ( astrm.hasKeyword(Well::D2TModel::sKeyDataSrc()) )
	    d2t->datasource = astrm.value();
    }

    float dah, val;
    while ( strm )
    {
	strm >> dah >> val;
	if ( !strm ) break;
	d2t->add( dah, val );
    }
    if ( d2t->size() < (csmdl ? 1 : 2) )
	{ delete d2t; d2t = 0; }

    if ( csmdl )
	wd.setCheckShotModel( d2t );
    else
	wd.setD2TModel( d2t );
    return d2t ? true : false;
}


bool Well::Reader::getDispProps() const
{
    StreamData sd = mkSD( sExtDispProps() );
    if ( !sd.usable() ) return false;

    const bool isok = getDispProps( *sd.istrm );
    sd.close();
    return isok;
}


bool Well::Reader::getDispProps( std::istream& strm ) const
{
    double version = 0.0;
    if ( !rdHdr(strm,sKeyDispProps(),version) )
	return false;

    ascistream astrm( strm, false );
    IOPar iop; iop.getFrom( astrm );
    wd.displayProperties(true).usePar( iop );
    wd.displayProperties(false).usePar( iop );
    return true;
}
