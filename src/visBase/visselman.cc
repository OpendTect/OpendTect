/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visselman.cc,v 1.3 2002-02-28 12:41:39 kristofer Exp $";

#include "visselman.h"
#include "visscene.h"

#include "Inventor/nodes/SoSelection.h"
#include "Inventor/SoPath.h"


visBase::SelectionManager::SelectionManager()
    : node( new SoSelection )
    , selnotifer( this )
    , deselnotifer( this )
{
    node->ref();
    node->addSelectionCallback(visBase::SelectionManager::selectCB, this );
    node->addDeselectionCallback(visBase::SelectionManager::deSelectCB, this );
}


visBase::SelectionManager::~SelectionManager()
{
    node->removeSelectionCallback( visBase::SelectionManager::selectCB, this );
    node->removeDeselectionCallback(visBase::SelectionManager::deSelectCB,this);
    node->unref();
}


visBase::SelectionManager::Policy visBase::SelectionManager::policy() const
{
    if ( node->policy.getValue()== SoSelection::SHIFT )
	return Shift; 
    if ( node->policy.getValue()== SoSelection::TOGGLE )
	return Toggle;
    else
	return Single;
}


void visBase::SelectionManager::setPolicy( visBase::SelectionManager::Policy p )
{
    if ( p==Shift )
	node->policy.setValue( SoSelection::SHIFT );
    else if ( p==Toggle )
	node->policy.setValue( SoSelection::TOGGLE );
    else
	node->policy.setValue( SoSelection::SINGLE );
}


void visBase::SelectionManager::deSelectAll()
{
    node->deselectAll();
}


void visBase::SelectionManager::notifySelection(const VisualObject& assoc,
						const CallBack& cb )
{
    const int idx = indexOf( assoc );
    if ( idx<0 ) return;

    selnotifiers[idx]->notify(cb);
}


void visBase::SelectionManager::deNotifySelection(const VisualObject& assoc,
						  const CallBack& cb )
{
    const int idx = indexOf( assoc );
    if ( idx<0 ) return;

    selnotifiers[idx]->remove(cb);
}


void visBase::SelectionManager::notifyDeSelection(const VisualObject& assoc,
						  const CallBack& cb )
{
    const int idx = indexOf( assoc );
    if ( idx<0 ) return;

    deselnotifiers[idx]->notify(cb);
}


void visBase::SelectionManager::deNotifyDeSelection(const VisualObject& assoc,
						    const CallBack& cb )
{
    const int idx = indexOf( assoc );
    if ( idx<0 ) return;

    deselnotifiers[idx]->remove(cb);
}


int visBase::SelectionManager::nrSelected() const
{
    return node->getNumSelected();
}


const visBase::VisualObject*
visBase::SelectionManager::getSelected( int idx ) const
{
    const SoPathList* list = node->getList();
    if ( !list ) return false;

    const int size = nrSelected();
    const SoPath* path = (*list)[idx];
    const SoNode* tail = path->getTail();

    for ( int idy=0; idy<detobjs.size(); idy++ )
    {
	if ( tail==detobjs[idy]->getData() )
	    return assobjs[idy];
    }

    return 0;
}


void visBase::SelectionManager::regSelObject( const VisualObject& a,
					      const SceneObject& d )
{
    for ( int idx=0; idx<assobjs.size(); idx++ )
    {
	if ( assobjs[idx] == &a && detobjs[idx] == &d )
	    return;
    }

    assobjs += &a;
    detobjs += &d;
    selnotifiers += new Notifier<SelectionManager>( this );
    deselnotifiers += new Notifier<SelectionManager>( this );
}


void visBase::SelectionManager::unRegSelObject(const VisualObject& a,
					       const SceneObject* d )
{
    for ( int idx=0; idx<assobjs.size(); idx++ )
    {
	if ( assobjs[idx] == &a && (!d || detobjs[idx]==d) )
	{
	    assobjs.remove( idx );
	    detobjs.remove( idx );
	    delete selnotifiers[idx]; selnotifiers.remove( idx );
	    delete deselnotifiers[idx]; deselnotifiers.remove( idx );
	    return;
	}
    }
}


int visBase::SelectionManager::indexOf( const VisualObject& a ) const
{
    for ( int idx=0; idx<assobjs.size(); idx++ )
    {
	if ( assobjs[idx] == &a )
	{
	    return idx;
	}
    }

    return -1;
}


void visBase::SelectionManager::select( SoPath* path )
{
    SoNode* tail = path->getTail();

    for ( int idx=0; idx<detobjs.size(); idx++ )
    {
	if ( tail==detobjs[idx]->getData() )
	    selnotifiers[idx]->trigger();
    }

    selnotifer.trigger();
}


void visBase::SelectionManager::deSelect( SoPath* path )
{
    SoNode* tail = path->getTail();

    for ( int idx=0; idx<detobjs.size(); idx++ )
    {
	if ( tail==detobjs[idx]->getData() )
	    deselnotifiers[idx]->trigger();
    }

    deselnotifer.trigger();
}


void visBase::SelectionManager::selectCB( void* obj, SoPath* path )
{
    ((visBase::SelectionManager*) obj)->select( path );
}


void visBase::SelectionManager::deSelectCB( void* obj, SoPath* path )
{
    ((visBase::SelectionManager*) obj)->deSelect( path );
}
