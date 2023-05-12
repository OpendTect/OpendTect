/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellman.h"

#include "file.h"
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
#include "welllog.h"
#include "welllogset.h"
#include "wellmarker.h"
#include "wellreader.h"
#include "welltrack.h"
#include "welltransl.h"
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


Well::LoadReqs::~LoadReqs()
{
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


Well::LoadReqs& Well::LoadReqs::remove( SubObjType typ )
{
    reqs_[typ]=0;
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
    return true;
}


BufferString Well::LoadReqs::toString() const
{
    BufferString res;
    BufferString tmp;
    tmp.add( "Inf Trck D2T CSMdl Mrkrs Logs LogInfos DispProps2D DispProps3D" );
    BufferStringSet nms;
    nms.unCat( tmp, " " );
    for ( int ib=0; ib<reqs_.size(); ib++ )
    {
	if ( reqs_[ib]==1 )
	    res.add( nms.get(ib) ).addSpace();
    }
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
{
    addFileSystemWatchCB( nullptr );
    getWellKeys( allwellsids_ );
    mAttachCB( IOM().afterSurveyChange, Man::checkForUndeletedRef );
}


Well::Man::~Man()
{
    detachAllNotifiers();
    checkForUndeletedRef( nullptr );
}


void Well::Man::addFileSystemWatchCB( CallBacker* cb )
{
    mEnsureExecutedInMainThread( Man::addFileSystemWatchCB );
    const FilePath welldir( wellDirPath() );
    const FilePath wellomffp( welldir, ".omf" );
    FSW().addPath(welldir.fullPath());
    FSW().addFile(wellomffp.fullPath());
    mAttachCB( FSW().fileChanged, Man::wellFilesChangedCB );
    mAttachCB( FSW().directoryChanged, Man::wellDirChangedCB );
}


void Well::Man::wellFilesChangedCB( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack( BufferString, fpstr, cb );
    const FilePath fp( fpstr );
    const FilePath wellomffp( wellDirPath(), ".omf" );
    if ( fp != wellomffp )
	return;

    TypeSet<MultiID> initwellids, currwellids;
    initwellids = allwellsids_;
    getWellKeys( currwellids );
    allwellsids_ = currwellids;
    if ( initwellids.size() > currwellids.size() )
    {
	initwellids.createDifference( currwellids );
	for ( const auto& id : initwellids )
	{
	    if ( !isLoaded(id) )
		continue;

	    removeObject( id );
	}
    }
    else if ( currwellids.size() > initwellids.size() )
    {
	currwellids.createDifference( initwellids );
	for ( const auto& id : currwellids )
	    get( id );
    }
    else
	reloadAll();
}


void Well::Man::wellDirChangedCB( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack( BufferString, fpstr, cb );
    const FilePath fp( fpstr );
    const FilePath welldirfp( wellDirPath() );
    if ( fp != welldirfp )
	return;

    reloadAll();
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


void Well::Man::removeObject( const Well::Data* wd )
{
    auto* wdtmp = const_cast<Well::Data*>( wd );
    const int idx = wells_.indexOf( wdtmp );
    if ( idx < 0 ) return;

    wells_.removeSingle( idx );
}


void Well::Man::removeObject( const MultiID& key )
{
    const int idx = gtByKey( key );
    if ( !wells_.validIdx(idx) )
	return;

    wells_.removeSingle( idx );
}


RefMan<Well::Data> Well::Man::get( const MultiID& key )
{
    return get( key, LoadReqs() );
}


RefMan<Well::Data> Well::Man::get( const MultiID& key, LoadReqs reqs )
{
    msg_.setEmpty();

    const int wdidx = gtByKey( key );
    RefMan<Data> wd = wdidx < 0 ? nullptr : wells_[wdidx];
    if ( wd && wd->loadState().includes(reqs) )
        return wd;

    if ( wdidx >=0 && wd )
    {
	reqs.exclude( wd->loadState() );
	if ( !readReqData(key,*wd,reqs) )
	    return nullptr;

	return wd;
    }

    return addNew( key, reqs );
}


RefMan<Well::Data> Well::Man::get( const DBKey& key, LoadReqs reqs )
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
    if ( readReqData(key, *wd, reqs) )
    {
	wd->setMultiID( key );
	wells_ += wd;
    }
    else
	wd.erase();

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


bool Well::Man::readReqData( const MultiID& key, Data& wd,
			     LoadReqs reqs )
{
    Reader rdr( key, wd );
    if ( reqs.includes(Inf) && !rdr.getInfo() )
    { msg_ = rdr.errMsg(); return false; }

    if ( reqs.includes(Trck) && !rdr.getTrack() )
    { msg_ = rdr.errMsg(); return false; }

    if ( reqs.includes(D2T) )
	rdr.getD2T();
    if ( reqs.includes(Mrkrs) )
	rdr.getMarkers();
    if ( reqs.includes(Logs) )
	rdr.getLogs();
    else if ( reqs.includes(LogInfos) )
	rdr.getLogs( true );
    if ( reqs.includes(CSMdl) )
	rdr.getCSMdl();
    if ( reqs.includes(DispProps2D) || reqs.includes(DispProps3D) )
	rdr.getDispProps();

    return true;
}


bool Well::Man::isLoaded( const MultiID& key ) const
{
    return gtByKey( key ) >= 0;
}


bool Well::Man::reload( const MultiID& key, LoadReqs lreqs )
{
    const int wdidx = gtByKey( key );
    if ( wdidx<0 ) return false;

    RefMan<Data> wd = wells_[wdidx];
    if ( lreqs.isEmpty() )
	lreqs = wd->loadState();
    LoadReqs usereqs( lreqs );
    usereqs.exclude( LoadReqs( Logs, LogInfos ) );
    if ( !readReqData(key,*wd,usereqs) )
	return false;

    if ( lreqs.includes(Logs) )
	reloadLogs( key );
    else if ( lreqs.includes(LogInfos) )
    {
	NotifyStopper ns( wd->logs().logAdded );
	readReqData( key, *wd, LoadReqs(LogInfos) );
	wd->logschanged.trigger( -1 );
    }

    wd->reloaded.trigger();
    return true;
}


void Well::Man::reloadAll()
{
    isreloading_ = true;
    for ( int idx=0; idx<wells_.size(); idx++ )
    {
	RefMan<Data> wd = wells_[idx];
	NotifyStopper nslogschgd( wd->logschanged );
	const MultiID& wid = wd->multiID();
	reload( wid );
    }

    isreloading_ = false;
}


bool Well::Man::reloadDispPars( const MultiID& key, bool for2d )
{
    const int wdidx = gtByKey( key );
    if ( wdidx<0 ) return false;

    const LoadReqs lreqs(for2d ? DispProps2D : DispProps3D);
    RefMan<Data> wd = wells_[wdidx];
    if ( !readReqData(key,*wd,lreqs) )
	return false;

    for2d ? wd->disp2dparschanged.trigger() : wd->disp3dparschanged.trigger();
    return true;
}


bool Well::Man::reloadLogs( const MultiID& key )
{
    const int wdidx = gtByKey( key );
    if ( wdidx<0 )
	return false;
    BufferStringSet loadedlogs;
    RefMan<Data> wd = wells_[wdidx];
    wd->logs().getNames( loadedlogs, true );
    NotifyStopper ns( wd->logs().logAdded );
    if ( !readReqData(key,*wd,LoadReqs(LogInfos)) )
	return false;

    bool res = true;
    for ( auto* loadedlog : loadedlogs )
	if ( !wd->getLog(*loadedlog) )
	    res = false;

    wd->logschanged.trigger( -1 );
    return res;
}


bool Well::Man::validID( const MultiID& mid ) const
{
    const IOObj* ioobj = IOM().get( mid );
    return ioobj ? ioobj->group()==mTranslGroupName(Well) : false;
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
    if ( !onlyloaded )
    {
	const IOObjContext ctxt = mIOObjContext(Well);
	const IODir iodir( ctxt.getSelKey() );
	const IODirEntryList list( iodir, ctxt );
	for ( int idx=0; idx<list.size(); idx++ )
	{
	    const IOObj* ioobj = list.get(idx)->ioobj_;
	    if ( ioobj )
		ids += ioobj->key();
	}
    }
    else
    {
	const auto& wells = MGR().wells();
	for ( int idx=0; idx<wells.size(); idx++ )
	{
	    if ( wells[idx] )
		ids += wells[idx]->multiID();
	}
    }

    return !ids.isEmpty();
}


bool Well::Man::getWellNames( BufferStringSet& wellnms, bool onlyloaded )
{
    TypeSet<MultiID> ids;
    getWellKeys( ids );
    for ( int idx=0; idx<ids.size(); idx++ )
	wellnms.add( IOM().nameOf(ids[idx]) );

    return !wellnms.isEmpty();
}


bool Well::Man::getAllMarkerNames( BufferStringSet& nms,
				   const RefObjectSet<const Data>& wds )
{
    nms.setEmpty();
    ManagedObjectSet<BufferStringSet> wellmarkernames;
    for ( auto* wd : wds )
    {
	auto* markernames = new BufferStringSet;
	wd->markers().getNames( *markernames );
	if ( markernames->isEmpty() )
	    delete markernames;
	else
	    wellmarkernames.add( markernames );
    }

    if ( wellmarkernames.isEmpty() )
	return false;

    return mergeOrderedStrings( wellmarkernames, nms );
}


bool Well::Man::getAllMarkerNames( BufferStringSet& nms, bool onlyloaded )
{
    TypeSet<MultiID> ids;
    MGR().getWellKeys( ids, onlyloaded );
    RefObjectSet<const Data> wds;
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	ConstRefMan<Data> wd = MGR().get( ids[idx], LoadReqs(Mrkrs) );
	if ( wd )
	    wds.add( wd.ptr() );
    }

    return getAllMarkerNames( nms, wds );
}


bool Well::Man::getAllMarkerInfo( BufferStringSet& nms,
				    TypeSet<OD::Color>& cols, bool onlyloaded )
{
    nms.setEmpty();
    cols.setEmpty();
    TypeSet<MultiID> ids;
    MGR().getWellKeys( ids, onlyloaded );
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	ConstRefMan<Data> wd = MGR().get( ids[idx], LoadReqs(Mrkrs) );
	BufferStringSet markernms;
	TypeSet<OD::Color> colors;
	wd->markers().getNames( markernms );
	wd->markers().getColors( colors );
	for ( int im=0; im<markernms.size(); im++ )
	{
	    if ( !nms.isPresent( markernms.get( im ) ) )
	    {
		nms.add( markernms.get( im ) );
		cols += colors[im];
	    }
	}
    }
    return !nms.isEmpty();
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

    Writer wwr( wd->multiID(), *wd );
    if ( !wwr.putLogs() )
    {
	msg_ = wwr.errMsg();
	return false;
    }

    wd->logschanged.trigger(-1);
    return true;
}


bool Well::Man::getAllLogNames( BufferStringSet& lognms, bool onlyloaded )
{
    lognms.setEmpty();
    TypeSet<MultiID> ids;
    getWellKeys( ids, onlyloaded );
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	BufferStringSet logs;
	MGR().getLogNamesByID( ids[idx], logs, onlyloaded );
	lognms.add( logs, false );
    }
    return !lognms.isEmpty();
}


bool Well::Man::getAllMnemonics( MnemonicSelection& mns, bool onlyloaded )
{
    mns.setEmpty();
    TypeSet<MultiID> ids;
    getWellKeys( ids, onlyloaded );
    for ( const auto& wid : ids )
    {
	RefMan<Data> wd = MGR().get( wid, LoadReqs(LogInfos) );
	MnemonicSelection tmpmns;
	wd->logs().getAllAvailMnems( tmpmns );
	for ( const auto* mn : tmpmns )
	    mns.addIfNew( mn );
    }
    return !mns.isEmpty();
}


bool Well::Man::renameLog( const TypeSet<MultiID>& keys, const char* oldnm,
							 const char* newnm )
{
    if ( keys.isEmpty() )
	return false;

    for ( int idx=0; idx<keys.size(); idx++ )
    {
	const MultiID& key = keys.get( idx );
	RefMan<Data> wd = MGR().get( key, LoadReqs(LogInfos) );
	Writer wwr( wd->multiID(), *wd );
	if ( !wwr.renameLog(oldnm,newnm) )
	    return false;

	wd->logs().renameDefaultLog( oldnm, newnm );
	if ( !wwr.putDefLogs() )
	{
	    Well::Log* log = wd->logs().getLog( newnm );
	    wd->logs().removeDefault( *log->mnemonic() );
	}
    }

    return true;
}


bool Well::Man::getMarkersByID( const MultiID& mid, BufferStringSet& nms )
{
    nms.setEmpty();
    if ( MGR().validID( mid ) )
    {
	ConstRefMan<Data> wd = MGR().get( mid, LoadReqs(Mrkrs) );
	if ( wd )
	    wd->markers().getNames( nms );
    }
    return !nms.isEmpty();
}


bool Well::Man::getMarkersByID( const MultiID& mid, BufferStringSet& nms,
				TypeSet<OD::Color>& cols )
{
    nms.setEmpty();
    cols.setEmpty();
    if ( MGR().validID( mid ) )
    {
	ConstRefMan<Data> wd = MGR().get( mid, LoadReqs(Mrkrs) );
	if ( wd )
	{
	    wd->markers().getNames( nms );
	    wd->markers().getColors( cols );
	}
    }
    return nms.isEmpty();
}


bool Well::Man::getMarkersByID( const MultiID& mid, BufferStringSet& nms,
				TypeSet<OD::Color>& cols, TypeSet<float>& zs )
{
    nms.setEmpty();
    cols.setEmpty();
    zs.setEmpty();
    if ( MGR().validID( mid ) )
    {
	ConstRefMan<Data> wd = MGR().get( mid, LoadReqs(Mrkrs) );
	if ( wd )
	    wd->markers().getNamesColorsMDs( nms, cols, zs );
    }
    return !nms.isEmpty();
}


bool Well::Man::getLogNamesByID( const MultiID& ky, BufferStringSet& nms,
				 bool onlyloaded )
{
    nms.setEmpty();
    if ( !MGR().validID( ky ) )
	return false;

    const int idx = MGR().gtByKey( ky );
    const bool isloaded = idx>=0;
    if ( !onlyloaded )
    {
	RefMan<Data> wd;
	if ( isloaded )
	    wd = MGR().wells()[idx];
	else
	    wd = MGR().addNew( ky, LoadReqs(Inf) );

	if ( wd )
	{
	    Reader rdr( ky, *wd );
	    rdr.getLogInfo( nms );
	}
    }
    else if ( isloaded )
	MGR().wells()[idx]->logs().getNames( nms );

    return !nms.isEmpty();
}


void Well::Man::getLogIDs( const MultiID& ky, const BufferStringSet& lognms,
			   TypeSet<int>& ids)
{
    ids.setEmpty();
    BufferStringSet all_lognms;
    getLogNamesByID( ky, all_lognms, false );
    for ( const auto* lognm : lognms )
    {
	const int lidx = all_lognms.indexOf( lognm->buf() );
	if ( lidx!=-1 )
	    ids += lidx;
    }
}


void Well::Man::getLogIDs( const MultiID& ky, const MnemonicSelection& mns,
			   TypeSet<int>& ids)
{
    if ( !MGR().validID( ky ) )
	return;

    ConstRefMan<Data> wd = MGR().get( ky, LoadReqs(LogInfos) );
    if( !wd )
	return;

    for ( const auto* mn : mns )
    {
	const Log* log = wd->logs().getLog( *mn );
	if ( log )
	    ids += wd->logs().indexOf( log->name() );
    }
}


bool Well::Man::getLogNames( const MultiID& ky, BufferStringSet& nms,
			     bool forceLoad )
{
    nms.setEmpty();
    if ( MGR().isLoaded(ky) && forceLoad )
    {
	RefMan<Data> wd = MGR().get( ky, LoadReqs(LogInfos) );
	if ( !wd )
	    return false;

	wd->logs().getNames( nms );
    }
    else if ( MGR().isLoaded(ky) )
    {
	RefMan<Data> wd = MGR().get( ky );
	if ( !wd )
	    return false;

	wd->logs().getNames( nms );
    }
    else
    {
	RefMan<Data> wd = new Well::Data;
	Well::Reader wr( ky, *wd );
	wr.getLogInfo( nms );
	if ( nms.isEmpty() )
	    return wr.getInfo(); // returning whether the well exists
    }

    return true;
}


bool Well::Man::getMarkerNames( BufferStringSet& nms )
{
    nms.setEmpty();
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
	PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj( Well );
	const IODirEntryList del( iodir, ctio->ctxt_ );
	RefMan<Well::Data> data = new Well::Data;
	for ( int idx=0; idx<del.size(); idx++ )
	{
	    const IOObj* ioobj = del[idx]->ioobj_;
	    if ( !ioobj )
		continue;

	    Well::Reader wr( *ioobj, *data );
	    if ( wr.getInfo() && data->info().uwid_ == uwi )
		return ioobj->clone();
	}
    }

    return 0;
}


bool Well::Man::writeAndRegister( const MultiID& key ,
						    PtrMan<Well::Log>& log )
{
    if ( !log )
	return false;

    RefMan<Well::Data> wd = get( key, Well::LoadReqs( Well::LogInfos ) );
    if ( !wd )
	return false;

    const BufferString newlognm = log->name();
    Well::LogSet& currlogset = wd->logs();
    Well::Log currlogcopy;
    bool logadded = false;
    if ( currlogset.isPresent(newlognm) )
    {
	Well::Log& currlog = *currlogset.getLog( newlognm.buf() );
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
    Well::Writer wtr( key, *wd );
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

	msg_ = wtr.errMsg();
	return false;
    }

    const int newlogidx = currlogset.indexOf( newlognm );
    wd->logschanged.trigger( newlogidx );
    return true;
}


bool Well::Man::writeAndRegister( const MultiID& key ,
				  ObjectSet<Well::Log>& logset )
{
    bool res = true;
    ObjectSet<PtrMan<Well::Log>> manlogset;
    while ( !logset.isEmpty() )
	manlogset.add( new PtrMan<Well::Log>(logset.pop()) );

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
    auto& wells = MGR().wells();
    infoset.add( "Number of Wells", wells.size() );
    for ( int idx=0; idx<wells.size(); idx++ )
    {
	ConstRefMan<Data> wd = wells[idx];
	if ( wd )
	{
	    const BufferString wellname = wd->info().name();
	    infoset.add( IOPar::compKey(wellname,"References"),
			 wd->nrRefs()-1 );
	    infoset.add( IOPar::compKey(wellname,"Load State"),
			 wd->loadState().toString() );
	    infoset.add( IOPar::compKey(wellname,"Markers"),
			 wd->markers().size() );
	    const LogSet& ls = wd->logs();
	    int nlogswithdata = 0;
	    for (int il=0; il<ls.size(); il++)
	    {
		const Log& log = ls.getLog( il );
		if ( log.isLoaded() )
		    nlogswithdata++;
	    }
	    infoset.add( IOPar::compKey(wellname,"Logs available"), ls.size() );
	    infoset.add( IOPar::compKey(wellname,"Logs with Info"), ls.size() );
	    infoset.add( IOPar::compKey(wellname,"Logs with data"),
			 nlogswithdata );
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
    const UnitOfMeasure* storunit = Well::Man::surveyDepthStorageUnit();
    const UnitOfMeasure* dispunit = Well::Man::surveyDepthDisplayUnit();
    return getConvertedValue( zval, dispunit, storunit );
}


float Well::storageToDisplayDepth( float zval )
{
    const UnitOfMeasure* storunit = Well::Man::surveyDepthStorageUnit();
    const UnitOfMeasure* dispunit = Well::Man::surveyDepthDisplayUnit();
    return getConvertedValue( zval, storunit, dispunit );
}
