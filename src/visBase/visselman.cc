/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visselman.cc,v 1.6 2002-03-11 10:53:40 kristofer Exp $";

#include "visselman.h"
#include "visscene.h"
#include "visdataman.h"

#include "Inventor/nodes/SoSelection.h"
#include "Inventor/SoPath.h"


visBase::SelectionManager::SelectionManager()
    : selnotifer( this )
    , deselnotifer( this )
{ }


visBase::SelectionManager::~SelectionManager()
{ }


void visBase::SelectionManager::deSelectAll()
{
    while ( selscenes.size() )
	selscenes[0]->deSelectAll();
}


int visBase::SelectionManager::nrSelected() const
{
    return selected.size();
}


const visBase::DataObject*
visBase::SelectionManager::getSelDataObject( int idx ) const
{ return selected[idx]; }


const visBase::Scene*
visBase::SelectionManager::getSelScene( int idx ) const
{ return selscenes[idx]; }


int visBase::SelectionManager::getSelNr(const DataObject* d,
					const Scene* scene) const 
{
    for ( int idx=0; idx<selected.size(); idx++ )
    {
	if ( d==selected[idx] && (!scene || scene==selscenes[idx] ))
	    return idx;
    }

    return -1;
}


void visBase::SelectionManager::regSelObject( DataObject& newsel )
{
    if ( selobjs.indexOf( &newsel )>= 0 ) return;
    selobjs += &newsel;
}


void visBase::SelectionManager::unRegSelObject( DataObject& sel )
{
    int idx = selobjs.indexOf( &sel );
    if ( idx>= 0 ) return;

    selobjs.remove( idx );
}


void visBase::SelectionManager::select( Scene* scene, SoPath* path )
{
    SoNode* tail = path->getTail();

    bool changed = false;

    for ( int idx=0; idx<selobjs.size(); idx++ )
    {
	if ( tail==selobjs[idx]->getSelObj() )
	{
	    if ( getSelNr( selobjs[idx], scene )>=0 ) continue;
	    
	    selected += selobjs[idx];
	    selscenes += scene;
	    
	    selobjs[idx]->triggerSel();
	    changed = true;
	}
    }

    if ( changed ) selnotifer.trigger();
}


void visBase::SelectionManager::deSelect( Scene* scene, SoPath* path )
{
    SoNode* tail = path->getTail();

    bool changed = false;

    for ( int idx=0; idx<selobjs.size(); idx++ )
    {
	if ( tail==selobjs[idx]->getSelObj() )
	{
	    const int pos = getSelNr( selobjs[idx], scene );
	    if ( pos<0 ) continue;
	    
	    selected.remove(pos);
	    selscenes.remove(pos);
	    
	    selobjs[idx]->triggerDeSel();
	    changed = true;
	}
    }

    if ( changed ) deselnotifer.trigger();
}


void visBase::SelectionManager::selectCB( void* obj, SoPath* path )
{
    visBase::DataManager::manager.selMan().select( (visBase::Scene*) obj,
	    					   path );
}


void visBase::SelectionManager::deSelectCB( void* obj, SoPath* path )
{
    visBase::DataManager::manager.selMan().deSelect((visBase::Scene*) obj,
	    					   path );
}
