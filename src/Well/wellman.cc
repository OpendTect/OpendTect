/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellman.h"

#include "filepath.h"
#include "filesystemwatcher.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "mnemonics.h"
#include "ptrman.h"
#include "surveydisklocation.h"
#include "survinfo.h"

#include "welldata.h"
#include "wellio.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellmarker.h"
#include "wellreader.h"
#include "welltrack.h"
#include "welltransl.h"
#include "wellupdate.h"
#include "wellwriter.h"

Well::LoadReqs::LoadReqs( bool addall )
{
    if ( addall )
	setToAll();
}


Well::LoadReqs::LoadReqs( SubObjType typ )
{
    add( typ );
}


Well::LoadReqs::LoadReqs( SubObjType typ1, SubObjType typ2 )
{
    add( typ1 ).add( typ2 );
}


Well::LoadReqs::LoadReqs( SubObjType typ1, SubObjType typ2, SubObjType typ3 )
{
    add( typ1 ).add( typ2 ).add( typ3 );
}


Well::LoadReqs::LoadReqs( const OD::String& lognm )
{
    addLog( lognm.buf() );
}


Well::LoadReqs::LoadReqs( const BufferStringSet& lognms )
{
    addLogs( lognms );
}


Well::LoadReqs::~LoadReqs()
{
}


bool Well::LoadReqs::operator ==( const LoadReqs& oth ) const
{
    if ( &oth == this )
	return true;

    return reqs_ == oth.reqs_ && lognms_ == oth.lognms_;
}


bool Well::LoadReqs::operator !=( const LoadReqs& oth ) const
{
    return !(*this == oth);
}


Well::LoadReqs Well::LoadReqs::getLoadReqFromFileExt( const char* ext )
{
    if ( StringView(odIO::sExtWell()) == ext )
	return LoadReqs( Inf );
    else if ( StringView(odIO::sExtTrack()) == ext )
	return LoadReqs( Trck );
    else if ( StringView(odIO::sExtLog()) == ext )
	return LoadReqs( LogInfos );
    else if ( StringView(odIO::sExtMarkers()) == ext )
	return LoadReqs( Mrkrs );
    else if ( StringView(odIO::sExtD2T()) == ext )
	return LoadReqs( D2T );
    else if ( StringView(odIO::sExtCSMdl()) == ext )
	return LoadReqs( CSMdl );
    else if ( StringView(odIO::sExtDispProps()) == ext )
	return LoadReqs( DispProps3D );
    else
	return LoadReqs( false );
}


Well::LoadReqs Well::LoadReqs::All()
{
    LoadReqs ret( false );
    ret.reqs_.set();
    if ( !SI().zIsTime() )
	ret.reqs_[D2T] = 0;

    return ret;
}


Well::LoadReqs Well::LoadReqs::AllNoLogs()
{
    LoadReqs ret( false );
    ret.reqs_.set();
    if ( !SI().zIsTime() )
	ret.reqs_[D2T] = 0;

    ret.reqs_[Logs] = 0;
    ret.lognms_.setEmpty();
    return ret;
}


Well::LoadReqs& Well::LoadReqs::add( SubObjType typ )
{
    if ( typ != D2T || SI().zIsTime() )
	reqs_[typ] = 1;
    if ( typ == Logs )
	reqs_[LogInfos] = 1;
    return *this;
}


Well::LoadReqs& Well::LoadReqs::addLog( const char* lognm )
{
    reqs_[LogInfos] = 1;
    lognms_.addIfNew( lognm );
    return *this;
}


Well::LoadReqs& Well::LoadReqs::addLogs( const BufferStringSet& lognms )
{
    reqs_[LogInfos] = 1;
    lognms_.add( lognms, false );
    return *this;
}


Well::LoadReqs& Well::LoadReqs::remove( SubObjType typ )
{
    reqs_[typ]=0;
    if ( typ == LogInfos || typ == Logs )
	lognms_.setEmpty();

    return *this;
}


Well::LoadReqs& Well::LoadReqs::setToAll()
{
    *this = All();
    return *this;
}


Well::LoadReqs& Well::LoadReqs::setEmpty()
{
    reqs_.reset();
    lognms_.setEmpty();
    return *this;
}


Well::LoadReqs& Well::LoadReqs::include( const LoadReqs& oth )
{
    for ( int idx=0; idx<mWellNrSubObjTypes; idx++ )
    {
	if ( oth.reqs_[idx]==1 )
	    reqs_[ idx ] = 1;
    }

    if ( reqs_[Logs]==1 )
	reqs_[LogInfos] = 1;

    lognms_.add( oth.lognms_, false );
    if ( !lognms_.isEmpty() )
	reqs_[LogInfos] = 1;

    return *this;
}


Well::LoadReqs& Well::LoadReqs::exclude( const LoadReqs& oth )
{
    for ( int idx=0; idx<mWellNrSubObjTypes; idx++ )
    {
	if ( oth.reqs_[idx]==1 )
	    reqs_[ idx ] = 0;
    }

    if ( reqs_[Logs]==1 )
	reqs_[LogInfos] = 1;

    for ( const auto* lognm : oth.lognms_ )
	lognms_.remove( lognm->buf() );
    if ( !lognms_.isEmpty() )
	reqs_[LogInfos] = 1;

    return *this;
}


Well::LoadReqs& Well::LoadReqs::excludeLogSel()
{
    lognms_.setEmpty();
    return exclude( LogInfos ).exclude( Logs );
}


Well::LoadReqs& Well::LoadReqs::allowMissingLogs( bool yn )
{
    allowmissinglogs_ = yn;
    return *this;
}


bool Well::LoadReqs::isEmpty() const
{
    return reqs_.none();
}


bool Well::LoadReqs::includes( SubObjType typ ) const
{
    return reqs_[typ];
}


bool Well::LoadReqs::includes( const LoadReqs& oth ) const
{
    for ( int idx=0; idx<mWellNrSubObjTypes; idx++ )
	if ( oth.reqs_[idx]==1 && reqs_[idx]==0 )
	    return false;

    for ( const auto* lognm : oth.lognms_ )
	if ( !lognms_.isPresent(lognm->buf()) )
	    return false;

    return true;
}


BufferString Well::LoadReqs::toString() const
{
    BufferString res;
    BufferString tmp;
    tmp.add( "Inf Trck D2T CSMdl Mrkrs Logs LogInfos DispProps2D DispProps3D" );
    BufferStringSet nms;
    nms.unCat( tmp, " " );
    for ( size_t ib=0; ib<reqs_.size(); ib++ )
    {
	if ( reqs_[ib]==1 )
	{
	    if ( !res.isEmpty() )
		res.addSpace();

	    res.add( nms.get(ib).buf() );
	}
    }

    if ( !lognms_.isEmpty() )
	res.add( "Logs names: " ).add( lognms_.cat("`").buf() );

    return res;
}


Well::Man* Well::Man::mgr_ = nullptr;

Well::Man& Well::MGR()
{
    if ( !::Well::Man::mgr_ )
	::Well::Man::mgr_ = new ::Well::Man;
    return *::Well::Man::mgr_;
}


const UnitOfMeasure* Well::Man::depthstorageunit_ = nullptr;
const UnitOfMeasure* Well::Man::depthdisplayunit_ = nullptr;


Well::Man::Man()
    : wellsAdded(this)
    , wellsRemoved(this)
    , wellgrpid_( IOObjContext::getStdDirData(IOObjContext::WllInf)->groupID() )
{
    mAttachCB( IOM().afterSurveyChange, Man::checkForUndeletedRef );
    mAttachCB( IOM().implUpdated, Man::wellAddedToDB );
    mAttachCB( IOM().entryAdded, Man::wellAddedToDB);
    mAttachCB( IOM().entriesAdded, Man::wellsAddedToDB);
    mAttachCB( IOM().entryRemoved, Man::wellEntryRemovedCB );
    mAttachCB( IOM().entriesRemoved, Man::wellEntriesRemovedCB );
}


Well::Man::~Man()
{
    detachAllNotifiers();
    checkForUndeletedRef( nullptr );
}


void Well::Man::wellAddedToDB( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack( const MultiID&, key, cb );
    if ( key.groupID()!=wellgrpid_ || !IOM().implExists(key) )
	return;

    const IOObj* ioobj = IOM().get( key );
    if ( !ioobj )
	return;

    if ( isLoaded(key) )
    {
	const LoadReqs& curreq = loadState( key );
	reload( key, curreq );
    }

    const TypeSet<MultiID> keys( 1, key );
    wellsAdded.trigger( keys );
}


void Well::Man::wellsAddedToDB( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule())
	return;

    mCBCapsuleUnpack( const TypeSet<MultiID>&, keys, cb );
    TypeSet<MultiID> addedkeys;
    TypeSet<MultiID> loadedkeys;
    TypeSet<LoadReqs> loadedreqs;
    for ( const auto& key : keys )
    {
	if ( key.groupID() != wellgrpid_ )
	    continue;

	const IOObj* ioobj = IOM().get( key );
	if ( !ioobj )
	    continue;

	addedkeys.addIfNew( key );
	if ( isLoaded(key) )
	{
	    const Well::LoadReqs& currreq = loadState( key );
	    if ( loadedkeys.addIfNew(key) )
		loadedreqs += currreq;
	}
    }

    if ( addedkeys.isEmpty() )
	return;

    int idx = 0;
    for ( const auto& lkey : loadedkeys )
    {
	reload( lkey, loadedreqs.get(idx) );
	idx++;
    }

    wellsAdded.trigger( addedkeys );
}


void Well::Man::wellEntryRemovedCB( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule())
	return;

    mCBCapsuleUnpack( const MultiID&, key, cb );
    if ( key.groupID() != wellgrpid_ )
	return;

    const TypeSet<MultiID> keys( 1, key );
    wellsRemoved.trigger( keys );
}


void Well::Man::wellEntriesRemovedCB( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule())
	return;

    mCBCapsuleUnpack( const TypeSet<MultiID>&, keys, cb );
    TypeSet<MultiID> wellkeys;
    for ( const auto& key : keys )
    {
	if ( key.groupID() == wellgrpid_ )
	    wellkeys += key;
    }

    if ( wellkeys.isEmpty() )
	return;

    wellsRemoved.trigger( wellkeys );
}


void Well::Man::cleanupNullPtrs()
{
    wells_.cleanupNullPtrs();
}


void Well::Man::checkForUndeletedRef( CallBacker* )
{
#ifdef __debug__
    for ( int idx=0; idx<wells_.size(); idx++ )
    {
	RefMan<Data> wd = wells_[idx];
	if ( wd )
	{
	    BufferString tmp("Well ", wd->name(), " has ");
	    tmp.add( wd->nrRefs()-1 ).add(" undeleted references" );
	    pErrMsg( tmp );
	}

    }
#endif
    wells_.erase();
    depthstorageunit_ = nullptr;
    depthdisplayunit_ = nullptr;
}


void Well::Man::removeObject( const Data* wd )
{
    auto* wdtmp = const_cast<Data*>( wd );
    const BufferString wdtmpnm = wd->name();
    const int idx = wells_.indexOf( wdtmp );
    if ( idx < 0 )
	return;

    wells_.removeSingle( idx );
}


void Well::Man::removeObject( const MultiID& key )
{
    const int idx = gtByKey( key );
    if ( !wells_.validIdx(idx) )
	return;

    const BufferString wdtmpnm = wells_[idx]->name();
    wells_.removeSingle( idx );
}


RefMan<Well::Data> Well::Man::get( const MultiID& key )
{
    return get( key, LoadReqs::All() );
}


RefMan<Well::Data> Well::Man::get( const MultiID& key, const LoadReqs& reqs )
{
    errmsg_.setEmpty();

    const int wdidx = gtByKey( key );
    RefMan<Data> wd = wdidx < 0 ? nullptr : wells_[wdidx];
    if ( wd && wd->loadState().includes(reqs) )
	return wd;

    LoadReqs lreqs( reqs );
    if ( wdidx >=0 && wd )
    {
	lreqs.exclude( wd->loadState() );
	if ( !readReqData(key,lreqs,*wd) )
	    return nullptr;

	return wd;
    }

    return addNew( key, lreqs );
}


RefMan<Well::Data> Well::Man::get( const DBKey& key, const LoadReqs& reqs )
{
    SurveyDiskLocation sdl;
    if ( key.hasSurveyLocation() )
	sdl = key.surveyDiskLocation();

    SurveyChanger chgr( sdl );
    return get( sCast(const MultiID&,key), reqs );
}


RefMan<Well::Data> Well::Man::addNew( const MultiID& key, LoadReqs reqs )
{
    RefMan<Data> wd = new Data;
    if ( !wd )
	return nullptr;

    if ( !readReqData(key,reqs,*wd) )
	return nullptr;

    wd->setMultiID( key );
    wells_ += wd;

    return wd;
}

Coord Well::Man::getMapLocation( const MultiID& id ) const
{
    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj )
	return Coord::udf();

    RefMan<Data> data = new Data;
    Coord maploc;
    Reader rdr( *ioobj, *data );
    return rdr.getMapLocation(maploc) ? maploc : Coord::udf();
}


bool Well::Man::readReqData( const MultiID& key, const LoadReqs& reqs, Data& wd)
{
    const Reader rdr( key, wd );
    const uiRetVal uirv = rdr.readReqData( reqs );
    if ( uirv.isError() )
	errmsg_ = uirv.messages().cat();

    return uirv.isOK();
}


bool Well::Man::isLoaded( const MultiID& key ) const
{
    return gtByKey( key ) >= 0;
}


Well::LoadReqs Well::Man::loadState( const MultiID& key ) const
{
    if ( !isLoaded(key) )
	return LoadReqs(false);

    const int wdidx = gtByKey( key );
    ConstRefMan<Data> wd = wdidx < 0 ? nullptr : wells_[wdidx];
    if ( !wd )
	return LoadReqs();

    return wd->loadState();
}


bool Well::Man::reload( const MultiID& key, LoadReqs lreqs )
{
    const int wdidx = gtByKey( key );
    if ( wdidx<0 )
	return false;

    RefMan<Data> wd = wells_[wdidx];
    if ( !wd )
	return false;

    if ( lreqs.isEmpty() )
	lreqs = wd->loadState();

    LoadReqs usereqs( lreqs );
    usereqs.excludeLogSel();
    if ( !readReqData(key,usereqs,*wd) )
	return false;

    const BufferStringSet& lognms = lreqs.logNames();
    bool res = true;
    if ( lreqs.includes(Logs) && lognms.isEmpty() )
    {
	res = reloadLogs( key );
    }
    else if ( lreqs.includes(LogInfos) || !lognms.isEmpty() )
    {
	Well::LogSet& logs = wd->logs();
	NotifyStopper nsrm( logs.logRemoved );
	NotifyStopper nsadd( logs.logAdded );
	const LoadReqs logreqs( lognms );
	for ( const auto* nm : lognms )
	{
	    const char* lognm = nm->buf();
	    Well::Log* log = logs.isLoaded( lognm )
			   ? logs.getLog( lognm )
			   : getNonConst( logs.getLogInfos( lognm ) );
	    if ( !log )
		continue;

	    //Empty the container, but keep the name set
	    log->setMnemonicLabel( nullptr );
	    log->setUnitMeasLabel( nullptr );
	    log->dahRange() = Interval<float>::udf();
	    log->setValueRange( Interval<float>::udf() );
	    log->pars().setEmpty();
	    log->setEmpty();
	}

	res = readReqData( key, logreqs, *wd );
	if ( res )
	    wd->logschanged.trigger( -1 );
    }

    if ( res )
	wd->reloaded.trigger();

    return res;
}


void Well::Man::reloadAll()
{
    isreloading_ = true;
    for ( int idx=0; idx<wells_.size(); idx++ )
    {
	RefMan<Data> wd = wells_[idx];
	if ( !wd )
	    continue;

	NotifyStopper nslogschgd( wd->logschanged );
	const MultiID& wid = wd->multiID();
	reload( wid );
    }

    isreloading_ = false;
}


bool Well::Man::reloadDispPars( const MultiID& key, bool for2d )
{
    const int wdidx = gtByKey( key );
    if ( wdidx<0 )
	return false;

    isreloading_ = true;
    const LoadReqs lreqs( for2d ? DispProps2D : DispProps3D );
    RefMan<Data> wd = wells_[wdidx];
    if ( !wd || !readReqData(key,lreqs,*wd) )
	return false;

    isreloading_ = false;
    for2d ? wd->disp2dparschanged.trigger() : wd->disp3dparschanged.trigger();
    return true;
}


bool Well::Man::reloadLogs( const MultiID& key )
{
    const int wdidx = gtByKey( key );
    if ( wdidx<0 )
	return false;

    RefMan<Data> wd = wells_[wdidx];
    if ( !wd )
	return false;

    BufferStringSet lognms, loadedlognms;
    Well::LogSet& logs = wd->logs();
    logs.getNames( loadedlognms, true );
    logs.getNames( lognms );
    if ( lognms.isEmpty() )
	return true;

    isreloading_ = true;
    NotifyStopper nsrm( logs.logRemoved );
    NotifyStopper nsadd( logs.logAdded );
    for ( const auto* nm : lognms )
    {
	const char* lognm = nm->buf();
	Well::Log* log = logs.isLoaded( lognm )
		       ? logs.getLog( lognm )
		       : getNonConst( logs.getLogInfos( lognm ) );
	if ( !log )
	    continue;

	//Empty the container, but keep the name set
	log->setMnemonicLabel( nullptr );
	log->setUnitMeasLabel( nullptr );
	log->dahRange() = Interval<float>::udf();
	log->setValueRange( Interval<float>::udf() );
	log->pars().setEmpty();
	log->setEmpty();
    }

    const LoadReqs lreqs( loadedlognms );
    if ( !readReqData(key,lreqs,*wd) )
	return false;

    isreloading_ = false;
    wd->logschanged.trigger( -1 );
    return true;
}


bool Well::Man::validID( const MultiID& mid ) const
{
    PtrMan<IOObj> ioobj = IOM().get( mid );
    return ioobj ? ioobj->group()==WellTranslatorGroup::sGroupName() : false;
}


int Well::Man::gtByKey( const MultiID& key ) const
{
    for ( int idx=0; idx<wells_.size(); idx++ )
    {
	if ( wells_[idx] && wells_[idx]->multiID() == key )
	    return idx;
    }

    return -1;
}


bool Well::Man::getWellKeys( TypeSet<MultiID>& ids, bool onlyloaded )
{
    bool haserror = false;
    if ( onlyloaded )
    {
	const auto& wells = MGR().wells();
	for ( int idx=0; idx<wells.size(); idx++ )
	{
	    ConstRefMan<Data> wd = wells[idx];
	    if ( wd )
		ids += wd->multiID();
	    else
		haserror = true;
	}
    }
    else
    {
	const IOObjContext& ctxt = WellTranslatorGroup::ioContext();
	const IODir iodir( ctxt.getSelKey() );
	if ( iodir.isBad() )
	    return false;

	const IODirEntryList list( iodir, ctxt );
	for ( int idx=0; idx<list.size(); idx++ )
	{
	    const IOObj* ioobj = list.get(idx)->ioobj_;
	    if ( ioobj )
		ids += ioobj->key();
	}
    }

    return !haserror;
}


bool Well::Man::getWellNames( BufferStringSet& wellnms, bool onlyloaded )
{
    if ( onlyloaded )
    {
	const auto& wells = MGR().wells();
	for ( int idx=0; idx<wells.size(); idx++ )
	{
	    if ( wells[idx] )
		wellnms.add( wells[idx]->name() );
	}
    }
    else
    {
	const IOObjContext& ctxt = WellTranslatorGroup::ioContext();
	const IODir iodir( ctxt.getSelKey() );
	if ( iodir.isBad() )
	    return false;

	const IODirEntryList entries( iodir, ctxt );
	entries.getIOObjNames( wellnms );
    }

    return true;
}


bool Well::Man::getAllMarkerNames( BufferStringSet& nms,
				   const RefObjectSet<const Data>& wds )
{
    ManagedObjectSet<BufferStringSet> wellmarkernames;
    for ( const auto* wd : wds )
    {
	auto* markernames = new BufferStringSet;
	wd->markers().getNames( *markernames );
	if ( markernames->isEmpty() )
	    delete markernames;
	else
	    wellmarkernames.add( markernames );
    }

    if ( wellmarkernames.isEmpty() )
	return true;

    return mergeOrderedStrings( wellmarkernames, nms );
}


bool Well::Man::getAllMarkerNames( BufferStringSet& nms, bool onlyloaded )
{
    TypeSet<MultiID> ids;
    if ( !MGR().getWellKeys(ids,onlyloaded) )
	return false;

    bool haserror = false;
    RefObjectSet<const Data> wds;
    for ( const auto& wid : ids )
    {
	ConstRefMan<Data> wd = MGR().get( wid, LoadReqs(Mrkrs) );
	if ( !wd )
	{
	    haserror = true;
	    continue;
	}

	wds.add( wd.ptr() );
    }

    return getAllMarkerNames( nms, wds ) && !haserror;
}


bool Well::Man::getAllMarkerInfo( BufferStringSet& nms,
				  TypeSet<OD::Color>& cols, bool onlyloaded )
{
    TypeSet<MultiID> ids;
    if ( !MGR().getWellKeys(ids,onlyloaded) )
	return false;

    bool haserror = false;
    for ( const auto& wid : ids )
    {
	ConstRefMan<Data> wd = MGR().get( wid, LoadReqs(Mrkrs) );
	if ( !wd )
	{
	    haserror = true;
	    continue;
	}

	BufferStringSet markernms;
	TypeSet<OD::Color> colors;
	wd->markers().getNames( markernms );
	wd->markers().getColors( colors );
	for ( int im=0; im<markernms.size(); im++ )
	{
	    if ( !nms.isPresent(markernms.get(im)) )
	    {
		nms.add( markernms.get( im ) );
		cols += colors[im];
	    }
	}
    }

    return !haserror;
}


bool Well::Man::deleteLogs( const MultiID& key,
			    const BufferStringSet& logstodel )
{
    const LoadReqs loadreq( Logs );
    RefMan<Data> wd = get( key, loadreq );
    if ( !wd )
	return false;

    LogSet& wls = wd->logs();
    for ( int idl=0; idl<logstodel.size(); idl++ )
    {
	const BufferString& logname = logstodel.get( idl );
	const int logidx = wls.indexOf( logname );
	if ( logidx<0 )
	    continue;

	NotifyStopper ns( wls.logRemoved );
	delete wls.remove( logidx );
    }

    PtrMan<Writer> wwr = new Writer( wd->multiID(), *wd );
    if ( !wwr || !wwr->putLogs() )
    {
	errmsg_ = wwr->errMsg();
	return false;
    }

    wwr.erase();
    wd->logschanged.trigger(-1);
    return true;
}


bool Well::Man::deleteMarkers( const MultiID& key,
			       const BufferStringSet& markerstodel )
{
    const LoadReqs loadreq( Mrkrs );
    RefMan<Data> wd = get( key, loadreq );
    if ( !wd )
	return false;

    MarkerSet& markers = wd->markers();
    for ( const auto* markernm : markerstodel )
    {
	const int idx = markers.indexOf( *markernm );
	if ( idx < 0 )
	    continue;

	NotifyStopper ns( wd->markerschanged );
	delete markers.removeSingle( idx );
    }

    PtrMan<Writer> wwr = new Writer( wd->multiID(), *wd );
    if ( !wwr || !wwr->putMarkers() )
    {
	errmsg_ = wwr->errMsg();
	return false;
    }

    wwr.erase();
    wd->markerschanged.trigger();
    return true;
}


bool Well::Man::getAllLogNames( BufferStringSet& alllognms,
				bool onlyloadedwells )
{
    TypeSet<MultiID> ids;
    if ( !getWellKeys(ids,onlyloadedwells) )
	return false;

    bool haserror = false;
    for ( const auto& wid : ids )
    {
	BufferStringSet lognms;
	if ( !MGR().getLogNamesByID(wid,lognms) )
	{
	    haserror = true;
	    continue;
	}

	alllognms.add( lognms, false );
    }

    return !haserror;
}


bool Well::Man::getAllMnemonics( MnemonicSelection& allmns,
				 bool onlyloadedwells )
{
    TypeSet<MultiID> ids;
    if ( !getWellKeys(ids,onlyloadedwells) )
	return false;

    bool haserror = false;
    const LoadReqs lreqs( LogInfos );
    for ( const auto& wid : ids )
    {
	ConstRefMan<Data> wd = MGR().get( wid, lreqs );
	if ( !wd )
	{
	    haserror = true;
	    continue;
	}

	MnemonicSelection mns;
	wd->logs().getAllAvailMnems( mns );
	for ( const auto* mn : mns )
	    allmns.addIfNew( mn );
    }

    return !haserror;
}


bool Well::Man::renameLog( const TypeSet<MultiID>& keys, const char* oldnm,
							 const char* newnm )
{
    const LoadReqs lreqs( LogInfos );
    bool haserror = false;
    for ( const auto& key : keys )
    {
	RefMan<Data> wd = MGR().get( key, lreqs );
	if ( !wd )
	{
	    haserror = true;
	    continue;
	}

	Writer wwr( wd->multiID(), *wd );
	if ( !wwr.renameLog(oldnm,newnm) )
	{
	    haserror = true;
	    continue;
	}

	wd->logs().renameDefaultLog( oldnm, newnm );
	if ( !wwr.putDefLogs() )
	{
	    Log* log = wd->logs().getLog( newnm );
	    if ( log )
		wd->logs().removeDefault( *log->mnemonic() );
	}
    }

    return !haserror;
}


bool Well::Man::getMarkersByID( const MultiID& mid, BufferStringSet& nms )
{
    ConstRefMan<Data> wd = MGR().get( mid, LoadReqs(Mrkrs) );
    if ( !wd )
	return false;

    wd->markers().getNames( nms );
    return true;
}


bool Well::Man::getMarkersByID( const MultiID& mid, BufferStringSet& nms,
				TypeSet<OD::Color>& cols )
{
    ConstRefMan<Data> wd = MGR().get( mid, LoadReqs(Mrkrs) );
    if ( !wd )
	return false;

    wd->markers().getNames( nms );
    wd->markers().getColors( cols );
    return true;
}


bool Well::Man::getMarkersByID( const MultiID& mid, BufferStringSet& nms,
				TypeSet<OD::Color>& cols, TypeSet<float>& zs )
{
    ConstRefMan<Data> wd = MGR().get( mid, LoadReqs(Mrkrs) );
    if ( !wd )
	return false;

    wd->markers().getNamesColorsMDs( nms, cols, zs );
    return true;
}


bool Well::Man::getLogNamesByID( const MultiID& ky, BufferStringSet& nms )
{
    ConstRefMan<Data> wd = MGR().get( ky, LoadReqs(LogInfos) );
    if ( !wd )
	return false;

    wd->logs().getNames( nms );
    return true;
}


void Well::Man::getLogIDs( const MultiID& ky, const BufferStringSet& lognms,
			   TypeSet<int>& ids )
{
    BufferStringSet alllognms;
    getLogNamesByID( ky, alllognms );
    for ( const auto* lognm : lognms )
    {
	const int lidx = alllognms.indexOf( lognm->buf() );
	if ( lidx!=-1 )
	    ids += lidx;
    }
}


void Well::Man::getLogIDs( const MultiID& ky, const MnemonicSelection& mns,
			   TypeSet<int>& ids )
{
    ConstRefMan<Data> wd = MGR().get( ky, LoadReqs(LogInfos) );
    if ( !wd )
	return;

    for ( const auto* mn : mns )
    {
	const Log* log = wd->logs().getLog( *mn );
	if ( log )
	    ids += wd->logs().indexOf( log->name() );
    }
}


bool Well::Man::getLogNames( const MultiID& ky, BufferStringSet& nms,
			     bool forceload )
{
    const bool isloaded = MGR().isLoaded( ky );
    if ( isloaded && forceload && !MGR().reload(ky,LoadReqs(LogInfos)) )
	return false;

    return getLogNamesByID( ky, nms );
}


bool Well::Man::getMarkerNames( BufferStringSet& nms )
{
    return MGR().getAllMarkerNames( nms );
}


const UnitOfMeasure* Well::Man::surveyDepthStorageUnit()
{
    if ( !depthstorageunit_ )
	depthstorageunit_ = UnitOfMeasure::surveyDefDepthStorageUnit();

    return depthstorageunit_;
}


const UnitOfMeasure* Well::Man::surveyDepthDisplayUnit()
{
    if ( !depthdisplayunit_ )
	depthdisplayunit_ = UnitOfMeasure::surveyDefDepthUnit();

    return depthdisplayunit_;
}


IOObj* Well::findIOObj( const char* nm, const char* uwi )
{
    const IODir iodir( IOObjContext::getStdDirData(IOObjContext::WllInf)->id_ );
    if ( nm && *nm )
    {
	const IOObj* ioobj = iodir.get( nm, "Well" );
	if ( ioobj )
	    return ioobj->clone();
    }

    if ( uwi && *uwi )
    {
	const IOObjContext ctxt = mIOObjContext( Well );
	const IODirEntryList del( iodir, ctxt );
	RefMan<Data> data = new Data;
	for ( int idx=0; idx<del.size(); idx++ )
	{
	    const IOObj* ioobj = del[idx]->ioobj_;
	    if ( !ioobj )
		continue;

	    Reader wr( *ioobj, *data );
	    if ( wr.getInfo() && data->info().uwid_ == uwi )
		return ioobj->clone();
	}
    }

    return nullptr;
}


uiRetVal Well::Man::writeLogHeaders( const MultiID& key,
				     const BufferStringSet& lognms )
{
    RefMan<Data> wd = get( key, LoadReqs(LogInfos) );
    if ( !wd )
	return errmsg_;

    Writer wtr( key, *wd );
    const LogSet& logs = wd->logs();
    uiRetVal uirv;
    for ( const auto* lognm : lognms )
    {
	const Well::Log* log = logs.getLogInfos( lognm->buf() );
	if ( !log )
	{
	    uirv.add( tr("Cannot find header information for the log: '%1'")
			.arg(lognm->buf()) );
	    continue;
	}

	if ( !wtr.putLog(*log) )
	    uirv.add( wtr.errMsg() );
    }

    return uirv;
}


bool Well::Man::writeAndRegister( const MultiID& key, PtrMan<Well::Log>& log )
{
    if ( !log )
	return false;

    RefMan<Data> wd = get( key, LoadReqs(LogInfos) );
    if ( !wd )
	return false;

    const BufferString newlognm = log->name();
    LogSet& currlogset = wd->logs();
    Log currlogcopy;
    bool logadded = false;
    if ( currlogset.isPresent(newlognm) )
    {
	Log& currlog = *currlogset.getLog( newlognm.buf() );
	currlogcopy = currlog;
	currlog = *log;
	log = nullptr;
    }
    else
    {
	NotifyStopper ns( currlogset.logAdded );
	currlogset.add( log.release() );
	logadded = true;
    }

    NotifyStopper fswns( FSW().directoryChanged );
    Writer wtr( key, *wd );
    if ( !wtr.putLog(*currlogset.getLog(newlognm.buf())) )
    {
	if ( currlogcopy.isLoaded() )
	    *currlogset.getLog(newlognm.buf()) = currlogcopy;
	else if ( logadded )
	{
	    NotifyStopper rmns( currlogset.logRemoved );
	    const int idx = currlogset.indexOf( newlognm );
	    delete currlogset.remove( idx );
	}

	errmsg_ = wtr.errMsg();
	return false;
    }

    const int newlogidx = currlogset.indexOf( newlognm );
    wd->logschanged.trigger( newlogidx );
    return true;
}


bool Well::Man::writeAndRegister( const MultiID& key, ObjectSet<Log>& logset )
{
    bool res = true;
    ObjectSet<PtrMan<Log>> manlogset;
    while ( !logset.isEmpty() )
	manlogset.add( new PtrMan<Log>(logset.pop()) );

    for ( int idx=0; idx<manlogset.size(); idx++ )
    {
	if ( !manlogset.get(idx)->ptr() )
	    continue;

	if ( !writeAndRegister(key,*manlogset.get(idx)) )
	{
	    res = false;
	    continue;
	}
    }

    deepErase( manlogset );

    return res;
}


const Mnemonic* Well::Man::getMnemonicOfLog( const char* lognm ) const
{
    for ( int idx=0; idx<wells_.size(); idx++ )
    {
	ConstRefMan<Data> wd = wells_[idx];
	if ( !wd.ptr() )
	    continue;

	const Mnemonic* mn = wd->logs().getMnemonicOfLog( lognm );
	if ( mn )
	    return mn;
    }

    return nullptr;
}


void Well::Man::dumpMgrInfo( StringPairSet& infoset )
{
    WeakPtrSet<Data>& wells = MGR().wells();
    infoset.add( "Number of Wells", wells.size() );
    for ( int idx=0; idx<wells.size(); idx++ )
    {
	ConstRefMan<Data> wd = wells[idx];
	if ( wd )
	{
	    const OD::String& wellname = wd->info().name();
	    infoset.add( IOPar::compKey(wellname,"References"),
			 wd->nrRefs()-1 );
	    infoset.add( IOPar::compKey(wellname,"Markers"),
			 wd->markers().size() );
	    infoset.add( IOPar::compKey(wellname,"Load State"),
			 wd->loadState().toString() );
	    const LogSet& logs = wd->logs();
	    infoset.add( IOPar::compKey(wellname,"Logs with available info"),
				      logs.size() );
	    infoset.add( IOPar::compKey(wellname,"Logs loaded"),
			 logs.nrLoaded() );
	}
    }
}


const BufferString Well::Man::wellDirPath()
{
    const FilePath welldir( IOM().rootDir().fullPath(), sWellSubDir() );
    return welldir.fullPath();
}


bool Well::Man::isReloading() const
{
    return isreloading_;
}


float Well::displayToStorageDepth( float zval )
{
    const UnitOfMeasure* storunit = Man::surveyDepthStorageUnit();
    const UnitOfMeasure* dispunit = Man::surveyDepthDisplayUnit();
    return getConvertedValue( zval, dispunit, storunit );
}


float Well::storageToDisplayDepth( float zval )
{
    const UnitOfMeasure* storunit = Man::surveyDepthStorageUnit();
    const UnitOfMeasure* dispunit = Man::surveyDepthDisplayUnit();
    return getConvertedValue( zval, storunit, dispunit );
}


bool Well::putUWI( const MultiID& key, const char* uwi )
{
    if ( key.isUdf() )
	return false;

    PtrMan<IOObj> ioobj = IOM().get( key );
    if ( !ioobj )
	return false;

    ioobj->pars().set( Info::sKeyUwid(), uwi );
    return IOM().commitChanges( *ioobj );
}


bool Well::putUWIs( const ObjectSet<std::pair<const MultiID,
    const char*>>& uwiset )
{
    if ( uwiset.isEmpty() )
	return true;

    ObjectSet<const IOObj> ioobjs;
    for ( const auto* pair : uwiset )
    {
	const MultiID& key = pair->first;
	if ( key.isUdf() )
	    continue;

	IOObj* ioobj = IOM().get( key );
	const StringView uwi( pair->second );
	if ( !ioobj )
	    continue;

	ioobj->pars().set( Info::sKeyUwid(), uwi );
	ioobjs += ioobj;
    }

    return IOM().commitChanges( ioobjs );
}
