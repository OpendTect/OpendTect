/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: visdataman.cc,v 1.6 2002-04-10 07:40:28 kristofer Exp $";

#include "visdataman.h"
#include "visdata.h"
#include "visselman.h"

#include "Inventor/SoPath.h"

visBase::DataManager visBase::DataManager::manager;

visBase::DataManager& visBase::DM() { return visBase::DataManager::manager; }


visBase::DataManager::DataManager()
    : freeid( 0 )
    , selman( *new SelectionManager )
{
    reInit();
}


visBase::DataManager::~DataManager()
{
    bool res = removeAll();
    delete &selman;
}


bool visBase::DataManager::reInit()
{
    return removeAll();
}


bool visBase::DataManager::removeAll(int nriterations)
{
    bool objectsleft = false;
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( !refcounts[idx] )
	{
	    remove( idx );
	    idx--;
	}
	else objectsleft = true;
    }

    if ( !objectsleft ) return true;
    if ( !nriterations )
    {
#if DEBUG==yes
	cerr << "Internal error in 'visBase::DataManager::removeAll():\n'"
		"All objects not unreferenced" << endl;
#endif
	while ( objects.size() ) remove( 0 );
	return false;
    }

    return removeAll( nriterations-1 );
}


void visBase::DataManager::ref( int id )
{
    int idx = getIdx( id );
    if ( idx<0 )
	return;

    refcounts[idx]++;
}


void visBase::DataManager::ref( const DataObject* d )
{
    int idx = objects.indexOf( d );
    if ( idx<0 ) return;

    refcounts[idx]++;
}


void visBase::DataManager::unRef( int id )
{
    int idx = getIdx( id );
    if ( idx<0 )
	return;

#if DEBUG==yes
    if ( !refcounts[idx] )
    {
	cerr << "Internal error in 'visBase::DataManager::unRef( int id )'"
	        "Decreasing a zero reference" << endl;
    }
#endif

    refcounts[idx]--;
    if ( !refcounts[idx] )
	remove( idx );
}


void visBase::DataManager::unRef( const DataObject* d )
{
    int idx = objects.indexOf( d );
    if ( idx<0 ) return;

#if DEBUG==yes
    if ( !refcounts[idx] )
    {
	cerr << "Internal error in 'visBase::DataManager::unRef( int id )'"
	        "Decreasing a zero reference" << endl;
    }
#endif

    refcounts[idx]--;
    if ( !refcounts[idx] )
	 remove( idx );
}


visBase::DataObject* visBase::DataManager::getObj( int id ) 
{
    const int idx = getIdx( id );

    if ( idx<0 ) return 0;

    return objects[idx];
}
	

const visBase::DataObject* visBase::DataManager::getObj( int id ) const
{
    const int idx = getIdx( id );

    if ( idx<0 ) return 0;

    return objects[idx];
}


int visBase::DataManager::addObj( DataObject* obj )
{
    int idx = objects.indexOf( obj );
    if ( idx<0 )
    {
	objects += obj;
	ids += freeid++;
	refcounts += 0;
	idx = ids.size()-1;
    }

    return ids[idx];
}


int visBase::DataManager::getId( const DataObject* obj ) const
{
    int idx = objects.indexOf( obj );
    if ( idx<0 ) return -1;

    return ids[idx];
}


int visBase::DataManager::getId( const SoPath* path ) const
{
    const int nrobjs = objects.size();

    for ( int pathidx=path->getLength()-1; pathidx>=0; pathidx-- )
    {
	SoNode* node = path->getNode( pathidx );

	for ( int idx=0; idx<nrobjs; idx++ )
	{
	    const SoNode* objnode = objects[idx]->getData();
	    if ( !objnode ) continue;

	    if ( objnode==node )
		return ids[idx];
	}
    }

    return -1;
}


void visBase::DataManager::getIds( const SoPath* path, TypeSet<int>& res ) const
{
    res.erase();

    const int nrobjs = objects.size();

    for ( int pathidx=path->getLength()-1; pathidx>=0; pathidx-- )
    {
	SoNode* node = path->getNode( pathidx );

	for ( int idx=0; idx<nrobjs; idx++ )
	{
	    const SoNode* objnode = objects[idx]->getData();
	    if ( !objnode ) continue;

	    if ( objnode==node )
		res += ids[idx];
	}
    }
}


void visBase::DataManager::remove( int idx )
{
    DataObject* obj = objects[idx];

    objects.remove( idx );
    ids.remove( idx );
    refcounts.remove( idx );

    obj->remove();
}


int visBase::DataManager::getIdx( int id ) const
{ return ids.indexOf( id ); }
