/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visselman.cc,v 1.19 2012/01/12 17:32:16 cvsjaap Exp $";

#include "visselman.h"
#include "visscene.h"
#include "visdataman.h"
#include "thread.h"

namespace visBase
{

SelectionManager::SelectionManager()
    : selnotifier( this )
    , deselnotifier( this )
    , reselnotifier( this )
    , allowmultiple( false )
    , mutex( *new Threads::Mutex )
{}


SelectionManager::~SelectionManager()
{ 
    delete &mutex;
}


void SelectionManager::setAllowMultiple( bool yn )
{
    Threads::MutexLocker lock( mutex );
    while ( !yn && selectedids.size()>1 ) deSelect( selectedids[0], false );
    allowmultiple = yn;
}


void SelectionManager::select( int newid, bool keepoldsel, bool lock )
{
    if ( lock ) mutex.lock();
    if ( selectedids.indexOf( newid ) == -1 )
    {
	if ( !allowmultiple || !keepoldsel )
	    deSelectAll(false);

	DataObject* dataobj = DM().getObject( newid );

	if ( dataobj )
	{
	    selectedids += newid;
	    dataobj->triggerSel();
	    selnotifier.trigger(newid);
	}
    }

    if ( lock ) mutex.unLock();
}


void SelectionManager::deSelect( int id, bool lock )
{
    if ( lock ) mutex.lock();

    int idx = selectedids.indexOf( id );
    if ( idx!=-1 )
    {
	selectedids.remove( idx );

	DataObject* dataobj = DM().getObject( id );
	if ( dataobj )
	{
	    dataobj->triggerDeSel();
	    deselnotifier.trigger(id);
	}

    }

    if ( lock ) mutex.unLock();
}


void SelectionManager::deSelectAll(bool lock)
{
    if ( lock ) mutex.lock();
    while ( selectedids.size() ) deSelect( selectedids[0], false );
    if ( lock ) mutex.unLock();
}

}; // namespace visBase
