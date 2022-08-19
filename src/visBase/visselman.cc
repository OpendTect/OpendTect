/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visselman.h"
#include "visscene.h"
#include "visdataman.h"
#include "thread.h"

namespace visBase
{

SelectionManager::SelectionManager()
    : selnotifier( this )
    , deselnotifier( this )
    , updateselnotifier( this )
    , reselnotifier( this )
    , allowmultiple_( false )
    , mutex_( *new Threads::Mutex )
{}


SelectionManager::~SelectionManager()
{
    delete &mutex_;
}


void SelectionManager::setAllowMultiple( bool yn )
{
    Threads::MutexLocker lock( mutex_ );

    while ( !yn && selectedids_.size()>1 )
	deSelect( selectedids_[0], false );

    allowmultiple_ = yn;
}


void SelectionManager::select( VisID newid, bool keepoldsel, bool lock )
{
    if ( lock ) mutex_.lock();

    if ( !selectedids_.isPresent(newid) )
    {
	if ( !allowmultiple_ || !keepoldsel )
	    deSelectAll( false );

	DataObject* dataobj = DM().getObject( newid );

	if ( dataobj )
	{
	    selectedids_ += newid;
	    dataobj->triggerSel();
	    selnotifier.trigger( newid );
	}
    }

    if ( lock ) mutex_.unLock();
}


void SelectionManager::deSelect( VisID id, bool lock )
{
    if ( lock ) mutex_.lock();

    int idx = selectedids_.indexOf( id );
    if ( idx!=-1 )
    {
	selectedids_.removeSingle( idx );

	DataObject* dataobj = DM().getObject( id );
	if ( dataobj )
	{
	    dataobj->triggerDeSel();
	    deselnotifier.trigger( id );
	}

    }

    if ( lock ) mutex_.unLock();
}


void SelectionManager::deSelectAll(bool lock)
{
    if ( lock ) mutex_.lock();

    while ( selectedids_.size() )
	deSelect( selectedids_[0], false );

    if ( lock ) mutex_.unLock();
}


void SelectionManager::updateSel( VisID id, bool lock )
{
    if ( lock ) mutex_.lock();

    if ( selectedids_.isPresent(id) )
	updateselnotifier.trigger( id );

    if ( lock ) mutex_.unLock();
}

} // namespace visBase
