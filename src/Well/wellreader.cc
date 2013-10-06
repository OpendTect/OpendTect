/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "wellreader.h"

#include "ascstream.h"
#include "bufstringset.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "ioobj.h"
#include "ioman.h"
#include "keystrs.h"
#include "ptrman.h"
#include "separstr.h"
#include "survinfo.h"
#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "welldisp.h"
#include "od_istream.h"

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



Well::IO::IO( const char* f )
    	: basenm_(f)
{
    FilePath fp( basenm_ );
    fp.setExtension( 0, true );
    const_cast<BufferString&>(basenm_) = fp.fullPath();
}


const char* Well::IO::getFileName( const char* ext, int nr ) const
{
    return mkFileName( basenm_, ext, nr );
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


static const char* rdHdr( od_istream& strm, const char* fileky,
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


#define mGetInpStream(ext,nr,todo) \
    od_istream strm( getFileName(ext,nr) ); \
    if ( !strm.isOK() ) { todo; }



Well::Reader::Reader( const char* f, Well::Data& w )
	: Well::IO(f)
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
    mGetInpStream( sExtWell(), 0, return false );

    wd.info().setName( getFileName(sExtWell()) );
    return getInfo( strm );
}


bool Well::Reader::getInfo( od_istream& strm ) const
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
	    wd.info().surfacecoord.parseUsrStr( astrm.value() );
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


bool Well::Reader::getOldTimeWell( od_istream& strm ) const
{
    ascistream astrm( strm, false );
    astrm.next();
    while ( !atEndOfSection(astrm) ) astrm.next();

    // get track
    Coord3 c3, prevc, c0;
    float dah = 0;
    while ( strm.isOK() )
    {
	strm >> c3.x >> c3.y >> c3.z;
	if ( !strm.isOK() || c3.distTo(c0) < 1 ) break;

	if ( !wd.track().isEmpty() )
	    dah += (float) c3.distTo( prevc );
	wd.track().addPoint( c3, (float) c3.z, dah );
	prevc = c3;
    }
    if ( wd.track().size() < 1 )
	return false;

    wd.info().surfacecoord = wd.track().pos(0);
    wd.info().setName( FilePath(basenm_).fileName() );

    // create T2D
    D2TModel* d2t = new D2TModel( Well::D2TModel::sKeyTimeWell() );
    wd.setD2TModel( d2t );
    for ( int idx=0; idx<wd.track().size(); idx++ )
	d2t->add( wd.track().dah(idx),(float) wd.track().pos(idx).z );

    return true;
}


bool Well::Reader::getTrack( od_istream& strm ) const
{
    Coord3 c, c0; float dah;
    while ( strm.isOK() )
    {
	strm >> c.x >> c.y >> c.z >> dah;
	if ( !strm.isOK() || c.distTo(c0) < 1 ) break;
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
    bool isold = false;
    mGetInpStream( sExtTrack(), 0, isold = true );
    if ( isold )
    {
	strm.open( getFileName(".well",0) );
	if ( !strm.isOK() )
	    return false;
    }

    ascistream astrm( strm );
    const double version = (double)astrm.majorVersion() +
			   ((double)astrm.minorVersion()/(double)10);
    if ( isold )
	{ IOPar dum; dum.getFrom( astrm ); }

    const bool isok = getTrack( strm );
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

    return isok;
}


void Well::Reader::getLogInfo( BufferStringSet& strs ) const
{
    for ( int idx=1;  ; idx++ )
    {
	mGetInpStream( sExtLog(), idx, break );

	double version = 0.0;
	if ( rdHdr(strm,sKeyLog(),version) )
	{
	    int bintyp = 0;
	    PtrMan<Well::Log> log = rdLogHdr( strm, bintyp, idx-1 );
	    if ( strs.isPresent( log->name() ) )
	    {
		BufferString msg(log->name());
		msg += " already present in the list, won't be read";
		pErrMsg( msg );
	    }
	    else
		strs.add( log->name() );
	}
    }
}


Interval<float> Well::Reader::getLogDahRange( const char* nm ) const
{
    Interval<float> ret( mUdf(float), mUdf(float) );
    if ( !nm || !*nm ) return ret;

    for ( int idx=1;  ; idx++ )
    {
	mGetInpStream( sExtLog(), idx, break );

	double version = 0.0;
	if ( !rdHdr(strm,sKeyLog(),version) )
	    continue;

	int bintype = 0;
	PtrMan<Well::Log> log = rdLogHdr( strm, bintype, wd.logs().size() );
	if ( log->name() != nm )
	    continue;

	readLogData( *log, strm, bintype );
	if ( log->isEmpty() )
	    continue;
	
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
	mGetInpStream( sExtLog(), idx, break );

	if ( !addLog(strm) )
	{
	    BufferString msg( "Could not read data from " );
	    msg.add( FilePath(basenm_).fileName() );
	    msg.add( " log #" ).add( idx );
	    strm.addErrMsgTo( msg );
	    ErrMsg( msg );
	    continue;
	}

	rv = true;
    }

    return rv;
}


Well::Log* Well::Reader::rdLogHdr( od_istream& strm, int& bintype, int idx )
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


bool Well::Reader::addLog( od_istream& strm ) const
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

    if ( !wd.track().dahRange().width() )
	getTrack();

    const float stopz = wd.track().dahRange().stop;
    for ( int idx=newlog->size()-1; idx>=0; idx-- )
    {
	if ( newlog->dahArr()[idx] > stopz )
	    newlog->valArr()[idx] = mUdf(float);
    }

    newlog->removeTopBottomUdfs();
    wd.logs().add( newlog );

    return true;
}


void Well::Reader::readLogData( Well::Log& wl, od_istream& strm,
       				int bintype ) const
{

    float v[2];
    while ( strm.isOK() )
    {
	if ( !bintype )
	    strm >> v[0] >> v[1];
	else
	{
	    strm.getBin( (char*)v, 2 * sizeof(float) );
	    if ( (bintype > 0) != __islittle__ )
	    {
		SwapBytes( v, sizeof(float) );
		SwapBytes( v+1, sizeof(float) );
	    }
	}
	if ( !strm.isOK() ) break;

	wl.addValue( v[0], v[1] );
    }
}


bool Well::Reader::getMarkers() const
{
    mGetInpStream( sExtMarkers(), 0, return false );
    return getMarkers( strm );
}


bool Well::Reader::getMarkers( od_istream& strm ) const
{
    double version = 0.0;
    if ( !rdHdr(strm,sKeyMarkers(),version) )
	return false;

    ascistream astrm( strm, false );
    
    IOPar iopar( astrm );
    if ( iopar.isEmpty() ) return false;

    const float stopz = wd.track().dahRange().stop;
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

	if ( wm->dah() > stopz )
	    delete wm;
	else
	    wd.markers().insertNew( wm );
    }

    return wd.markers().size();
}


bool Well::Reader::getD2T() const	{ return doGetD2T( false ); }
bool Well::Reader::getCSMdl() const	{ return doGetD2T( true ); }
bool Well::Reader::doGetD2T( bool csmdl ) const
{
    mGetInpStream( csmdl ? sExtCSMdl() : sExtD2T(), 0, return false );
    return doGetD2T( strm, csmdl );
}


bool Well::Reader::getD2T( od_istream& strm ) const
{ return doGetD2T(strm,false); }
bool Well::Reader::getCSMdl( od_istream& strm ) const
{ return doGetD2T(strm,true); }
bool Well::Reader::doGetD2T( od_istream& strm, bool csmdl ) const
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
    while ( strm.isOK() )
    {
	strm >> dah >> val;
	if ( !strm.isOK() ) break;
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
    mGetInpStream( sExtDispProps(), 0, return false );
    return getDispProps( strm );
}


bool Well::Reader::getDispProps( od_istream& strm ) const
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
