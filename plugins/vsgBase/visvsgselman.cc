/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvsgselman.h"

#include "visvsgdata.h"
#include "visvsgdataman.h"
#include "thread.h"

namespace dGBVis
{

SelectionManager::SelectionManager()
    : mutex_(*new Threads::Mutex(true))
{}


SelectionManager::~SelectionManager()
{
    delete &mutex_;
}


void SelectionManager::select( const VisID& newid, bool lock )
{
    if ( lock )
	mutex_.lock();

    DataObject* dataobj = DM().getObject( newid );
    if ( dataobj && dataobj->selectable() && !selectedids_.isPresent(newid) )
    {
	deSelectAll( false );
	selectedids_ += newid;
	dataobj->triggerSel();
    }

    if ( lock )
	mutex_.unLock();
}


void SelectionManager::deSelect( const VisID& id, bool lock )
{
    if ( lock )
	mutex_.lock();

    const int idx = selectedids_.indexOf( id );
    if ( idx >= 0 )
    {
	selectedids_.removeSingle( idx );
	if ( DataObject* dataobj = DM().getObject(id) )
	    dataobj->triggerDeSel();
    }

    if ( lock )
	mutex_.unLock();
}


void SelectionManager::deSelectAll( bool lock )
{
    if ( lock )
	mutex_.lock();

    while ( selectedids_.size() )
	deSelect( selectedids_[0], false );

    if ( lock )
	mutex_.unLock();
}


void SelectionManager::drop( const VisID& id )
{
    Threads::MutexLocker lock( mutex_ );
    const int idx = selectedids_.indexOf( id );
    if ( idx >= 0 )
	selectedids_.removeSingle( idx );
}

} // namespace dGBVis
