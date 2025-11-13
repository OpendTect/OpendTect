/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellwriter.h"

#include "wellioprov.h"
#include "wellodwriter.h"
#include "wellreader.h"
#include "welltransl.h"

#include "ascstream.h"
#include "databuf.h"
#include "envvars.h"
#include "file.h"
#include "filesystemwatcher.h"
#include "ioobj.h"
#include "ioman.h"
#include "keystrs.h"
#include "od_ostream.h"
#include "settings.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "welldisp.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellmarker.h"
#include "wellodreader.h"
#include "welltrack.h"
#include "uistrings.h"


bool Well::Writer::isFunctional( const MultiID& ky )
{
    PtrMan<IOObj> ioobj = IOM().get( ky );
    return ioobj ? isFunctional(*ioobj) : false;
}


bool Well::Writer::isFunctional( const IOObj& ioobj )
{
    RefMan<Data> wd = new Data;
    Writer wrr( ioobj, *wd );
    return wrr.isFunctional();
}


bool Well::Writer::canRenameLogs( const MultiID& ky )
{
    PtrMan<IOObj> ioobj = IOM().get( ky );
    if ( !ioobj )
	return false;

    RefMan<Data> wd = new Data;
    Writer wrr( *ioobj, *wd );
    if ( !wrr.isUsable() )
	return false;

    return wrr.wa_ ? wrr.wa_->canRenameLogs() : false;
}


bool Well::Writer::canWriteInParallel( const MultiID& ky )
{
    PtrMan<IOObj> ioobj = IOM().get( ky );
    if ( !ioobj )
	return false;

    RefMan<Data> wd = new Data;
    Writer wrr( *ioobj, *wd );
    if ( !wrr.isUsable() )
	return false;

    return wrr.wa_ ? wrr.wa_->canWriteInParallel() : false;
}


Well::Writer::Writer( const IOObj& ioobj, const Data& wd )
{
    nsfile_ = new NotifyStopper( FSW().fileChanged );
    nsdir_ = new NotifyStopper( FSW().directoryChanged );
    init( ioobj, wd );
}


Well::Writer::Writer( const MultiID& ky, const Data& wd )
{
    nsfile_ = new NotifyStopper( FSW().fileChanged );
    nsdir_ = new NotifyStopper( FSW().directoryChanged );
    ConstPtrMan<IOObj> ioobj = IOM().get( ky );
    if ( ioobj )
	init( *ioobj, wd );
    else
	errmsg_.appendPhrase( uiStrings::phrCannotFindDBEntry(ky) );
}


void Well::Writer::init( const IOObj& ioobj, const Data& wd )
{
    if ( ioobj.group() != mTranslGroupName(Well) )
	errmsg_ = tr("%1 is for a %2- not for a Well")
			.arg(ioobj.name()).arg(ioobj.group());
    else
    {
	wa_ = WDIOPF().getWriteAccess( ioobj, wd, errmsg_ );
	if ( !wa_ )
	    errmsg_ = uiStrings::phrCannotCreate(tr("writer of type %1"))
					   .arg(ioobj.translator());
    }
}


Well::Writer::~Writer()
{
    delete wa_;
    delete nsfile_;
    delete nsdir_;
}

#define mImplWWFn(rettyp,fnnm,typ,arg,udf) \
rettyp Well::Writer::fnnm( typ arg ) const \
{ return wa_ ? wa_->fnnm(arg) : udf; }
#define mImplSimpleWWFn(fnnm) \
bool Well::Writer::fnnm() const { return wa_ ? wa_->fnnm() : false; }

mImplSimpleWWFn(put)
mImplSimpleWWFn(putInfo)
mImplSimpleWWFn(putTrack)
mImplSimpleWWFn(putMarkers)
mImplSimpleWWFn(putD2T)
mImplSimpleWWFn(putCSMdl)
mImplSimpleWWFn(putDispProps)
mImplSimpleWWFn(putDefLogs)
mImplSimpleWWFn(isFunctional)

mImplWWFn(bool,putLog,const Log&,wl,false)


bool Well::Writer::put( const StoreReqs& reqs ) const
{
    if ( wa_->needsInfoAndTrackCombined() &&
	(reqs.includes(Inf) || reqs.includes(Trck)) )
	 putTrack();
    else
    {
	if ( reqs.includes(Inf) )
	    putInfo();

	if ( reqs.includes(Trck) )
	    putTrack();
    }

    if ( reqs.includes(CSMdl) )
	putCSMdl();

    if ( reqs.includes(D2T) )
	putD2T();

    if ( reqs.includes(Mrkrs) )
	putMarkers();

    if ( reqs.includes(Logs) || reqs.includes(LogInfos) )
	putLogs();

    if ( reqs.includes(DispProps2D) || reqs.includes(DispProps3D) )
	putDispProps();

    return true;
}


bool Well::Writer::putInfoAndTrack() const
{
    return put( StoreReqs(Inf,Trck) );
}


bool Well::Writer::putLogs() const
{
    if ( wa_ )
	return wa_->putLogs() && wa_->putDefLogs();

    return false;
}


bool Well::Writer::renameLog( const char* oldnm, const char* newnm )
{
    if ( !wa_ || !data() )
	return false;

    const LogSet& logs = data()->logs();
    const Log* wl = logs.getLogInfos( oldnm );
    if ( !wl )
    {
	errmsg_ = tr("Cannot find the log to rename in the set");
	return false;
    }

    if ( !wa_->renameLog(oldnm,newnm) )
	return false;

    if ( logs.isPresent(oldnm) )
    {
	pErrMsg("The writer implementation must rename the log in the set");
    }

    return logs.isPresent( newnm );
}


const Well::Data* Well::Writer::data() const
{
    return wa_ ? &wa_->data() : nullptr;
}


#define mErrStrmOper(oper,todo) \
{ setStrmErrMsg(strm,oper); todo; }
#define mErrRetStrmOper(oper) mErrStrmOper(oper,return false)

#define mGetOutStream(ext,nr,todo) \
    errmsg_.setEmpty(); \
    od_ostream strm( getFileName(ext,nr) ); \
    if ( !strm.isOK() ) mErrStrmOper(startWriteStr(),todo)


Well::odWriter::odWriter( const char* f, const Data& w, uiString& errmsg )
    : odIO(f,errmsg)
    , WriteAccess(w)
{
    init();
}


Well::odWriter::odWriter( const IOObj& ioobj, const Data& w,
			  uiString& errmsg )
    : odIO(ioobj.fullUserExpr(false),errmsg)
    , WriteAccess(w)
{
    getNonConst( wd_ ).setMultiID( ioobj.key() );
    init();
    ioobj.pars().getYN( sKeyLogStorage(), binwrlogs_ );
}


Well::odWriter::~odWriter()
{}


void Well::odWriter::init()
{
    binwrlogs_ = true;
    mSettUse(getYN,"dTect.Well logs","Binary format",binwrlogs_);
}


void Well::odWriter::setStrmErrMsg( od_stream& strm,
				    const uiString& oper ) const
{
    errmsg_ = tr("Cannot %1 for %2").arg( oper ).arg( strm.fileName() );
    strm.addErrMsgTo( errmsg_ );
}


uiString Well::odWriter::startWriteStr() const
{
    return tr("start writing");
}


bool Well::odWriter::isFunctional() const
{
    return !basenm_.isEmpty();
}


bool Well::odWriter::wrHdr( od_ostream& strm, const char* fileky ) const
{
    ascostream astrm( strm );
    return astrm.putHeader( fileky );
}


bool Well::odWriter::put() const
{
    return putTrack()
	&& putCSMdl()
	&& putD2T()
	&& putMarkers()
	&& putLogs()
	&& putDispProps();
}


bool Well::odWriter::putInfo() const
{
    return putTrack();
}


bool Well::odWriter::putTrack() const
{
    mGetOutStream( sExtWell(), 0, return false )
    return putInfo( strm ) && putTrack( strm );
}


bool Well::odWriter::putInfo( od_ostream& strm ) const
{
    if ( !wrHdr(strm,sKeyWell()) )
	mErrRetStrmOper(tr("write header (info/track)"))

    const auto& info = wd_.info();
    ascostream astrm( strm );
    astrm.put( Info::sKeyDepthUnit(),
	    UnitOfMeasure::surveyDefDepthStorageUnit()->symbol() );
    astrm.put( Info::sKeyUwid(), info.uwid_ );
    astrm.put( Info::sKeyOper(), info.oper_ );
    astrm.put( Info::sKeyField(), info.field_ );
    astrm.put( Info::sKeyCounty(), info.county_ );
    astrm.put( Info::sKeyState(), info.state_ );
    astrm.put( Info::sKeyProvince(), info.province_ );
    astrm.put( Info::sKeyCountry(), info.country_ );
    astrm.put( Info::sKeyWellType(), info.welltype_ );
    if ( info.surfacecoord_ != Coord(0,0) )
	astrm.put( Info::sKeyCoord(), info.surfacecoord_.toString() );
    if ( info.surfacelatlong_.isDefined() )
	astrm.put( sKey::LatLong(), info.surfacelatlong_.toString() );
    astrm.put( Info::sKeyReplVel(), info.replvel_ );
    astrm.put( Info::sKeyGroundElev(), info.groundelev_ );
    astrm.newParagraph();

    return true;
}


bool Well::odWriter::putTrack( od_ostream& strm ) const
{
    for ( int idx=0; idx<wd_.track().size(); idx++ )
    {
	const Coord3& c = wd_.track().pos(idx);
	    // don't try to do the following in one statement
	    // (unless for educational purposes)
        strm << c.x_ << od_tab;
        strm << c.y_ << od_tab;
        strm << c.z_ << od_tab;
	strm << wd_.track().dah(idx) << od_newline;
    }
    if ( !strm.isOK() )
	mErrRetStrmOper(tr("write track data"))

    return true;
}


DataBuffer* Well::odWriter::getLogBuffer( od_istream& istrm ) const
{
    ascistream asistrm( istrm, true );
    while ( !atEndOfSection(asistrm.next()) )
    {}

    if ( istrm.isOK() )
    {
	int size
	    = File::getFileSize(istrm.fileName()) - istrm.position();
	auto* databuf = new DataBuffer( size, 1 );
	istrm.getBin( databuf->data(), size );
	return databuf;
    }

    istrm.close();
    return nullptr;
}


bool Well::odWriter::putLogs() const
{
    ManagedObjectSet<DataBuffer> databufset;
    const LogSet& logs = wd_.logs();
    for ( int idx=logs.size()-1; idx>=0; idx-- )
    {
	const Log& wl = logs.getLogByIdx( idx );
	if ( wl.isEmpty() )
	    delete getNonConst( logs ).remove( idx );
    }

    TypeSet<int> bintypes( logs.size(), mUdf(int) );
    BufferStringSet lognms;
    logs.getNames( lognms );
    for ( int idx=0; idx<lognms.size(); idx++ )
    {
	const BufferString fnm( getFileName(sExtLog(), idx+1) );
	bintypes[idx] = odReader::getStorageType( fnm );
	od_istream istrm( fnm );
	if ( !istrm.isOK() )
	    continue;

	if ( logs.isLoaded(lognms.get(idx).buf()) )
	    continue;

	databufset.add( getLogBuffer(istrm) );
    }

    removeAll( sExtLog() );
    int idy = 0;
    for ( int idx=0; idx<lognms.size(); idx++ )
    {
	const char* lognm = lognms.get( idx ).buf();
	const Log* wl = logs.isLoaded( lognm ) ? logs.getLog( lognm )
					       : logs.getLogInfos( lognm);
	if ( !wl )
	    continue;

	const int bintype = bintypes.validIdx( idx) ? bintypes[idx] : mUdf(int);
	const DataBuffer* dbuf = wl->isLoaded() ? nullptr :
			    (databufset.validIdx(idy) ? databufset.get(idy++)
						      : nullptr);
	mGetOutStream( sExtLog(), idx+1, return false )

	errmsg_.setEmpty();
	if ( !putLog(strm,*wl,bintype,dbuf) )
	    return false;
    }

    return putDefLogs();
}


bool Well::odWriter::putLog( const Log& wl ) const
{
    const int logidx = getLogIndex( wl.name() );
    const BufferString logfnm = getFileName( odIO::sExtLog(), logidx );

    const int bintype = odReader::getStorageType( logfnm.str() );
    od_istream istrm( logfnm );
    const DataBuffer* dbuf = wl.isLoaded() ? nullptr : getLogBuffer( istrm );

    od_ostream strm( logfnm );
    const bool res = putLog( strm, wl, bintype, dbuf );
    delete dbuf;

    return res;
}


bool Well::odWriter::putLog( od_ostream& strm, const Log& wl,
			     int bintype, const DataBuffer* databuf ) const
{
    if ( !wrHdr(strm,sKeyLog()) )
	mErrRetStrmOper(tr("write header (log)"))

    if ( !wrLogHdr(wl,bintype,strm) || !wrLogData(wl,bintype,databuf,strm) )
	mErrRetStrmOper(tr("write log data"))

    return true;
}


bool Well::odWriter::wrLogHdr( const Log& wl, int bintype,
			       od_ostream& strm ) const
{
    if ( !strm.isOK() )
	return false;

    ascostream astrm( strm );
    astrm.put( Info::sKeyDepthUnit(),
	       UnitOfMeasure::surveyDefDepthStorageUnit()->symbol() );
    astrm.put( sKey::Name(), wl.name() );
    const bool havemnemonics = !StringView(wl.mnemonicLabel()).isEmpty();
    const bool haveunits = wl.haveUnit();
    const bool havepars = !wl.pars().isEmpty();
    if ( havemnemonics )
	astrm.put( Log::sKeyMnemLbl(), wl.mnemonicLabel() );

    if ( haveunits )
	astrm.put( Log::sKeyUnitLbl(), wl.unitMeasLabel() );

    astrm.putYN( Log::sKeyHdrInfo(), havepars );
    const char* stortype;
    if ( mIsUdf(bintype) || (bintype < 1 && bintype > 1) )
    {
	stortype = binwrlogs_ ? (__islittle__ ? "Binary" : "Swapped")
			      : "Ascii";
    }
    else
    {
	stortype = bintype == 0 ? "Ascii"
				: (bintype == 1 ? "Binary" : "Swapped");
    }

    astrm.put( Log::sKeyStorage(), stortype );
    const Interval<float>& dahrange = wl.dahRange();
    if ( !dahrange.isUdf() )
	astrm.put( Log::sKeyDahRange(), dahrange.start_, dahrange.stop_ );

    const Interval<float>& logrange = wl.valueRange();
    if ( !logrange.isUdf() )
	astrm.put( Log::sKeyLogRange(), logrange.start_, logrange.stop_ );

    astrm.newParagraph();
    if ( havepars )
	wl.pars().putTo( astrm );

    return strm.isOK();
}


bool Well::odWriter::wrLogData( const Log& wl, int bintype,
				const DataBuffer* databuf,
				od_ostream& strm ) const
{
    if ( !strm.isOK() )
	return false;

    if ( databuf )
    {
	strm.addBin( databuf->data(), databuf->size() );
	return true;
    }

    bool binwrlogs = binwrlogs_;
    if ( !mIsUdf(bintype) && bintype >=-1 && bintype <=-1 )
	binwrlogs = bintype != 0;

    Interval<int> wrintv( 0, wl.size()-1 );
    float dah, val;
    for ( ; wrintv.start_<wl.size(); wrintv.start_++ )
    {
	dah = wl.dah(wrintv.start_); val = wl.value(wrintv.start_);
	if ( !mIsUdf(dah) && !mIsUdf(val) )
	    break;
    }

    for ( ; wrintv.stop_>=0; wrintv.stop_-- )
    {
	dah = wl.dah(wrintv.stop_); val = wl.value(wrintv.stop_);
	if ( !mIsUdf(dah) && !mIsUdf(val) )
	    break;
    }

    float v[2];
    for ( int idx=wrintv.start_; idx<=wrintv.stop_; idx++ )
    {
	v[0] = wl.dah( idx );
	if ( mIsUdf(v[0]) )
	    continue;

	v[1] = wl.value( idx );
	if ( binwrlogs )
	    strm.addBin( v );
	else
	{
	    strm << v[0] << od_tab;
	    if ( mIsUdf(v[1]) )
		strm << sKey::FloatUdf();
	    else
		strm << v[1];

	    strm << od_newline;
	}
    }

    if ( !strm.isOK() )
	mErrRetStrmOper(tr("write log data"))

    return true;
}


bool Well::odWriter::renameLog( const char* oldnm, const char* newnm )
{
    const int logidx = getLogIndex( oldnm );
    const BufferString logfnm = getFileName( odIO::sExtLog(), logidx );
    const int bintype = odReader::getStorageType( logfnm );
    od_istream istrm( logfnm );
    const Log* wl = wd_.logs().getLogInfos( oldnm );
    const DataBuffer* dbuf = wl->isLoaded() ? nullptr : getLogBuffer( istrm );

    getNonConst( wl )->setName( newnm );

    od_ostream strm( logfnm );
    const bool res = putLog( strm, *wl, bintype, dbuf );
    delete dbuf;

    return res;
}


int Well::odWriter::getLogIndex( const char* lognm ) const
{
    int logidx = -1;
    //TODO: to be replaced by a proper well log identifier:
    int nrlogs = -1;
    if ( isFunctional() )
    {
	Reader rdr( wd_.multiID(), const_cast<Data&>( wd_ ) );
	if ( rdr.isUsable() )
	{
	    BufferStringSet lognms;
	    rdr.getLogNames( lognms );
	    logidx = lognms.indexOf( lognm );
	    nrlogs = lognms.size();
	}
    }

    if ( logidx < 0 )
    {
	//Unsafe !!!
	logidx = nrlogs < 0 ? 0 : nrlogs;
    }

    logidx++;
    return logidx;
}


bool Well::odWriter::putDefLogs() const
{
    mGetOutStream( sExtDefaults(), 0, return false )
    return putDefLogs( strm );
}


bool Well::odWriter::putDefLogs( od_ostream& strm ) const
{
    if ( !wrHdr(strm,sKeyDefaults()) )
	mErrRetStrmOper(tr("write header (default logs)"))

    ascostream astrm( strm );
    IOPar iop;
    wd_.logs().defaultLogFillPar( iop );
    iop.putTo( astrm );
    if ( !strm.isOK() )
	mErrRetStrmOper(tr("write well display parameters"))

    return true;
}


bool Well::odWriter::putMarkers() const
{
    mGetOutStream( sExtMarkers(), 0, return false )
    return putMarkers( strm );
}


bool Well::odWriter::putMarkers( od_ostream& strm ) const
{
    if ( !wrHdr(strm,sKeyMarkers()) )
	mErrRetStrmOper(tr("write header (markers)"))

    ascostream astrm( strm );
    astrm.put( Info::sKeyDepthUnit(),
	    UnitOfMeasure::surveyDefDepthStorageUnit()->symbol() );
    for ( int idx=0; idx<wd_.markers().size(); idx++ )
    {
	BufferString basekey; basekey += idx+1;
	const Marker& wm = *wd_.markers()[idx];
	const float dah = wm.dah();
	if ( mIsUdf(dah) )
	    continue;

	astrm.put( IOPar::compKey(basekey,sKey::Name()), wm.getDatabaseName() );
	astrm.put( IOPar::compKey(basekey,Marker::sKeyDah()), dah );
	astrm.put( IOPar::compKey(basekey,sKey::StratRef()),
		   wm.levelID().asInt() );
	BufferString bs;
	wm.getDatabaseColor().fill( bs );
	astrm.put( IOPar::compKey(basekey,sKey::Color()), bs );
    }

    if ( !strm.isOK() )
	mErrRetStrmOper(tr("write markers"))
    return true;
}


bool Well::odWriter::putD2T() const	{ return doPutD2T( false ); }
bool Well::odWriter::putCSMdl() const	{ return doPutD2T( true ); }
bool Well::odWriter::doPutD2T( bool csmdl ) const
{
    if ( (csmdl && !wd_.checkShotModel()) || (!csmdl && !wd_.d2TModel()) )
	return true;

    mGetOutStream( csmdl ? sExtCSMdl() : sExtD2T(), 0, return false )
    return doPutD2T( strm, csmdl );
}


bool Well::odWriter::putD2T( od_ostream& strm ) const
{ return doPutD2T( strm, false ); }
bool Well::odWriter::putCSMdl( od_ostream& strm ) const
{ return doPutD2T( strm, true ); }
bool Well::odWriter::doPutD2T( od_ostream& strm, bool csmdl ) const
{
    if ( !wrHdr(strm,sKeyD2T()) )
	mErrRetStrmOper(tr("write header (D2T model)"))

    ascostream astrm( strm );
    const D2TModel& d2t = *(csmdl ? wd_.checkShotModel(): wd_.d2TModel());
    astrm.put( sKey::Name(), d2t.name() );
    astrm.put( sKey::Desc(), d2t.desc );
    astrm.put( D2TModel::sKeyDataSrc(), d2t.datasource );
    astrm.put( Info::sKeyDepthUnit(),
	    UnitOfMeasure::surveyDefDepthStorageUnit()->symbol() );
    astrm.newParagraph();

    for ( int idx=0; idx<d2t.size(); idx++ )
    {
	const float dah = d2t.dah( idx );
	if ( mIsUdf(dah) )
	    continue;

	strm.addPrecise( dah );
	strm << od_tab;
	strm.addPrecise( d2t.t(idx) );
	strm << od_newline;
    }

    if ( !strm.isOK() )
	mErrRetStrmOper(tr("write Depth/Time data"))

    return true;
}


bool Well::odWriter::putDispProps() const
{
    mGetOutStream( sExtDispProps(), 0, return false )
    return putDispProps( strm );
}


bool Well::odWriter::putDispProps( od_ostream& strm ) const
{
    if ( !wrHdr(strm,sKeyDispProps()) )
	mErrRetStrmOper(tr("write header (display parameters)"))

    ascostream astrm( strm );
    IOPar iop;
    wd_.displayProperties(true).fillPar( iop );
    wd_.displayProperties(false).fillPar( iop );
    iop.putTo( astrm );
    if ( !strm.isOK() )
	mErrRetStrmOper(tr("write well display parameters"))

    return true;
}


// MultiWellWriter

MultiWellWriter::MultiWellWriter( const ObjectSet<Well::Data>& wds,
				  const TypeSet<StoreReqs>& reqs )
    : Executor("Saving Wells")
    , wds_(wds)
    , reqs_(reqs)
    , nrwells_(wds.size())
    , nrdone_(0)
{
    msg_ = tr("Writing Wells");
}


MultiWellWriter::~MultiWellWriter()
{}


od_int64 MultiWellWriter::totalNr() const
{ return nrwells_; }

od_int64 MultiWellWriter::nrDone() const
{ return nrdone_; }

uiString MultiWellWriter::uiMessage() const
{ return msg_; }

uiString MultiWellWriter::uiNrDoneText() const
{ return tr("Wells written"); }


int MultiWellWriter::nextStep()
{
    if ( nrdone_ >= totalNr() )
    {
	if ( wds_.size() == 0 )
	{
	    msg_ = tr( "No wells to be written" );
	    return ErrorOccurred();
	}

	return Finished();
    }

    ConstRefMan<Well::Data> wd = wds_[nrdone_];
    if ( !wd )
    {
	allwellswritten_ = false;
	nrdone_++;
	return MoreToDo();
    }

    if ( !store(wd->multiID(),*wd,reqs_[nrdone_]) )
    {
	failedwellids_.addIfNew( wd->multiID() );
	allwellswritten_ = false;
    }

    nrdone_++;
    return MoreToDo();
}


bool MultiWellWriter::store( const MultiID& key, const Well::Data& wd,
						 const StoreReqs& reqs )
{
    Well::Writer wrtr( key, wd );
    return wrtr.put( reqs );
}


// Well::WriteAccess
Well::WriteAccess::WriteAccess( const Data& wd )
    : wd_(wd)
{
    const BufferString uwi = wd.info().uwid_;
    const MultiID key = wd.multiID();
    if ( !key.isUdf() && !IOM().implExists(key) )
	putUWI( key, uwi );
}


Well::WriteAccess::~WriteAccess()
{}
