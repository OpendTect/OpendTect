/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Aug 2003
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "wellwriter.h"
#include "wellodwriter.h"
#include "wellioprov.h"
#include "welltransl.h"

#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "welldisp.h"
#include "ascstream.h"
#include "hiddenparam.h"
#include "od_ostream.h"
#include "od_istream.h"
#include "keystrs.h"
#include "envvars.h"
#include "settings.h"
#include "ioobj.h"
#include "ioman.h"
#include "file.h"
#include "databuf.h"

static HiddenParam<MultiWellWriter,
	   const ObjectSet<MultiWellWriter::StoreReqs>*> storereqs_(nullptr);

bool Well::Writer::isFunctional( const MultiID& ky )
{
    PtrMan<IOObj> ioobj = IOM().get( ky );
    return ioobj ? isFunctional(*ioobj) : false;
}


bool Well::Writer::isFunctional( const IOObj& ioobj )
{
    RefMan<Well::Data> wd = new Well::Data;
    Well::Writer wrr( ioobj, *wd );
    return wrr.isFunctional();
}


Well::Writer::Writer( const IOObj& ioobj, const Well::Data& wd )
    : wa_(0)
{
    init( ioobj, wd );
}


Well::Writer::Writer( const MultiID& ky, const Well::Data& wd )
    : wa_(0)
{
    IOObj* ioobj = IOM().get( ky );
    if ( !ioobj )
	errmsg_.appendPhrase( uiStrings::phrCannotFindDBEntry(ky) );
    else
    {
	init( *ioobj, wd );
	delete ioobj;
    }
}


void Well::Writer::init( const IOObj& ioobj, const Well::Data& wd )
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
}

#define mImplWWFn(rettyp,fnnm,typ,arg,udf) \
rettyp Well::Writer::fnnm( typ arg ) const \
{ return wa_ ? wa_->fnnm(arg) : udf; }
#define mImplSimpleWWFn(fnnm) \
bool Well::Writer::fnnm() const { return wa_ ? wa_->fnnm() : false; }

mImplSimpleWWFn(put)
mImplSimpleWWFn(putInfoAndTrack)
mImplSimpleWWFn(putLogs)
mImplSimpleWWFn(putMarkers)
mImplSimpleWWFn(putD2T)
mImplSimpleWWFn(putCSMdl)
mImplSimpleWWFn(putDispProps)
mImplSimpleWWFn(isFunctional)

mImplWWFn(bool,putLog,const Log&,wl,false)


#define mErrStrmOper(oper,todo) \
{ setStrmErrMsg(strm,oper); todo; }
#define mErrRetStrmOper(oper) mErrStrmOper(oper,return false)

#define mGetOutStream(ext,nr,todo) \
    errmsg_.setEmpty(); \
    od_ostream strm( getFileName(ext,nr) ); \
    if ( !strm.isOK() ) mErrStrmOper(startWriteStr(),todo)


Well::odWriter::odWriter( const char* f, const Well::Data& w, uiString& errmsg )
    : Well::odIO(f,errmsg)
    , Well::WriteAccess(w)
{
    init();
}


Well::odWriter::odWriter( const IOObj& ioobj, const Well::Data& w,
			  uiString& errmsg )
    : Well::odIO(ioobj.fullUserExpr(false),errmsg)
    , Well::WriteAccess(w)
{
    wd_.setMultiID( ioobj.key() );
    init();
    ioobj.pars().getYN( sKeyLogStorage(), binwrlogs_ );
}


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
    return putInfoAndTrack()
	&& putLogs()
	&& putMarkers()
	&& putD2T()
	&& putCSMdl()
	&& putDispProps();
}


bool Well::odWriter::putInfoAndTrack() const
{
    mGetOutStream( sExtWell(), 0, return false )
    return putInfoAndTrack( strm );
}


bool Well::odWriter::putInfoAndTrack( od_ostream& strm ) const
{
    if ( !wrHdr(strm,sKeyWell()) )
	mErrRetStrmOper(tr("write header (info/track)"))

    ascostream astrm( strm );
    astrm.put( Well::Info::sKeyDepthUnit(),
	    UnitOfMeasure::surveyDefDepthStorageUnit()->name() );
    astrm.put( Well::Info::sKeyUwid(), wd_.info().uwid );
    astrm.put( Well::Info::sKeyOper(), wd_.info().oper );
    astrm.put( Well::Info::sKeyState(), wd_.info().state );
    astrm.put( Well::Info::sKeyCounty(), wd_.info().county );
    if ( wd_.info().surfacecoord != Coord(0,0) )
	astrm.put( Well::Info::sKeyCoord(), wd_.info().surfacecoord.toString());
    astrm.put( Well::Info::sKeyReplVel(), wd_.info().replvel );
    astrm.put( Well::Info::sKeyGroundElev(), wd_.info().groundelev );
    astrm.newParagraph();

    return putTrack( strm );
}


bool Well::odWriter::putTrack( od_ostream& strm ) const
{
    for ( int idx=0; idx<wd_.track().size(); idx++ )
    {
	const Coord3& c = wd_.track().pos(idx);
	    // don't try to do the following in one statement
	    // (unless for educational purposes)
	strm << c.x << od_tab;
	strm << c.y << od_tab;
	strm << c.z << od_tab;
	strm << wd_.track().dah(idx) << od_newline;
    }
    if ( !strm.isOK() )
	mErrRetStrmOper(tr("write track data"))
    return true;
}


bool Well::odWriter::putTrack() const
{
    mGetOutStream( sExtTrack(), 0, return false )
    return putTrack( strm );
}


DataBuffer* Well::odWriter::getLogBuffer( od_istream& istrm ) const
{
    ascistream asistrm( istrm, true );
    while ( !atEndOfSection(asistrm.next()) )
    {}

    if ( istrm.isOK() )
    {
	int size = File::getFileSize(istrm.fileName()) - istrm.position();
	auto* databuf = new DataBuffer( size, 1 );
	istrm.getBin( databuf->data(), size );
	return databuf;
    }

    return nullptr;
}


bool Well::odWriter::putLogs() const
{
    ManagedObjectSet<DataBuffer> databufset;
    for ( int idx=0; idx<wd_.logs().size(); idx++ )
    {
	BufferString fnm( getFileName(sExtLog(), idx+1) );
	od_istream istrm( fnm );
	if ( !istrm.isOK() )
	    continue;

	const Well::Log& wl = wd_.logs().getLog(idx);
	if ( !wl.isLoaded() )
	    continue;

	databufset.add( getLogBuffer(istrm) );
    }

    removeAll( sExtLog() );
    int idy = 0;
    for ( int idx=0; idx<wd_.logs().size(); idx++ )
    {
	mGetOutStream( sExtLog(), idx+1, return false )

	errmsg_.setEmpty();
	const Well::Log& wl = wd_.logs().getLog(idx);
	const DataBuffer* dbuf = wl.isLoaded() ? nullptr
					       : databufset.get(idy++);
	if ( !writeLog(strm,wl,dbuf) )
	    return false;
    }

    return true;
}


bool Well::odWriter::putLog( const Well::Log& wl ) const
{
    const int logidx = wd_.logs().indexOf( wl.name() );
    if ( logidx<0 )
    {
	pErrMsg( "First add Log to Well::Data" );
	return false;
    }

    BufferString logfnm = getFileName( Well::odIO::sExtLog(), logidx+1 );
    od_istream istrm( logfnm );
    const DataBuffer* dbuf = wl.isLoaded() ? nullptr : getLogBuffer(istrm);
    istrm.close();
    od_ostream strm( logfnm );
    if ( !writeLog(strm,wl,dbuf) )
    {
	delete dbuf;
	return false;
    }

    delete dbuf;
    return true;
}


bool Well::odWriter::writeLog( od_ostream& strm, const Well::Log& wl,
					   const DataBuffer* databuf ) const
{
    if ( !wrHdr(strm,sKeyLog()) )
	mErrRetStrmOper(tr("write header (log)"))

    if ( !wrLogHdr(strm, wl) || !wrLogData(strm, wl, databuf) )
	mErrRetStrmOper(tr("write log data"))	 

    return true;
}


bool Well::odWriter::putLog( od_ostream& strm, const Well::Log& wl ) const
{
    if ( !writeLog(strm,wl) )
	return false;

    return true;
}


bool Well::odWriter::wrLogHdr( od_ostream& strm, const Well::Log& wl ) const
{
    if ( !strm.isOK() )
	return false;

    ascostream astrm( strm );
    astrm.put( Well::Info::sKeyDepthUnit(),
	    UnitOfMeasure::surveyDefDepthStorageUnit()->name() );
    astrm.put( sKey::Name(), wl.name() );
    const bool haveunits = *wl.unitMeasLabel();
    const bool havemnemonics = wl.mnemLabel();
    const bool havepars = !wl.pars().isEmpty();
    if ( havemnemonics )
	astrm.put( Well::Log::sKeyMnemLbl(), wl.mnemLabel() );
    if ( haveunits )
	astrm.put( Well::Log::sKeyUnitLbl(), wl.unitMeasLabel() );
    astrm.putYN( Well::Log::sKeyHdrInfo(), havepars );
    const char* stortyp = binwrlogs_ ? (__islittle__ ? "Binary" : "Swapped")
				     : "Ascii";
    astrm.put( Well::Log::sKeyStorage(), stortyp );
    astrm.put( Well::Log::sKeyDahRange(), wl.dahRange().start,
					  wl.dahRange().stop );
    astrm.newParagraph();
    if ( havepars )
	wl.pars().putTo( astrm );

    return strm.isOK();
}


bool Well::odWriter::wrLogData( od_ostream& strm, const Well::Log& wl,
					   const DataBuffer* databuf ) const
{
    if ( !strm.isOK() )
	return false;

    if ( databuf )
    {
	strm.addBin( databuf->data(), databuf->size() );
	return true;
    }

    Interval<int> wrintv( 0, wl.size()-1 );
    float dah, val;
    for ( ; wrintv.start<wl.size(); wrintv.start++ )
    {
	dah = wl.dah(wrintv.start);
	val = wl.value(wrintv.start);
	if ( !mIsUdf(dah) && !mIsUdf(val) )
	    break;
    }
    for ( ; wrintv.stop>=0; wrintv.stop-- )
    {
	dah = wl.dah(wrintv.stop);
	val = wl.value(wrintv.stop);
	if ( !mIsUdf(dah) && !mIsUdf(val) )
	    break;
    }

    float v[2];
    for ( int idx=wrintv.start; idx<=wrintv.stop; idx++ )
    {
	v[0] = wl.dah( idx );
	if ( mIsUdf(v[0]) )
	    continue;

	v[1] = wl.value( idx );
	if ( binwrlogs_ )
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
    astrm.put( Well::Info::sKeyDepthUnit(),
	    UnitOfMeasure::surveyDefDepthStorageUnit()->name() );
    for ( int idx=0; idx<wd_.markers().size(); idx++ )
    {
	BufferString basekey; basekey += idx+1;
	const Well::Marker& wm = *wd_.markers()[idx];
	const float dah = wm.dah();
	if ( mIsUdf(dah) )
	    continue;

	astrm.put( IOPar::compKey(basekey,sKey::Name()), wm.name() );
	astrm.put( IOPar::compKey(basekey,Well::Marker::sKeyDah()), dah );
	astrm.put( IOPar::compKey(basekey,sKey::StratRef()), wm.levelID() );
	BufferString bs; wm.color().fill( bs );
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
    const Well::D2TModel& d2t = *(csmdl ? wd_.checkShotModel(): wd_.d2TModel());
    astrm.put( sKey::Name(), d2t.name() );
    astrm.put( sKey::Desc(), d2t.desc );
    astrm.put( D2TModel::sKeyDataSrc(), d2t.datasource );
    astrm.put( Well::Info::sKeyDepthUnit(),
	    UnitOfMeasure::surveyDefDepthStorageUnit()->name() );
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


MultiWellWriter::MultiWellWriter( const ObjectSet<Well::Data>& wds )
    : Executor("Saving Wells")
    , wds_(wds)
    , nrwells_(wds.size())
    , nrdone_(0)
{
    storereqs_.setParam( this, nullptr );
    msg_ = tr("Writing Wells");
}


MultiWellWriter::MultiWellWriter( const ObjectSet<Well::Data>& wds,
				  const ObjectSet<StoreReqs>& reqs )
    : Executor("Saving Wells")
    , wds_(wds)
    , nrwells_(wds.size())
    , nrdone_(0)
{
    storereqs_.setParam( this, &reqs );
    msg_ = tr("Writing Wells");
}


MultiWellWriter::~MultiWellWriter()
{
    storereqs_.removeParam( this );
}


od_int64 MultiWellWriter::totalNr() const
{ return nrwells_; }

od_int64 MultiWellWriter::nrDone() const
{ return nrdone_; }

uiString MultiWellWriter::uiMessage() const
{ return msg_; }

uiString MultiWellWriter::uiNrDoneText() const
{ return tr("Wells read"); }


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

    StoreReqs reqs;
    if ( storereqs_.getParam(this)
	    && storereqs_.getParam(this)->validIdx(nrdone_) )
	reqs = *storereqs_.getParam(this)->get(nrdone_);

    if ( !store(wd->multiID(),*wd,reqs) )
	allwellswritten_ = false;

    nrdone_++;
    return MoreToDo();
}


bool MultiWellWriter::store( const MultiID& key, const Well::Data& wd,
						 const StoreReqs reqs )
{
    Well::Writer wrtr( key, wd );
    if ( reqs.includes(Well::Inf) || reqs.includes(Well::Trck) )
	wrtr.putInfoAndTrack();
    if ( reqs.includes(Well::D2T) )
	wrtr.putD2T();
    if ( reqs.includes(Well::Mrkrs) )
	wrtr.putMarkers();
    if ( reqs.includes(Well::Logs) )
	wrtr.putLogs();
    if ( reqs.includes(Well::LogInfos) )
	wrtr.putLogs();
    if ( reqs.includes(Well::CSMdl) )
	wrtr.putCSMdl();
    if ( reqs.includes(Well::DispProps2D) || reqs.includes(Well::DispProps3D) )
	wrtr.putDispProps();

    return true;
}
