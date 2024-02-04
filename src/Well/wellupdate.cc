/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellupdate.h"

#include "bufstringset.h"
#include "ctxtioobj.h"
#include "file.h"
#include "filepath.h"
#include "threadlock.h"
#include "timer.h"
#include "welldata.h"
#include "wellman.h"

#include <QHashIterator>

WellUpdateQueue::WellUpdateQueue()
    : CallBacker()
    , canupdate( this )
    , wellgrpid_( IOObjContext::getStdDirData(IOObjContext::WllInf)->groupID() )
{
    addTimerCB( nullptr );
}


WellUpdateQueue::~WellUpdateQueue()
{
    detachAllNotifiers();
    updobjidqueue_.erase();
    updreqsqueue_.erase();
    delete timer_;
}


WellUpdateQueue& WellUpdateQueue::WUQ()
{
    mDefineStaticLocalObject( PtrMan<WellUpdateQueue>, wuq,
	(new WellUpdateQueue) );
    return *wuq;
}


void WellUpdateQueue::enqueue( const std::pair<MultiID,
			       Well::LoadReqs>& updateobj )
{
    Threads::Locker locker( lock_ );
    const int objid = updateobj.first.objectID();
    const Well::LoadReqs req = updateobj.second;
    if ( delobjidqueue_.isPresent(objid) )
	return;

    if ( !updobjidqueue_.isPresent(objid) )
    {
	if ( updobjidqueue_.push(objid) )
	    updreqsqueue_.push( req );

	timer_->start( 1000 );
	return;
    }

    const int idx = updobjidqueue_.indexOf(objid);
    updreqsqueue_[idx].include( updateobj.second );
    timer_->start( 1000 );
}


void WellUpdateQueue::enqueueWellsToBeDeleted( const MultiID& key )
{
    Threads::Locker locker( lock_ );
    const int objid = key.objectID();
    if ( !delobjidqueue_.isPresent(objid) )
	delobjidqueue_.push( objid );

    timer_->start( 1000 );
}


bool WellUpdateQueue::dequeue( std::pair<MultiID, Well::LoadReqs>& dqeditem )
{
    if ( isEmpty() )
	return false;

    Threads::Locker locker( lock_ );
    dqeditem.first = MultiID( wellgrpid_, updobjidqueue_.first() );
    dqeditem.second = updreqsqueue_.first();
    updobjidqueue_.removeSingle( 0 );
    updreqsqueue_.removeSingle( 0 );
    if ( isEmpty() )
	timer_->stop();

    return true;
}


bool WellUpdateQueue::dequeueWellsToBeDeleted( MultiID& id )
{
    if ( !hasWellsToBeDeleted() )
	return false;

    Threads::Locker locker( lock_ );
    id = MultiID( wellgrpid_, delobjidqueue_.first() );
    delobjidqueue_.removeSingle( 0 );
    if ( !isEmpty() )
	timer_->stop();

    return true;
}


int WellUpdateQueue::size() const
{
    return updobjidqueue_.size();
}


bool WellUpdateQueue::isEmpty() const
{
    return updobjidqueue_.isEmpty() && delobjidqueue_.isEmpty();
}


bool WellUpdateQueue::hasWellsToBeDeleted() const
{
    return !delobjidqueue_.isEmpty();
}


void WellUpdateQueue::checkUpdateStatus( CallBacker* )
{
    Threads::Locker locker( lock_ );
    if ( isEmpty() )
	return;

    locker.unlockNow();
    canupdate.trigger();
}


void WellUpdateQueue::addTimerCB( CallBacker* )
{
    mEnsureExecutedInMainThread( WellUpdateQueue::addTimerCB );
    timer_ = new Timer( "WellUpdateQueue timer" );
    mAttachCB( timer_->tick, WellUpdateQueue::checkUpdateStatus );
}

// !> ---- WellFileList ----


WellFileList::WellFileList()
    : triggerupdate( this )
{
    BufferStringSet filenmsfp;
    const BufferString welldir = Well::Man::wellDirPath();
    File::makeRecursiveFileList( welldir, filenmsfp );
    for ( const auto* nmfp : filenmsfp )
    {
	FilePath fullpath( *nmfp );
	QString filenm = fullpath.fileName().buf();
	QString lastmodified = File::timeLastModified( *nmfp );
	filelist_[filenm] = lastmodified;
    }

    TypeSet<MultiID> ids;
    Well::Man::getWellKeys( ids );
    for ( const auto& id : ids )
    {
	QString keystr = id.toString().str();
	allidsnmpair_[keystr] = "";
    }
}


WellFileList::~WellFileList()
{
    detachAllNotifiers();
}


WellFileList& WellFileList::operator =( const WellFileList& oth )
{
    Threads::Locker locker( lock_ );
    filelist_ = oth.fileList();
    return *this;
}


void WellFileList::addLoadedWells( const BufferString& wellnm,
				   const MultiID& key )
{
    Threads::Locker locker( lock_ );
    QString nmstr = wellnm.str();
    QString idstr = key.toString().str();
    loadednmidpair_[nmstr] = idstr;
    allidsnmpair_[idstr] = nmstr;
}


void WellFileList::removeFromLoadedWells( const BufferString& wellnm )
{
    Threads::Locker locker( lock_ );
    loadednmidpair_.remove( QString( wellnm.str() ) );
}


int WellFileList::nrFiles() const
{
    return filelist_.size();
}


int WellFileList::nrWells() const
{
    return allidsnmpair_.size();
}


bool WellFileList::hasLoadedWellInfo() const
{
    Threads::Locker locker( lock_ );
    return !loadednmidpair_.isEmpty();
}


const QHash<const QString,QString>& WellFileList::loadedNameIDPairs() const
{
    Threads::Locker locker( lock_ );
    return loadednmidpair_;
}


const QHash<const QString,QString>& WellFileList::allIdsNamePairs() const
{
    Threads::Locker locker( lock_ );
    return allidsnmpair_;
}


const QHash<const QString,QString>& WellFileList::fileList() const
{
    Threads::Locker locker( lock_ );
    return filelist_;
}


void WellFileList::catchChange()
{
    WellFileList currlist;
    TypeSet<MultiID> ids;
    Well::Man::getWellKeys( ids, true );
    for ( const auto& id : ids )
    {
	const Well::Data* wd = Well::MGR().get( id, Well::LoadReqs(Well::Inf) );
	if ( !wd )
	    continue;

	currlist.addLoadedWells( wd->name(), id );
    }

    catchChangedWells( currlist );
    catchChangedFiles( currlist );
}


bool WellFileList::getDeletedWells( const WellFileList& oth )
{
    if ( oth.nrWells() >= nrWells() )
	return false;

    QHashIterator<const QString,QString> iter( allidsnmpair_ );
    while ( iter.hasNext() )
    {
	iter.next();
	if ( oth.allIdsNamePairs().contains(iter.key()) )
	    continue;

	BufferString idstr( iter.key() );
	WellUpdateQueue::WUQ().enqueueWellsToBeDeleted( MultiID(idstr.str()) );
    }

    return true;
}


bool WellFileList::getRenamedWells( const WellFileList& oth )
{
    if ( oth.nrWells() != nrWells() )
	return false;

    QHashIterator<const QString,QString> iter( oth.allIdsNamePairs() );
    while ( iter.hasNext() )
    {
	iter.next();
	if ( allidsnmpair_.value( iter.key() ).isEmpty() )
	    continue;

	if ( allidsnmpair_.contains(iter.key())
	    && allidsnmpair_.value(iter.key()) == iter.value() )
	    continue;

	BufferString idstr( iter.key() );
	const MultiID id( idstr.str() );
	Well::LoadReqs req = Well::MGR().loadState( id );
	if ( req.isEmpty() )
	    continue;

	std::pair<MultiID, Well::LoadReqs> idreqpair( id, req );
	WellUpdateQueue::WUQ().enqueue( idreqpair );
    }

    return true;

}


bool WellFileList::getDeletedFiles( const WellFileList& oth )
{
    if ( oth.nrFiles() >= nrFiles() )
	return false;

    QHashIterator<const QString,QString> iter( filelist_ );
    while ( iter.hasNext() )
    {
	iter.next();
	if ( oth.fileList().contains(iter.key()) )
	    continue;

	updateWellQueue( iter.key() );
    }

    return true;
}


bool WellFileList::getAddedFiles( const WellFileList& oth )
{
    if ( oth.nrFiles() <= nrFiles() )
	return false;

    QHashIterator<const QString,QString> iter( oth.fileList() );
    while ( iter.hasNext() )
    {
	iter.next();
	if ( filelist_.contains(iter.key()) )
	    continue;

	QString nm = iter.key();
	BufferString nmbuf( nm );
	updateWellQueue( iter.key() );
    }

    return true;
}


bool WellFileList::getChangedFiles( const WellFileList& oth )
{
    if ( oth.nrFiles() != nrFiles() )
	return false;

    QHashIterator<const QString,QString> iter( oth.fileList() );
    while ( iter.hasNext() )
    {
	iter.next();
	const BufferString nm( iter.key() );
	const BufferString val( iter.value() );
	const BufferString currval( filelist_.value( iter.key() ) );
	if ( filelist_.contains(iter.key())
	    && filelist_.value(iter.key()) == iter.value() )
	    continue;
	else if ( filelist_.contains(iter.key()) )
	    updateWellQueue( iter.key() );
    }

    return true;
}


void WellFileList::updateWellQueue( const QString& fnm, bool reqall )
{
    BufferString fname( fnm );
    const FilePath fp( fname );
    BufferString basenm = fp.baseName();
    BufferString extstr = fp.extension();
    BufferString ext(".",extstr);

    if ( basenm.isEmpty() || ext.isEmpty() )
	return;

    Well::LoadReqs inclreq( false );
    if ( reqall )
	inclreq = Well::LoadReqs::All();
    else
	inclreq = Well::LoadReqs::getLoadReqFromFileExt( ext );

    if ( inclreq.isEmpty() )
	return;

    if ( inclreq.includes(Well::Logs) || inclreq.includes(Well::LogInfos) )
    {
	const BufferString logfilechar = basenm.find( "^" );
	basenm = basenm.remove( logfilechar.buf() );
    }

    const QString wellnm = basenm.buf();
    if ( !loadednmidpair_.contains(wellnm) )
	return;

    const BufferString idstr( loadednmidpair_.value(wellnm) );
    const MultiID id( idstr.str() );
    std::pair<MultiID, Well::LoadReqs> idreqpair( id, inclreq );
    fname = File::getAbsolutePath( Well::Man::wellDirPath(), fname );
    if ( !File::isInUse(fname) )
	WellUpdateQueue::WUQ().enqueue( idreqpair );
}


bool WellFileList::catchChangedWells( const WellFileList& currlist )
{
    return getDeletedWells( currlist )
	|| getRenamedWells( currlist );
}


bool WellFileList::catchChangedFiles( const WellFileList& currlist )
{
    return getDeletedFiles( currlist )
	|| getAddedFiles( currlist )
	|| getChangedFiles( currlist );
}
