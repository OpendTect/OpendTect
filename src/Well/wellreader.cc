/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellreader.h"
#include "wellodreader.h"
#include "wellioprov.h"
#include "welltransl.h"

#include "ascstream.h"
#include "bufstringset.h"
#include "dbkey.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "iopar.h"
#include "ioobj.h"
#include "ioman.h"
#include "keystrs.h"
#include "od_istream.h"
#include "ptrman.h"
#include "separstr.h"
#include "survinfo.h"
#include "uistrings.h"
#include "wellman.h"
#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "welldisp.h"
#include "wellwriter.h"


const char* Well::odIO::sKeyWell()	{ return "Well"; }
const char* Well::odIO::sKeyTrack()	{ return "Track"; }
const char* Well::odIO::sKeyLog()	{ return "Well Log"; }
const char* Well::odIO::sKeyDefaults(){ return "Default Logs"; }
const char* Well::odIO::sKeyMarkers()	{ return "Well Markers"; }
const char* Well::odIO::sKeyD2T()	{ return "Depth2Time Model"; }
const char* Well::odIO::sKeyDispProps()	{ return "Display Properties"; }
const char* Well::odIO::sExtWell()	{ return ".well"; }
const char* Well::odIO::sExtTrack()	{ return ".track"; }
const char* Well::odIO::sExtLog()	{ return ".wll"; }
const char* Well::odIO::sExtMarkers()	{ return ".wlm"; }
const char* Well::odIO::sExtD2T()	{ return ".wlt"; }
const char* Well::odIO::sExtCSMdl()	{ return ".csmdl"; }
const char* Well::odIO::sExtDispProps()	{ return ".disp"; }
const char* Well::odIO::sExtDefaults()	{ return ".defs"; }
const char* Well::odIO::sExtWellTieSetup() { return ".tie"; }


// Well::ReadAccess

Well::ReadAccess::ReadAccess( Well::Data& wd )
    : wd_(wd)
{}


Well::ReadAccess::~ReadAccess()
{}


bool Well::ReadAccess::needsAdd( const char* lognm, bool needjustinfo ) const
{
    const Well::LogSet& logs = data().logs();
    const bool ispresent = (needjustinfo && logs.isLoaded(lognm)) ||
			  (!needjustinfo && logs.isLoaded(lognm));
    return !ispresent;
}


bool Well::ReadAccess::addToLogSet( Log* newlog, bool needjustinfo ) const
{
    if ( !newlog )
	return false;

    if ( !needjustinfo && !newlog->isEmpty() )
    {
	newlog->removeTopBottomUdfs();
	newlog->updateAfterValueChanges();
	if ( newlog->isEmpty() )
	    return false;
    }

    wd_.logs().add( newlog );
    return true;
}


bool Well::ReadAccess::updateDTModel( D2TModel* dtmodel, bool ischeckshot,
				      uiString& errmsg ) const
{
    uiString msg;
    if ( !dtmodel || !dtmodel->ensureValid(wd_,msg) )
    {
	delete dtmodel;
	errmsg = msg;
	return false;
    }

    if ( ischeckshot )
	wd_.setCheckShotModel( dtmodel );
    else
	wd_.setD2TModel( dtmodel );

    return true;
}


bool Well::ReadAccess::updateDTModel( D2TModel* dtmodel, bool iscs,
				      BufferString& errmsg ) const
{
    uiString msg;
    const bool ret = updateDTModel( new D2TModel(*dtmodel),
				    iscs, msg );
    msg.getFullString( &errmsg );
    return ret;
}


bool Well::ReadAccess::updateDTModel( D2TModel* dtmodel, const Track&, float,
					      bool iscs ) const
{
    if ( !dtmodel )
	return false;

    uiString dum;
    return updateDTModel( new D2TModel(*dtmodel), iscs,
			  dum );
}


void Well::ReadAccess::adjustTrackIfNecessary( bool frommarkers ) const
{
    Interval<float> newmdrg;
    if ( frommarkers )
    {
	const MarkerSet& markers = wd_.markers();
	if ( markers.isEmpty() )
	    return;

	newmdrg.set( mUdf(float), -mUdf(float) );
	for ( const auto* marker : markers )
	    newmdrg.include( marker->dah() );
    }
    else
    {
	const LogSet& logs = wd_.logs();
	if ( logs.isEmpty() )
	    return;

	newmdrg = logs.dahInterval();
    }

    if ( const_cast<Track&>(wd_.track()).extendIfNecessary(newmdrg) )
	wd_.trackchanged.trigger();
}


bool Well::ReadAccess::getD2TByName( const char* ) const
{
    return getD2T();
}


bool Well::ReadAccess::getD2TByID( const D2TID& ) const
{
    return getD2T();
}


bool Well::ReadAccess::getD2TInfo( BufferStringSet& ) const
{
    return false;
}


bool Well::ReadAccess::getCSMdlByName( const char* ) const
{
    return getCSMdl();
}


bool Well::ReadAccess::getCSMdlByID( const D2TID& ) const
{
    return getCSMdl();
}


bool Well::ReadAccess::getCSMdlInfo( BufferStringSet& ) const
{
    return false;
}


void Well::ReadAccess::getLogInfo( BufferStringSet& lognms ) const
{
    if ( getLogs(true) )
	data().logs().getNames( lognms );
}


uiString Well::ReadAccess::sCannotReadFileHeader() const
{
    return tr( "Cannot read file header" );
}


// Well::Reader

Well::Reader::Reader( const IOObj& ioobj, Data& wd )
{
    init( ioobj, wd );
}


Well::Reader::Reader( const MultiID& ky, Data& wd )
{
    IOObj* ioobj = IOM().get( ky );
    if ( !ioobj )
	errmsg_ = uiStrings::phrCannotFindDBEntry(
		    tr("for well ID %1 in data store" )).arg( ky );
    else
    {
	init( *ioobj, wd );
	delete ioobj;
    }
}


void Well::Reader::init( const IOObj& ioobj, Data& wd )
{
    if ( ioobj.group() != mTranslGroupName(Well) )
	errmsg_.appendPhrase( tr("%1 is not a Well, but a '%2'") )
	    .arg( ioobj.name() ).arg( ioobj.group() );
    else
    {
	ra_ = WDIOPF().getReadAccess( ioobj, wd, errmsg_ );
	if ( !ra_ )
	    errmsg_.appendPhrase( uiStrings::phrCannotCreate(
			tr("reader of type '%1'").arg( ioobj.translator() ) ) );
    }
}


Well::Reader::~Reader()
{
    delete ra_;
}


bool Well::Reader::get() const
{
    if ( !ra_ )
	return false;

    auto* wd = const_cast<Data*>( data() );
    if ( !wd )
	return false;

    wd->setD2TModel( nullptr );
    wd->setCheckShotModel( nullptr );

    if ( !getInfo() || !getTrack() )
	return false;

    if ( SI().zIsTime() )
    {
	if ( !getD2T() )
	    return false;

	getCSMdl();
    }

    getLogs( false );
    getMarkers();
    getDispProps();

    return true;
}


uiRetVal Well::Reader::readReqData( const LoadReqs& lreqs ) const
{
    uiRetVal uirv;
    const Data* wd = data();
    const bool isusable = isUsable() && wd;
    if ( !isusable )
	uirv.add( errMsg() );

    if ( isusable && lreqs.includes(Inf) && !getInfo() )
	uirv.add( errMsg() );

    if ( isusable && lreqs.includes(Trck) && !getTrack() )
	uirv.add( errMsg() );

    if ( isusable && lreqs.includes(CSMdl) && !getCSMdl() )
	uirv.add( errMsg() );

    if ( isusable && lreqs.includes(D2T) && !getD2T() )
	uirv.add( errMsg() );

    if ( isusable && lreqs.includes(Mrkrs) && !getMarkers() )
	uirv.add( errMsg() );

    bool logres = true;
    const LogSet& logs = wd->logs();
    const BufferStringSet& reqlognms = lreqs.logNames();
    const bool allowmissinglogs = lreqs.allowMissingLogs();
    BufferStringSet lognms;
    if ( isusable && lreqs.includes(Logs) )
    {
	if ( wd->loadState().includes(LogInfos) )
	{
	    BufferStringSet alllognms;
	    logs.getNames( alllognms );
	    if ( reqlognms.isEmpty() ) //All (not already loaded)
	    {
		for ( const auto* nm : alllognms )
		{
		    const char* lognm = nm->buf();
		    if ( !logs.isLoaded(lognm) )
		    {
			if ( (allowmissinglogs && alllognms.isPresent(lognm)) ||
			     !allowmissinglogs )
			    lognms.add( lognm );
		    }
		}
	    }
	    else
	    {
		for ( const auto* nm : reqlognms )
		{
		    const char* lognm = nm->buf();
		    if ( !logs.isLoaded(lognm) )
		    {
			if ( (allowmissinglogs && alllognms.isPresent(lognm)) ||
			     !allowmissinglogs )
			    lognms.add( lognm );
		    }
		}
	    }

	    if ( !lognms.isEmpty() )
		logres = getLogs( lognms );
	}
	else
	    logres = getLogs( false );
    }
    else if ( isusable && lreqs.includes(LogInfos) )
    {
	logres = getLogs( true );
	BufferStringSet alllognms;
	logs.getNames( alllognms );
	if ( !reqlognms.isEmpty() )
	{
	    for ( const auto* nm : reqlognms )
	    {
		const char* lognm = nm->buf();
		if ( !logs.isLoaded(lognm) )
		{
		    if ( (allowmissinglogs && alllognms.isPresent(lognm)) ||
			 !allowmissinglogs )
			lognms.add( lognm );
		}
	    }

	    if ( !lognms.isEmpty() )
		logres = getLogs( lognms );
	}
    }

    if ( !logres )
	uirv.add( errMsg() );

    if ( isusable && (lreqs.includes(DispProps2D) ||
		      lreqs.includes(DispProps3D)) )
    {
	if ( !getDispProps() )
	    uirv.add( errMsg() );
    }

    return uirv;
}


bool Well::Reader::getInfo() const
{
    return ra_ ? ra_->getInfo() : false;
}


bool Well::Reader::getTrack() const
{
    const bool trackloaded = ra_ && ra_->getTrack();
    if ( trackloaded )
	const_cast<Track&>( data()->track() ).updateDahRange();

    return trackloaded;
}


bool Well::Reader::getMarkers() const
{
    return ra_ ? ra_->getMarkers() : false;
}


bool Well::Reader::getD2T() const
{
    if ( data() && (!getInfo() || (data()->track().isEmpty() && !getTrack())) )
	return false;

    return ra_ ? ra_->getD2T() : false;
}


bool Well::Reader::getCSMdl() const
{
    if ( data() && data()->track().isEmpty() && (!getTrack() || !getInfo()) )
	return false;

    return ra_ ? ra_->getCSMdl() : false;
}


bool Well::Reader::getLogs( bool needjustinfo ) const
{
    return ra_ ? ra_->getLogs( needjustinfo ) && ra_->getDefLogs() : false;
}


bool Well::Reader::getLogs( const BufferStringSet& lognms ) const
{
    uiRetVal uirv;
    for ( const auto* lognm : lognms )
    {
	if ( !getLog(lognm->buf()) )
	{
	    uirv.add( errmsg_ );
	    errmsg_.setEmpty();
	}
    }

    errmsg_.setEmpty();
    if ( !getDefLogs() )
	uirv.add( errmsg_ );

    if ( uirv.isError() )
	errmsg_ = uirv.messages().cat();

    return uirv.isOK();
}


bool Well::Reader::getLog( const char* nm ) const
{
    return ra_ ? ra_->getLog( nm ) : false;
}


bool Well::Reader::getLogByID( const LogID& id ) const
{
    return ra_ ? ra_->getLogByID( id ) : false;
}


void Well::Reader::getLogInfo( BufferStringSet& lognms ) const
{
    getLogNames( lognms );
}


void Well::Reader::getLogNames( BufferStringSet& lognms ) const
{
    if ( getLogs(true) && data() )
	data()->logs().getNames( lognms );
}


bool Well::Reader::getDefLogs() const
{
    return ra_ ? ra_->getDefLogs() : false;
}


bool Well::Reader::getDispProps() const
{
    return ra_ ? ra_->getDispProps() : false;
}


Well::Data* Well::Reader::data()
{
    return ra_ ? &ra_->data() : nullptr;
}


const Well::Data* Well::Reader::data() const
{
    return getNonConst(this)->data();
}


bool Well::Reader::getMapLocation( Coord& coord ) const
{
    if ( !data() || !getInfo() )
	return false;

    const Data& wd = *data();
    coord = wd.info().surfacecoord_;
    return true;
}


bool Well::Reader::canReadInParallel( const MultiID& ky )
{
    PtrMan<IOObj> ioobj = IOM().get( ky );
    if ( !ioobj )
	return false;

    RefMan<Data> wd = new Data;
    Reader rdr( *ioobj, *wd );
    if ( !rdr.isUsable() )
	return false;

    return rdr.ra_ ? rdr.ra_->canReadInParallel() : false;
}


// Well::odIO

Well::odIO::odIO( const char* f, uiString& errmsg )
    : basenm_(f)
    , errmsg_(errmsg)
{
    FilePath fp( basenm_ );
    fp.setExtension( nullptr, true );
    const_cast<BufferString&>(basenm_) = fp.fullPath();
}


Well::odIO::~odIO()
{}


const char* Well::odIO::getFileName( const char* ext, int nr ) const
{
    return mkFileName( basenm_, ext, nr );
}


const char* Well::odIO::mkFileName( const char* bnm, const char* ext, int nr )
{
    mDeclStaticString( fnm );
    fnm = bnm;
    if ( nr )
	{ fnm += "^"; fnm += nr; }
    fnm += ext;
    return fnm;
}


const char* Well::odIO::getMainFileName( const IOObj& ioobj )
{
    return ioobj.fullUserExpr( true );
}


const char* Well::odIO::getMainFileName( const MultiID& mid )
{
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
	return nullptr;

    return getMainFileName( *ioobj );
}


bool Well::odIO::removeAll( const char* ext ) const
{
    for ( int idx=1; ; idx++ )
    {
	BufferString fnm( getFileName(ext,idx) );
	if ( !File::exists(fnm) )
	    break;
	else if ( !File::remove(fnm) )
	    { errmsg_ = uiStrings::phrCannotRemove( fnm ); return false; }
    }
    return true;
}


static const char* rdHdr( od_istream& strm, const char* fileky, double& ver )
{
    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType(fileky) )
    {
	BufferString msg( strm.fileName(), " has type '" );
	msg += astrm.fileType(); msg += "' whereas it should be '";
	msg += fileky; msg += "'";
	ErrMsg( msg );
	return nullptr;
    }

    ver = (double)astrm.majorVersion() +
	  ((double)astrm.minorVersion() / (double)10);
    mDeclStaticString( hdrln ); hdrln = astrm.headerStartLine();
    return hdrln.buf();
}


#define mGetInpStream(ext,nr,seterr,todo) \
    errmsg_.setEmpty(); \
    od_istream strm( getFileName(ext,nr) ); \
    if ( !strm.isOK() ) \
    { \
	if ( seterr ) \
	    setInpStrmOpenErrMsg( strm ); \
	todo; \
    }

// Well::odReader

Well::odReader::odReader( const char* f, Data& w, uiString& errmsg )
    : odIO(f,errmsg)
    , ReadAccess(w)
{
    FilePath fp( f );
    fp.setExtension( nullptr );
    wd_.info().setName( fp.fileName() );
}


Well::odReader::odReader( const IOObj& ioobj, Data& w, uiString& errmsg )
    : odIO(ioobj.fullUserExpr(true),errmsg)
    , ReadAccess(w)
{
    wd_.info().setName( ioobj.name() );
    wd_.setMultiID( ioobj.key() );
}


Well::odReader::~odReader()
{}


bool Well::odReader::getInfo() const
{
    mGetInpStream( sExtWell(), 0, true, return false )

    wd_.info().source_.set( getFileName(sExtWell()) );
    return getInfo( strm );
}


void Well::odReader::setInpStrmOpenErrMsg( od_istream& strm ) const
{
    strm.addErrMsgTo( errmsg_ );
}


void Well::odReader::setStrmOperErrMsg( od_istream& strm,
                                        const uiString& oper ) const
{
    errmsg_ = tr("%1 ['%2']").arg( oper ).arg( strm.fileName() );
    strm.addErrMsgTo( errmsg_ );
}


#define mErrStrmOper(oper,todo) { setStrmOperErrMsg( strm, oper ); todo; }
#define mErrRetStrmOper(oper) mErrStrmOper(oper,return false)


static const char* sKeyOldreplvel()	{ return "Replacement velocity"; }
static const char* sKeyOldgroundelev()	{ return "Ground Level elevation"; }

bool Well::odReader::getInfo( od_istream& strm ) const
{
    double version = 0.0;
    const char* hdrln = rdHdr( strm, sKeyWell(), version );
    if ( !hdrln )
	mErrRetStrmOper( sCannotReadFileHeader() )

    bool badhdr = *hdrln != 'd';
    if ( !badhdr )
    {
	if ( *(hdrln+1) == 'G' )
	    return getOldTimeWell(strm);
	else if ( *(hdrln+1) != 'T' )
	    badhdr = true;
    }
    if ( badhdr )
	mErrRetStrmOper( sCannotReadFileHeader() )

    ascistream astrm( strm, false );
    while ( !atEndOfSection(astrm.next()) )
    {
	if ( astrm.hasKeyword(Info::sKeyUwid()) )
	    wd_.info().uwid_ = astrm.value();
	else if ( astrm.hasKeyword(Info::sKeyOper()) )
	    wd_.info().oper_ = astrm.value();
	else if ( astrm.hasKeyword(Info::sKeyField()) )
	    wd_.info().field_ = astrm.value();
	else if ( astrm.hasKeyword(Info::sKeyCounty()) )
	    wd_.info().county_ = astrm.value();
	else if ( astrm.hasKeyword(Info::sKeyState()) )
	    wd_.info().state_ = astrm.value();
	else if ( astrm.hasKeyword(Info::sKeyProvince()) )
	    wd_.info().province_ = astrm.value();
	else if ( astrm.hasKeyword(Info::sKeyCountry()) )
	    wd_.info().country_ = astrm.value();
	else if ( astrm.hasKeyword(Info::sKeyWellType()) )
	    wd_.info().welltype_ = (OD::WellType) astrm.getIValue();
	else if ( astrm.hasKeyword(Info::sKeyCoord()) )
	    wd_.info().surfacecoord_.fromString( astrm.value() );
	else if ( astrm.hasKeyword(sKeyOldreplvel()) ||
		  astrm.hasKeyword(Info::sKeyReplVel()) )
	    wd_.info().replvel_ = astrm.getFValue();
	else if ( astrm.hasKeyword(sKeyOldgroundelev()) ||
		  astrm.hasKeyword(Info::sKeyGroundElev()) )
	    wd_.info().groundelev_ = astrm.getFValue();
    }

    Coord surfcoord = wd_.info().surfacecoord_;
    if ( (mIsZero(surfcoord.x_,0.001) && mIsZero(surfcoord.x_,0.001))
         || (mIsUdf(surfcoord.x_) && mIsUdf(surfcoord.x_)) )
    {
	if ( wd_.track().isEmpty() && !getTrack(strm) )
	    return false;

	wd_.info().surfacecoord_ = wd_.track().pos( 0 );
    }

    return true;
}


bool Well::odReader::getOldTimeWell( od_istream& strm ) const
{
    ascistream astrm( strm, false );
    astrm.next();
    while ( !atEndOfSection(astrm) ) astrm.next();

    // get track
    Coord3 c3, prevc, c0;
    float dah = 0;
    while ( strm.isOK() )
    {
        strm >> c3.x_ >> c3.y_ >> c3.z_;
	if ( !strm.isOK() || c3.distTo(c0) < 1 ) break;

	if ( !wd_.track().isEmpty() )
	    dah += (float) c3.distTo( prevc );
        wd_.track().addPoint( c3, (float) c3.z_, dah );
	prevc = c3;
    }
    if ( wd_.track().isEmpty() )
	mErrRetStrmOper( tr("No valid well track data found") )

    wd_.info().surfacecoord_ = wd_.track().pos(0);
    wd_.info().source_.set( FilePath(basenm_).fileName() );

    // create T2D
    D2TModel* d2t = new D2TModel( D2TModel::sKeyTimeWell() );
    for ( int idx=0; idx<wd_.track().size(); idx++ )
        d2t->add( wd_.track().dah(idx),(float) wd_.track().pos(idx).z_ );

    if ( !updateDTModel(d2t,false,errmsg_) )
	mErrRetStrmOper( tr("Time/Depth relation in file is not valid") )

    return true;
}


bool Well::odReader::getTrack( od_istream& strm ) const
{
    Coord3 c; float dah;
    wd_.track().setEmpty();
    while ( strm.isOK() )
    {
        strm >> c.x_ >> c.y_ >> c.z_ >> dah;
	if ( !strm.isOK() ) break;
        wd_.track().addPoint( c, (float) c.z_, dah );
    }
    if ( wd_.track().isEmpty() )
	mErrRetStrmOper( tr("No valid well track data found") )

    wd_.track().updateDahRange();

    return true;
}


bool Well::odReader::getTrack() const
{
    bool isold = false;
    mGetInpStream( sExtTrack(), 0, false, isold = true )
    if ( isold )
    {
	strm.open( getFileName(".well",0) );
	if ( !strm.isOK() )
	    mErrRetStrmOper( tr("No valid main well file found") )
    }

    ascistream astrm( strm );
    const double version = (double)astrm.majorVersion() +
			   ((double)astrm.minorVersion()/(double)10);
    if ( isold )
	{ IOPar dum; dum.getFrom( astrm ); }

    const bool isok = getTrack( strm );
    if ( SI().zInFeet() && version < 4.195 )
    {
	Track& welltrack = wd_.track();
	for ( int idx=0; idx<welltrack.size(); idx++ )
	{
	    Coord3 pos = welltrack.pos( idx );
            pos.z_ *= mToFeetFactorD;
            welltrack.setPoint( idx, pos, (float) pos.z_ );
	}
    }

    return isok;
}


void Well::odReader::getLogInfos( BufferStringSet& nms,
				  TypeSet<int>& idxs ) const
{
    for ( int idx=1;  ; idx++ )
    {
	mGetInpStream( sExtLog(), idx, false, break )

	double version = 0.0;
	if ( rdHdr(strm,sKeyLog(),version) )
	{
	    int bintyp = 0;
	    PtrMan<Log> log = rdLogHdr( strm, bintyp, idx-1 );
	    if ( nms.isPresent(log->name()) )
	    {
		BufferString msg( log->name() );
		msg += " already present in the list, won't be read";
		ErrMsg( msg );
	    }
	    else
	    {
		nms.add( log->name() );
		idxs += idx;
	    }
	}
    }
}


bool Well::odReader::getLog( const char* lognm ) const
{
    BufferStringSet nms;
    TypeSet<int> idxs;
    getLogInfos( nms, idxs );
    const int lognmidx = nms.indexOf( lognm );
    if ( lognmidx<0 )
    {
	errmsg_ = tr("Cannot find log '%1' in well '%2'").arg( lognm )
							 .arg( data().name() );
	return false;
    }

    const int logfileidx = idxs[lognmidx];
    mGetInpStream( sExtLog(), logfileidx, true, return false )
    return addLog( strm );
}


bool Well::odReader::getLogByID( const LogID& id ) const
{
    BufferStringSet nms;
    TypeSet<int> idxs;
    getLogInfos( nms, idxs );

    const int logidx = idxs.indexOf( id.asInt() );
    if ( logidx<0 )
	return false;

    const int logfileidx = id.asInt();
    mGetInpStream( sExtLog(), logfileidx, true, return false )
    return addLog( strm );
}


bool Well::odReader::getLogs( bool needjustinfo ) const
{
    bool rv = true;
    for ( int idx=1;  ; idx++ )
    {
	mGetInpStream( sExtLog(), idx, false, break )

	if ( !addLog(strm,needjustinfo) )
	{
	    setStrmOperErrMsg( strm, tr("read data") );
	    ErrMsg( errmsg_ ); errmsg_.setEmpty();
	    rv = false;
	    continue;
	}
    }

    if ( rv )
	adjustTrackIfNecessary();

    return rv;
}


Well::Log* Well::odReader::rdLogHdr( od_istream& strm, int& bintype, int idx )
{
    auto* newlog = new Log;
    ascistream astrm( strm, false );
    bool havehdrinfo = false;
    bintype = 0;
    while ( !atEndOfSection(astrm.next()) )
    {
	if ( astrm.hasKeyword(sKey::Name()) )
	    newlog->setName( astrm.value() );
	if ( astrm.hasKeyword(Log::sKeyMnemLbl()) )
	    newlog->setMnemonicLabel( astrm.value() );
	if ( astrm.hasKeyword(Log::sKeyUnitLbl()) )
	    newlog->setUnitMeasLabel( astrm.value() );
	if ( astrm.hasKeyword(Log::sKeyHdrInfo()) )
	    havehdrinfo = astrm.getYN();
	if ( astrm.hasKeyword(Log::sKeyStorage()) )
	{
	    const BufferString stortype = astrm.value();
	    bintype = stortype == "Ascii" ? 0
		    : (stortype == "Binary" ? 1
					    : (stortype == "Swapped" ? -1:-2));
	}
	if ( astrm.hasKeyword(Log::sKeyDahRange()) )
	{
	    newlog->addValue( astrm.getFValue(0), mUdf(float) );
	    newlog->addValue( astrm.getFValue(1), mUdf(float) );
	    newlog->dahRange().set( astrm.getFValue(0), astrm.getFValue(1) );
	}
	if ( astrm.hasKeyword(Log::sKeyLogRange()) )
	{
	    const Interval<float> valrg( astrm.getFValue(0),astrm.getFValue(1));
	    newlog->setValueRange( valrg );
	}
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


bool Well::odReader::addLog( od_istream& strm, bool needjustinfo ) const
{
    double version = 0.0;
    if ( !rdHdr(strm,sKeyLog(),version) )
	return false;

    int bintype = 0;
    Log* newlog = rdLogHdr( strm, bintype, wd_.logs().size() );
    if ( !newlog )
	mErrRetStrmOper( sCannotReadFileHeader() )

    if ( !needsAdd(newlog->name().buf(),needjustinfo) )
	return true;

    const Log* cnewlog = newlog;
    const bool udfranges = cnewlog->dahRange().isUdf() ||
			   cnewlog->valueRange().isUdf();
    if ( !needjustinfo || udfranges )
	readLogData( *newlog, strm, bintype );

    if ( wd_.track().isEmpty() )
	getTrack();

    const bool addedok = addToLogSet( newlog, needjustinfo );
    if ( addedok && udfranges )
    {
	const BufferStringSet lognms( newlog->name().buf() );
	Well::MGR().writeLogHeaders( wd_.multiID(), lognms );
    }

    if ( addedok && SI().zInFeet() && version<4.195 && !needjustinfo )
    {
	Log* wl = getNonConst( data().logs() ).getLog( newlog->name().buf() );
	for ( int idx=0; idx<wl->size(); idx++ )
	    wl->dahArr()[idx] = wl->dah(idx) * mToFeetFactorF;

	wl->updateDahRange();
    }

    if ( addedok )
	adjustTrackIfNecessary();

    return addedok;
}


void Well::odReader::readLogData( Log& wl, od_istream& strm, int bintype ) const
{
    wl.setEmpty();
    float v[2];
    while ( strm.isOK() )
    {
	if ( bintype )
	{
	    strm.getBin( v, 2 * sizeof(float) );
	    if ( (bintype > 0) != __islittle__ )
	    {
		SwapBytes( v, sizeof(float) );
		SwapBytes( v+1, sizeof(float) );
	    }
	}
	else
	    strm >> v[0] >> v[1];

	if ( !strm.isOK() )
	    break;

	wl.addValue( v[0], v[1] );
    }
}


bool Well::odReader::getDefLogs() const
{
    const BufferString deflogfnm = getFileName(sExtDefaults());
    if ( !File::exists(deflogfnm) )
	return true; // Not a reason for failure

    mGetInpStream( sExtDefaults(), 0, true, return false )
    return getDefLogs( strm );
}


bool Well::odReader::getDefLogs( od_istream& strm ) const
{
    double version = 0.0;
    if ( !rdHdr(strm,sKeyDefaults(),version) )
	mErrRetStrmOper( sCannotReadFileHeader() )

    ascistream astrm( strm, false );
    IOPar iop;
    iop.getFrom( astrm );
    wd_.logs().defaultLogUsePar( iop );
    return true;
}


bool Well::odReader::getMarkers() const
{
    mGetInpStream( sExtMarkers(), 0, true, return false )
    return getMarkers( strm );
}


bool Well::odReader::getMarkers( od_istream& strm ) const
{
    double version = 0.0;
    if ( !rdHdr(strm,sKeyMarkers(),version) )
	mErrRetStrmOper( sCannotReadFileHeader() )

    ascistream astrm( strm, false );

    IOPar iopar( astrm );
    if ( iopar.isEmpty() )
	return true;

    if ( wd_.track().isEmpty() )
    {
	if ( !getTrack() )
	    return false;
    }

    const Interval<float> trackdahrg = wd_.track().dahRange();
    const bool havetrack = !wd_.track().isEmpty();
    wd_.markers().erase();
    BufferString bs;
    for ( int idx=1;  ; idx++ )
    {
	BufferString basekey; basekey += idx;
	BufferString key = IOPar::compKey( basekey, sKey::Name() );
	if ( !iopar.get(key,bs) ) break;

	auto* wm = new Marker( bs );

	key = IOPar::compKey( basekey, Marker::sKeyDah() );
	if ( !iopar.get(key,bs) )
	{
	    delete wm;
	    continue;
	}

	const float val = bs.toFloat();
	wm->setDah( (SI().zInFeet() && version<4.195) ? (val*mToFeetFactorF)
						      : val );
	key = IOPar::compKey( basekey, sKey::StratRef() );

	Strat::LevelID lvlid;
	iopar.get( key, lvlid );
	wm->setLevelID( lvlid );

	key = IOPar::compKey( basekey, sKey::Color() );
	if ( iopar.get(key,bs) )
	{
	    OD::Color col( wm->color() );
	    col.use( bs.buf() );
	    wm->setColor( col );
	}

	if ( !trackdahrg.includes(wm->dah(),false) && havetrack )
	    delete wm;
	else
	    wd_.markers().insertNew( wm );
    }

    adjustTrackIfNecessary( true );

    return true;
}


bool Well::odReader::getD2T() const	{ return doGetD2T( false ); }
bool Well::odReader::getCSMdl() const	{ return doGetD2T( true ); }
bool Well::odReader::doGetD2T( bool csmdl ) const
{
    mGetInpStream( csmdl ? sExtCSMdl() : sExtD2T(), 0, false, )
    return File::exists(strm.fileName()) ? strm.isOK() && doGetD2T(strm,csmdl)
					 : true;
}


bool Well::odReader::getD2T( od_istream& strm ) const
{ return doGetD2T(strm,false); }
bool Well::odReader::getCSMdl( od_istream& strm ) const
{ return doGetD2T(strm,true); }
bool Well::odReader::doGetD2T( od_istream& strm, bool csmdl ) const
{
    double version = 0.0;
    if ( !rdHdr(strm,sKeyD2T(),version) )
	mErrRetStrmOper( sCannotReadFileHeader() )

    ascistream astrm( strm, false );
    auto* d2t = new D2TModel;
    while ( !atEndOfSection(astrm.next()) )
    {
	if ( astrm.hasKeyword(sKey::Name()) )
	    d2t->setName( astrm.value() );
	else if ( astrm.hasKeyword(sKey::Desc()) )
	    d2t->desc = astrm.value();
	else if ( astrm.hasKeyword(D2TModel::sKeyDataSrc()) )
	    d2t->datasource = astrm.value();
    }

    float dah, val;
    while ( strm.isOK() )
    {
	strm >> dah >> val;
	if ( !strm.isOK() )
	    break;
	d2t->add( dah, val );
    }
    d2t->updateDahRange();
    if ( !updateDTModel(d2t,csmdl,errmsg_) )
	mErrRetStrmOper( tr("Time/Depth relation in file is not valid") )

    return true;
}


bool Well::odReader::getDispProps() const
{
    mGetInpStream( sExtDispProps(), 0, true, return false )
    return getDispProps( strm );
}


bool Well::odReader::getDispProps( od_istream& strm ) const
{
    double version = 0.;
    if ( !rdHdr(strm,sKeyDispProps(),version) )
	mErrRetStrmOper( sCannotReadFileHeader() )

    ascistream astrm( strm, false );
    IOPar iop; iop.getFrom( astrm );
    wd_.displayProperties( true ).usePar( iop );
    wd_.displayProperties( false ).usePar( iop );
    return true;
}


int Well::odReader::getStorageType( const char* fnm )
{
    if ( !File::exists(fnm) )
	return mUdf(int);

    od_istream strm( fnm );
    if ( !strm.isOK() )
	return mUdf(int);

    int bintype = mUdf(int);
    double version = 0.0;
    if ( !rdHdr(strm,sKeyLog(),version) )
	return mUdf(int);

    PtrMan<Log> log = rdLogHdr( strm, bintype, mUdf(int) );
    return log ? bintype : mUdf(int);
}


// MultiWellReader
MultiWellReader::MultiWellReader( const TypeSet<MultiID>& ids,
				  RefObjectSet<Well::Data>& wds,
				  const Well::LoadReqs reqs )
    : ParallelTask("Reading well data")
    , keys_(*new DBKeySet(ids))
    , wds_(wds)
    , nrwells_(ids.size())
    , reqs_(reqs)
{
    init();
}


MultiWellReader::MultiWellReader( const DBKeySet& keys,
				  RefObjectSet<Well::Data>& wds,
				  const Well::LoadReqs reqs )
    : ParallelTask("Reading well data")
    , keys_(*new DBKeySet(keys))
    , wds_(wds)
    , nrwells_(keys.size())
    , reqs_(reqs)
{
    init();
}


MultiWellReader::~MultiWellReader()
{
    delete chgr_;
    delete &keys_;
}


void MultiWellReader::init()
{
    msg_ = tr("Reading well data");
    allowMissingLogs( true );
    allsamesurvey_ = true;
    setMaxNrThreads();
    if ( keys_.size() > 1 )
	return;

    const DBKey& firstdbky = keys_.first();
    if ( firstdbky.isInCurrentSurvey() )
    {
	for ( int idx=1; idx<keys_.size(); idx++ )
	{
	    if ( !keys_.get(idx).isInCurrentSurvey() )
	    {
		allsamesurvey_ = false;
		return;
	    }
	}
    }
    else
    {
	const SurveyDiskLocation& sdl = firstdbky.surveyDiskLocation();
	for ( int idx=1; idx<keys_.size(); idx++ )
	{
	    const DBKey& dbky = keys_.get( idx );
	    if ( dbky.isInCurrentSurvey() ||
		 dbky.surveyDiskLocation() != sdl )
	    {
		allsamesurvey_ = false;
		return;
	    }
	}
    }
}


void MultiWellReader::setMaxNrThreads()
{
    maxnrthreads_ = mUdf(int); //auto (from base class)
    if ( !allsamesurvey_ )
    {
	maxnrthreads_ = 1;
	return;
    }

    for ( const auto* dbky : keys_ )
    {
	if ( !Well::Reader::canReadInParallel(*dbky) )
	{
	    maxnrthreads_ = 1;
	    return;
	}
    }
}


MultiWellReader& MultiWellReader::setReqs( const Well::LoadReqs& lreqs )
{
    reqs_ = lreqs;
    return *this;
}


MultiWellReader& MultiWellReader::forceRead( bool yn )
{
    forceread_ = yn;
    return *this;
}


MultiWellReader& MultiWellReader::allowMissingLogs( bool yn )
{
    reqs_.allowMissingLogs( yn );
    return *this;
}


uiString MultiWellReader::uiNrDoneText() const
{
    return tr("Wells read");
}


od_int64 MultiWellReader::nrIterations() const
{
    return nrwells_;
}


int MultiWellReader::maxNrThreads() const
{
    return allsamesurvey_ && mIsUdf(maxnrthreads_)
		? ParallelTask::maxNrThreads() : maxnrthreads_;
}


uiRetVal MultiWellReader::allMessages() const
{
    uiRetVal uirv( msg_ );
    uirv.add( uirv_ );
    return uirv;
}


bool MultiWellReader::hasFails() const
{
    return nrsuccess_ < totalNr();
}


RefMan<Well::Data> MultiWellReader::getWD( const DBKey& dbky,
					   bool islocal ) const
{
    for ( int idx=0; idx<wds_.size(); idx++ )
    {
	RefMan<Well::Data> wd = wds_.get( idx );
	if ( !wd )
	    continue;

	const MultiID& wid = wd->multiID();
	if ( wid == dbky )
	    return wd;
    }

    RefMan<Well::Data> wd;
    if ( islocal )
    {
	if ( Well::MGR().isLoaded(dbky) )
	{
	    const Well::LoadReqs lreqs = Well::MGR().loadState( dbky );
	    wd = Well::MGR().get( dbky, lreqs );
	}
    }
    else
    {
	wd = new Well::Data;
	wd->setMultiID( dbky );
    }

    return wd;
}


bool MultiWellReader::doPrepare( int nrthreads )
{
    msg_ = tr("Reading well data");
    uirv_.setOK();
    resetNrDone(); //Required ?
    nrsuccess_ = 0;

    if ( !allsamesurvey_ && nrthreads > 1 )
    {
	msg_ = tr("Cannot read sets of wells from multiple"
		  " surveys in parallel");
	return false;
    }

    islocal_.setEmpty();
    for ( const auto* dbky : keys_ )
    {
	const bool islocal = dbky->isInCurrentSurvey();
	islocal_ += islocal;
	RefMan<Well::Data> wd = getWD( *dbky, islocal );
	wds_.addIfNew( wd.ptr() );
    }

    deleteAndNullPtr( chgr_ );
    if ( allsamesurvey_ && !islocal_.isEmpty() && !islocal_.first() )
	chgr_ = new SurveyChanger( keys_.first().surveyDiskLocation() );

    return true;
}


bool MultiWellReader::doWork( od_int64 start, od_int64 stop, int threadidx )
{
    PtrMan<SurveyChanger> chgr;
    for ( od_int64 idx=start; idx<=stop; idx++, addToNrDone(1) )
    {
	const DBKey& dbky = keys_[sCast(int,idx)];
	const bool islocal = islocal_[sCast(int,idx)];
	if ( !chgr_ )
	{
	    if ( islocal && chgr )
		chgr = nullptr;
	    else if ( !islocal )
	    {
		const SurveyDiskLocation& sdl = dbky.surveyDiskLocation();
		if ( (chgr && chgr->changedToSurvey() != sdl )
			|| !chgr )
		    chgr = new SurveyChanger( sdl );
	    }
	}

	RefMan<Well::Data> wd = getWD( dbky, islocal );
	uiRetVal uirv;
	if ( islocal )
	{
	    if ( forceread_ )
	    {
		if ( Well::MGR().reload(dbky,reqs_) )
		    wd = Well::MGR().get( dbky, reqs_ );
	    }
	    else
		wd = Well::MGR().get( dbky, reqs_ );

	    if ( wd )
		wds_.addIfNew( wd.ptr() );
	    else
		uirv.add( Well::MGR().errMsg() );
	}
	else
	{
	    if ( wd )
	    {
		Well::Reader rdr( dbky, *wd );
		const uiRetVal rv = rdr.readReqData( reqs_ );
		if ( rv.isError() )
		    uirv.add( rv );
	    }
	}

	if ( !wd || uirv.isError() )
	{
	    if ( uirv.isOK() )
	    {
		const BufferString dbkystr = dbky.toString( !islocal );
		uirv.add( tr("Unspecified error loading the well data for"
			     "MultiID %1").arg(dbkystr) );
	    }
	    else
		uirv_.add( uirv );
	}
	else
	    nrsuccess_++;
    }

    return true;
}


bool MultiWellReader::doFinish( bool success )
{
    deleteAndNullPtr( chgr_ );
    if ( nrsuccess_ < 1 )
    {
	msg_ = tr("Failed to read any well data.");
	success = false;
    }
    else if ( hasFails() )
	msg_ = tr("Failed to read some of the well data.");

    return success;
}
