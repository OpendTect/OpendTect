/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visselman.cc,v 1.7 2002-04-10 07:40:28 kristofer Exp $";

#include "visselman.h"
#include "visscene.h"
#include "visdataman.h"
#include "thread.h"

#include "Inventor/nodes/SoSelection.h"
#include "Inventor/SoPath.h"


visBase::SelectionManager::SelectionManager()
    : selnotifer( this )
    , deselnotifer( this )
    , mutex( *new Threads::Mutex )
{ }


visBase::SelectionManager::~SelectionManager()
{ 
    delete &mutex;
}


void visBase::SelectionManager::setAllowMultiple( bool yn )
{
    Threads::Mutex::Locker lock( mutex );
    while ( !yn && selectedids.size()>1 ) deSelect( selectedids[0], false );
    allowmultiple = yn;
}


void visBase::SelectionManager::select( int newid, bool keepoldsel, bool lock )
{
    if ( lock ) mutex.lock();

    if ( !allowmultiple || !keepoldsel )
	deSelectAll(false);

    DataObject* dataobj = visBase::DM().getObj( newid );

    if ( dataobj )
    {
	selectedids += newid;
	dataobj->triggerSel();
	selnotifer.trigger();
    }

    if ( lock ) mutex.unlock();
}


void visBase::SelectionManager::deSelect( int id, bool lock )
{
    if ( lock ) mutex.lock();

    int idx = selectedids.indexOf( id );
    if ( idx!=-1 )
    {
	DataObject* dataobj = visBase::DM().getObj( id );
	if ( dataobj )
	{
	    selectedids.remove( idx );
	    dataobj->triggerDeSel();
	    deselnotifer.trigger();
	}
    }

    if ( lock ) mutex.unlock();
}


void visBase::SelectionManager::deSelectAll(bool lock)
{
    if ( lock ) mutex.lock();
    while ( selectedids.size() ) deSelect( selectedids[0], false );
    if ( lock ) mutex.unlock();
}
