/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Aug 2003
-*/


#include "wellreader.h"
#include "wellodreader.h"
#include "wellioprov.h"
#include "welltransl.h"

#include "ascstream.h"
#include "bufstringset.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "iopar.h"
#include "ioobj.h"
#include "keystrs.h"
#include "ptrman.h"
#include "separstr.h"
#include "staticstring.h"
#include "survinfo.h"
#include "welldata.h"
#include "wellinfo.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "welldisp.h"
#include "wellwriter.h"
#include "od_istream.h"
#include "uistrings.h"
#include "odversion.h"


const char* Well::odIO::sKeyWell()	{ return "Well"; }
const char* Well::odIO::sKeyTrack()	{ return "Track"; }
const char* Well::odIO::sKeyLog()	{ return "Well Log"; }
const char* Well::odIO::sKeyMarkers()	{ return "Well Markers"; }
const char* Well::odIO::sKeyD2T()	{ return "Depth2Time Model"; }
const char* Well::odIO::sKeyDispProps() { return "Display Properties"; }
const char* Well::odIO::sExtWell()	{ return ".well"; }
const char* Well::odIO::sExtLog()	{ return ".wll"; }
const char* Well::odIO::sExtMarkers()	{ return ".wlm"; }
const char* Well::odIO::sExtD2T()	{ return ".wlt"; }
const char* Well::odIO::sExtCSMdl()	{ return ".csmdl"; }
const char* Well::odIO::sExtDispProps() { return ".disp"; }
const char* Well::odIO::sExtWellTieSetup() { return ".tie"; }



bool Well::ReadAccess::addToLogSet( Well::Log* newlog, bool needjustinfo ) const
{
    if ( !newlog )
	return false;

    if ( !needjustinfo && newlog->isEmpty() )
    {
	newlog->removeTopBottomUdfs();
	if ( newlog->isEmpty() )
	    return false;
    }

    wd_.logs().add( newlog );
    return true;
}


bool Well::ReadAccess::updateDTModel( D2TModel& dtmodel, bool ischeckshot,
				      uiString& errmsg ) const

{
    uiString msg;
    if ( !dtmodel.ensureValid(wd_,msg) )
    {
	errmsg = msg;
	return false;
    }

    if ( ischeckshot )
	wd_.checkShotModel() = dtmodel;
    else
	wd_.d2TModel() = dtmodel;

    return true;
}


bool Well::ReadAccess::getAll( bool stoponerr ) const
{
    bool haveerr = !getTrack() || !getInfo();
    if ( stoponerr && haveerr )
	return false;

    if ( SI().zIsTime() )
    {
	haveerr = getD2T() || haveerr;
	if ( stoponerr && haveerr )
	    return false;
	getCSMdl();
    }

    getLogs();
    getMarkers();
    getDispProps();

    return haveerr;
}


/* When a well is stored, it will be stored in the surveyDefZStorageUnit()
   We need to make sure that the depth will end up in the object as dictated
   by SI().zInFeet().
   From 7.0, we can encounter wells from other surveys (shared wells), so we
   have to be prepared to convert to or from meters to feet. */

float Well::ReadAccess::getZFac( const IOPar& iop )
{
    BufferString depthunstr;
    iop.get( sKey::DepthUnit(), depthunstr );

    const bool needinfeet = SI().zInFeet();
    const bool storedinfeet = depthunstr.isEmpty() ? needinfeet
					: depthunstr.startsWith( "F" );
    if ( storedinfeet == needinfeet )
	return 1.0f;

    return storedinfeet ? mFromFeetFactorF : mToFeetFactorF;
}



Well::Reader::Reader( const IOObj& ioobj, Well::Data& wd )
    : ra_(0)
{
    init( ioobj, wd );
}


Well::Reader::Reader( const DBKey& ky, Well::Data& wd )
    : ra_(0)
{
    IOObj* ioobj = getIOObj( ky );
    if ( !ioobj )
	errmsg_ = uiStrings::phrCannotFindDBEntry(
				tr("for well ID %1 in data store" )).arg( ky );
    else
    {
	init( *ioobj, wd );
	delete ioobj;
    }
}


void Well::Reader::init( const IOObj& ioobj, Well::Data& wd )
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
    getInfo();
}


Well::Reader::~Reader()
{
    delete ra_;
}


#define mImplWRFn(rettyp,fnnm,typ,arg,udf) \
rettyp Well::Reader::fnnm( typ arg ) const \
{ return ra_ ? ra_->fnnm(arg) : udf; }
#define mImplSimpleWRFn(fnnm) \
bool Well::Reader::fnnm() const { return ra_ ? ra_->fnnm() : false; }

mImplSimpleWRFn(getInfo)
mImplSimpleWRFn(getMarkers)
mImplSimpleWRFn(getDispProps)
mImplWRFn(bool,getLogs,bool,needjustinfo,false)


bool Well::Reader::getTrack() const
{
    const bool isok = ra_ ? ra_->getTrack() : false;
    if ( isok )
    {
	Well::Data* wd = const_cast<Well::Data*>( data() );
	wd->track().setName( wd->name() );
    }
    return isok;
}


bool Well::Reader::getAll() const
{
    if ( !ra_ )
	return false;

    Well::Data* wd = const_cast<Well::Data*>( data() );
    if ( !wd )
	return false;

    return ra_->getAll( false );
}


bool Well::Reader::getD2T() const
{
    if ( !getTrack() || !getInfo() )
	return false;

    return ra_ ? ra_->getD2T() : false;
}


bool Well::Reader::getCSMdl() const
{
    if ( !getTrack() || !getInfo() )
	return false;

    return ra_ ? ra_->getCSMdl() : false;
}


mImplWRFn(bool,getLog,const char*,lognm,false)
void Well::Reader::getLogNames( BufferStringSet& lognms ) const
{ if ( ra_ ) ra_->getLogNames( lognms ); }
void Well::Reader::getLogInfo( ObjectSet<IOPar>& iops ) const
{ if ( ra_ ) ra_->getLogInfo( iops ); }
Well::Data* Well::Reader::data()
{ return ra_ ? &ra_->data() : 0; }


bool Well::Reader::getMapLocation( Coord& coord ) const
{
    if ( !data() || !getInfo() )
	return false;

    const Well::Data& wd = *data();
    coord = wd.info().surfaceCoord();
    return true;
}


Well::odIO::odIO( const char* f, uiString& e )
    : basenm_(f)
    , errmsg_(e)
{
    File::Path fp( basenm_ );
    fp.setExtension( 0, true );
    const_cast<BufferString&>(basenm_) = fp.fullPath();
}


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
    mDeclStaticString( ret );
    ret = ioobj.mainFileName();
    return ret.buf();
}


const char* Well::odIO::getMainFileName( const DBKey& dbky )
{
    PtrMan<IOObj> ioobj = getIOObj( dbky );
    return ioobj ? getMainFileName(*ioobj) : 0;
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


static bool rdHdr( od_istream& strm, const char* fileky )
{
    ascistream astrm( strm, true );
    if ( astrm.isOfFileType(fileky) )
	return true;

    BufferString msg( strm.fileName(), " has type '" );
    msg += astrm.fileType(); msg += "' whereas it should be '";
    msg += fileky; msg += "'";
    ErrMsg( msg );
    return false;
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


Well::odReader::odReader( const char* f, Well::Data& w, uiString& e )
    : Well::odIO(f,e)
    , Well::ReadAccess(w)
{
    File::Path fp( f );
    fp.setExtension( 0 );
    wd_.info().setName( fp.fileName() );
}


Well::odReader::odReader( const IOObj& ioobj, Well::Data& w, uiString& e )
    : Well::odIO(ioobj.mainFileName(),e)
    , Well::ReadAccess(w)
{
    wd_.info().setName( ioobj.name() );
}


bool Well::odReader::getInfo() const
{
    mGetInpStream( sExtWell(), 0, true, return false );

    wd_.info().setDataSource( getFileName(sExtWell()) );
    return getInfo( strm );
}


void Well::odReader::setInpStrmOpenErrMsg( od_istream& strm ) const
{
    errmsg_ = uiStrings::phrCannotOpenForRead( strm.fileName() );
    strm.addErrMsgTo( errmsg_ );
}


void Well::odReader::setStrmOperErrMsg( od_istream& strm,
					const uiString& oper ) const
{
    errmsg_ = toUiString("%1 ['%2']").arg( oper ).arg( strm.fileName() );
    strm.addErrMsgTo( errmsg_ );
}


#define mErrStrmOper(oper,todo) { setStrmOperErrMsg( strm, oper ); todo; }
#define mErrRetStrmOper(oper) mErrStrmOper(oper,return false)
#define mGetZFac(iop) const float zfac = getZFac( iop )


uiString Well::odReader::sCannotReadFileHeader() const
{
    return tr( "Cannot read file header" );
}


bool Well::odReader::getInfo( od_istream& strm ) const
{
    if ( !rdHdr(strm,sKeyWell()) )
	mErrRetStrmOper( sCannotReadFileHeader() )

    IOPar iop;
    ascistream astrm( strm, false );
    iop.getFrom( astrm );

    ChangeNotifyBlocker nb( wd_.info() );
    wd_.info().usePar( iop );

    const Coord surfcoord = wd_.info().surfaceCoord();
    if ( (mIsZero(surfcoord.x_,0.001) && mIsZero(surfcoord.x_,0.001))
	    || (mIsUdf(surfcoord.x_) && mIsUdf(surfcoord.x_)) )
    {
	mGetZFac( iop );
	if ( wd_.track().isEmpty() && !gtTrack(strm,zfac) )
	    return false;

	wd_.info().setSurfaceCoord( wd_.track().firstPos().getXY() );
    }

    return true;
}


bool Well::odReader::gtTrack( od_istream& strm, float zfac ) const
{
    Coord3 c, c0; float dah;
    wd_.track().setEmpty();

    ChangeNotifyBlocker nb( wd_.track() );
    while ( strm.isOK() )
    {
	strm >> c.x_ >> c.y_ >> c.z_ >> dah;
	if ( !strm.isOK() || c.distTo<float>(c0) < 1 )
	    break;
	wd_.track().addPoint( c.getXY(), (float)c.z_*zfac, dah*zfac );
    }
    if ( wd_.track().isEmpty() )
	mErrRetStrmOper( tr("No valid well track data found") )

    return true;
}


bool Well::odReader::getTrack() const
{
    od_istream strm( getFileName(sExtWell(),0) );
    if ( !strm.isOK() )
	mErrRetStrmOper( tr("No valid main well file found") )

    if ( !rdHdr(strm,sKeyWell()) )
	mErrRetStrmOper( sCannotReadFileHeader() )

    ascistream astrm( strm, false );
    IOPar iop; iop.getFrom( astrm );
    mGetZFac( iop );

    return gtTrack( strm, zfac );
}


void Well::odReader::adjustTrackIfNecessary( bool frommarkers ) const
{
    Interval<float> newmdrg;
    if ( frommarkers )
    {
	const MarkerSet& markers = wd_.markers();
	if ( markers.isEmpty() )
	    return;

	newmdrg.set( mUdf(float), -mUdf(float) );
	MarkerSetIter iter( markers );
	while ( iter.next() )
	    newmdrg.include( iter.getDah() );
    }
    else
    {
	const LogSet& logs = wd_.logs();
	if ( logs.isEmpty() )
	    return;

	newmdrg = logs.dahInterval();
    }

    const_cast<Track&>(wd_.track()).extendIfNecessary( newmdrg );
}


void Well::odReader::getLogNames( BufferStringSet& nms ) const
{
    TypeSet<int> idxs;
    getLogNames( nms, idxs );
}


void Well::odReader::getLogNames( BufferStringSet& nms,
				  TypeSet<int>& idxs ) const
{
    IOPar dumiop;
    for ( int idx=1;  ; idx++ )
    {
	mGetInpStream( sExtLog(), idx, false, break );
	if ( rdHdr(strm,sKeyLog()) )
	{
	    int bintyp = 0;
	    RefMan<Well::Log> log = rdLogHdr( strm, bintyp, idx-1, dumiop );
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


void Well::odReader::getLogInfo( ObjectSet<IOPar>& iops ) const
{
    IOPar dumiop;
    for ( int idx=1;  ; idx++ )
    {
	mGetInpStream( sExtLog(), idx, false, break );

	if ( rdHdr(strm,sKeyLog()) )
	{
	    int bintyp = 0;
	    RefMan<Well::Log> log = rdLogHdr( strm, bintyp, idx-1, dumiop );
	    IOPar* iop = new IOPar( log->pars() );
	    iop->set( sKey::Name(), log->name() );
	    iop->set( sKey::Unit(), log->unitMeasLabel() );
	    iops += iop;
	}
    }
}


bool Well::odReader::getLog( const char* lognm ) const
{
    BufferStringSet nms;
    TypeSet<int> idxs;
    getLogNames( nms, idxs );
    const int lognmidx = nms.indexOf( lognm );
    if ( lognmidx < 0 )
	return false;

    const int logfileidx = idxs[lognmidx];
    mGetInpStream( sExtLog(), logfileidx, true, return false );
    return addLog( strm );
}


bool Well::odReader::getLogs( bool needjustinfo ) const
{
    bool rv = true;
    wd_.logs().setEmpty();
    for ( int idx=1;  ; idx++ )
    {
	mGetInpStream( sExtLog(), idx, false, break );
	if ( !addLog(strm,needjustinfo) )
	{
	    setStrmOperErrMsg( strm, tr("read data") );
	    ErrMsg( errmsg_ ); errmsg_.setEmpty();
	    rv = false;
	}
    }

    if ( rv )
	adjustTrackIfNecessary();

    return rv;
}


Well::Log* Well::odReader::rdLogHdr( od_istream& strm, int& bintype, int ilog,
				     IOPar& iop )
{
    Well::Log* newlog = new Well::Log;
    ascistream astrm( strm, false );
    iop.getFrom( astrm );

    bintype = 0;
    for ( int idx=0; idx<iop.size(); idx++ )
    {
	const FixedString ky = iop.getKey( idx );
	const FixedString val = iop.getValue( idx );
	if ( ky == sKey::Name() )
	    newlog->setName( val );
	if ( ky == Well::Log::sKeyMnemLbl() )
	    newlog->setMnemLabel( val );
	if ( ky == Well::Log::sKeyUnitLbl() )
	    newlog->setUnitMeasLabel( val );
	if ( ky == Well::Log::sKeyDahRange() )
	{
	    newlog->addValue( astrm.getFValue(0), mUdf(float) );
	    newlog->addValue( astrm.getFValue(1), mUdf(float) );
	    newlog->dahRange().set( astrm.getFValue(0), astrm.getFValue(1) );
	}
	if ( astrm.hasKeyword(Log::sKeyLogRange()) )
	    newlog->valueRange().set( astrm.getFValue(0), astrm.getFValue(1) );

	if ( ky == Well::Log::sKeyStorage() )
	    bintype = val[0] == 'B' ? 1
		    : val[0] == 'S' ? -1 : 0;
    }

    if ( newlog->name().isEmpty() )
    {
	BufferString nm( "[" ); nm += ilog+1; nm += "]";
	newlog->setName( nm );
    }

    if ( iop.isTrue(Well::Log::sKeyHdrInfo()) )
	newlog->pars().getFrom( astrm );

    return newlog;
}


bool Well::odReader::addLog( od_istream& strm, bool needjustinfo ) const
{
    if ( !rdHdr(strm,sKeyLog()) )
	return false;

    int bintype = 0;
    IOPar logiop;
    Well::Log* newlog = rdLogHdr( strm, bintype, wd_.logs().size(), logiop );
    if ( !newlog )
	mErrRetStrmOper( sCannotReadFileHeader() )

    if ( !needjustinfo )
	newlog->removeTopBottomUdfs();

    if ( newlog->isEmpty() )
    {
	readLogData( *newlog, strm, bintype );
	TypeSet<float> dahvals, zvals;
	newlog->getData( dahvals, zvals );

	if ( wd_.track().isEmpty() )
	    getTrack();
	if ( !wd_.track().isEmpty() )
	{
	    const Interval<float> trackdahrg = wd_.track().dahRange();
	    for ( int idx=0; idx<dahvals.size(); idx++ )
	    {
		if ( !trackdahrg.includes(dahvals[idx],false) )
		{
		    dahvals.removeSingle( idx );
		    zvals.removeSingle( idx );
		    idx--;
		}
	    }
	}

	PropertyRef::StdType proptyp =	newlog->propType();
	if ( proptyp == PropertyRef::Son ||  proptyp == PropertyRef::Vel )
	{
	    mGetZFac( logiop );
	    for ( int idx=0; idx<zvals.size(); idx++ )
	    {
		if ( mIsUdf(zvals[idx]) )
		    continue;
		else if ( proptyp == PropertyRef::Son )
		    zvals[idx] /= zfac;
		else if ( proptyp == PropertyRef::Vel )
		    zvals[idx] *= zfac;
	    }
	}

	newlog->setData( dahvals, zvals );
    }

    if ( newlog && !newlog->isEmpty() )
	adjustTrackIfNecessary();

    return addToLogSet( newlog, needjustinfo );
}


void Well::odReader::readLogData( Well::Log& wl, od_istream& strm,
				int bintype ) const
{
    ChangeNotifyBlocker nb( wl );
    float v[2];
    while ( strm.isOK() )
    {
	if ( !bintype )
	    strm >> v[0] >> v[1];
	else
	{
	    strm.getBin( v, 2 * sizeof(float) );
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


bool Well::odReader::getMarkers() const
{
    mGetInpStream( sExtMarkers(), 0, true, return true );
    return getMarkers( strm );
}


bool Well::odReader::getMarkers( od_istream& strm ) const
{
    if ( !rdHdr(strm,sKeyMarkers()) )
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
    wd_.markers().setEmpty();
    BufferString bs;

    ChangeNotifyBlocker nb( wd_.markers() );
    mGetZFac( iopar );
    for ( int idx=1;  ; idx++ )
    {
	BufferString basekey; basekey += idx;
	BufferString key = IOPar::compKey( basekey, sKey::Name() );
	if ( !iopar.get(key,bs) )
	    break;

	Well::Marker wm( bs );
	ChangeNotifyBlocker mnb( wm, false );

	key = IOPar::compKey( basekey, Well::Marker::sKeyDah() );
	if ( !iopar.get(key,bs) )
	    { continue; }
	wm.setDah( bs.toFloat() * zfac );

	key = IOPar::compKey( basekey, sKey::StratRef() );
	Well::Marker::LevelID lvlid;
	iopar.get( key, lvlid );
	wm.setLevelID( lvlid );

	key = IOPar::compKey( basekey, sKey::Color() );
	if ( iopar.get(key,bs) )
	{
	    Color col( wm.color() );
	    col.use( bs.buf() );
	    wm.setColor( col );
	}

	if ( !trackdahrg.includes(wm.dah(),false) && havetrack )
	    continue;
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
    mGetInpStream( csmdl ? sExtCSMdl() : sExtD2T(), 0, true, return false );
    return doGetD2T( strm, csmdl );
}


bool Well::odReader::getD2T( od_istream& strm ) const
{ return doGetD2T(strm,false); }
bool Well::odReader::getCSMdl( od_istream& strm ) const
{ return doGetD2T(strm,true); }
bool Well::odReader::doGetD2T( od_istream& strm, bool csmdl ) const
{
    if ( !rdHdr(strm,sKeyD2T()) )
	mErrRetStrmOper( sCannotReadFileHeader() )

    ascistream astrm( strm, false );
    IOPar iop;
    iop.getFrom( astrm );
    Well::D2TModel d2t;
    ChangeNotifyBlocker nb( d2t );

    d2t.useHdrPar( iop );
    mGetZFac( iop );
    while ( strm.isOK() )
    {
	float dah, val; strm >> dah >> val;
	if ( !strm.isOK() )
	    break;
	d2t.setValueAt( dah*zfac, val );
    }

    if ( !updateDTModel(d2t,csmdl,errmsg_) )
	mErrRetStrmOper( tr("Time/Depth relation in file is not valid") )

    return true;
}


bool Well::odReader::getDispProps() const
{
    mGetInpStream( sExtDispProps(), 0, true, return false );
    return getDispProps( strm );
}


bool Well::odReader::getDispProps( od_istream& strm ) const
{
    if ( !rdHdr(strm,sKeyDispProps()) )
	mErrRetStrmOper( sCannotReadFileHeader() )

    ascistream astrm( strm, false );
    IOPar iop; iop.getFrom( astrm );
    ChangeNotifyBlocker nb1( wd_.displayProperties2d() );
    ChangeNotifyBlocker nb2( wd_.displayProperties3d() );
    wd_.displayProperties2d().usePar( iop );
    wd_.displayProperties3d().usePar( iop );
    return true;
}

// MultiWellReader
MultiWellReader::MultiWellReader( const DBKeySet& keys, Well::LoadReqs loadreq )
    : Executor("Reading well info")
    , keys_(keys)
    , nrwells_(keys.size())
    , nrdone_(0)
    , loadreq_(loadreq)
    , msg_(tr("Reading Well Info"))
{}


od_int64 MultiWellReader::totalNr() const
{ return nrwells_; }

od_int64 MultiWellReader::nrDone() const
{ return nrdone_; }

uiString MultiWellReader::message() const
{ return msg_; }

uiString MultiWellReader::nrDoneText() const
{ return tr("Wells read"); }


int MultiWellReader::nextStep()
{
    if ( nrdone_ >= totalNr() )
    {
	if ( wds_.size() != keys_.size() )
	    allwellsread_ = false;

	if ( wds_.size() == 0 )
	{
	    msg_ = tr("No wells to be read");
	    return Executor::ErrorOccurred();
	}
	else
	    return Executor::Finished();
    }

    const DBKey wkey = keys_[sCast(int,nrdone_)];
    nrdone_++;
    if ( wkey.isInvalid() )
	return MoreToDo();

    ConstRefMan<Well::Data> wd;
    wd = Well::MGR().fetch( wkey, loadreq_ );
    if ( wd )
	wds_.add( new ConstRefMan<Well::Data>(wd) );

    return MoreToDo();
}


void MultiWellReader::getDoneWells(
			ObjectSet<ConstRefMan<Well::Data>>& wds ) const
{
    wds = wds_;
}
