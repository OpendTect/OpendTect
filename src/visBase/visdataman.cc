/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: visdataman.cc,v 1.24 2004-05-07 12:35:20 nanne Exp $";

#include "visdataman.h"
#include "visdata.h"
#include "visselman.h"
#include "iopar.h"
#include "ptrman.h"
#include "errh.h"

#include "Inventor/SoPath.h"


const char* visBase::DataManager::freeidstr = "Free ID";
const char* visBase::DataManager::selmanprefix = "SelMan";

visBase::DataManager& visBase::DM()
{
    static visBase::DataManager* manager = 0;

    if ( !manager ) manager = new visBase::DataManager;

    return *manager;
}


visBase::DataManager::DataManager()
    : freeid( 0 )
    , selman( *new SelectionManager )
    , fact( *new Factory )
    , removeallnotify( this )
{
    reInit();
}


visBase::DataManager::~DataManager()
{
    bool res = removeAll();
    delete &selman;
    delete &fact;
}


bool visBase::DataManager::reInit()
{
    return removeAll();
}


void visBase::DataManager::fillPar( IOPar& par, TypeSet<int>& storids ) const
{
    IOPar selmanpar;
    selman.fillPar( selmanpar, storids );
    par.mergeComp( selmanpar, selmanprefix );

    par.set(freeidstr, freeid );

    for ( int idx=0; idx<storids.size(); idx++ )
    {
	IOPar dataobjpar;
	const DataObject* dataobj = getObj( storids[idx] );
	if ( !dataobj ) continue;
	dataobj->fillPar( dataobjpar, storids );

	BufferString idstr = storids[idx];
	par.mergeComp( dataobjpar, idstr );
    }
}


bool visBase::DataManager::usePar( const IOPar& par )
{
    reInit();

    if ( !par.get( freeidstr, freeid ))
	return false;

    TypeSet<int> lefttodo;
    for ( int idx=0; idx<freeid; idx++ )
	lefttodo += idx;

    ObjectSet<DataObject> createdobj;

    bool change = true;
    while ( lefttodo.size() && change )
    {
	change = false;
	for ( int idx=0; idx<lefttodo.size(); idx++ )
	{
	    PtrMan<IOPar> iopar = par.subselect( lefttodo[idx] );
	    if ( !iopar )
	    {
		lefttodo.remove( idx );
		idx--;
		change = true;
		continue;
	    }

	    const char* type = iopar->find( DataObject::typestr );
	    DataObject* obj = factory().create( type );
	    if ( !obj ) continue;

	    int no = objects.indexOf( obj );
	    ids[no] = lefttodo[idx];
	    obj->id_ = lefttodo[idx];  


	    int res = obj->usePar( *iopar );
	    if ( res==-1 )
		return false;
	    if ( res==0 )
	    {
		//Remove object			
		obj->ref();
		obj->unRef();
		continue;
	    }

	    createdobj += obj;
	    obj->ref();
	   
	    lefttodo.removeFast( idx );
	    idx--;
	    change = true;
	}
    }

    int maxid = -1;
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	if ( ids[idx]>maxid ) maxid=ids[idx];
    }

    freeid = maxid+1;

    for ( int idx=0;idx<createdobj.size(); idx++ )
	createdobj[idx]->unRefNoDelete();

    return change;
}


bool visBase::DataManager::removeAll(int nriterations)
{
    removeallnotify.trigger();

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
	pErrMsg("All objects not unreferenced");
	while ( objects.size() )
	{
	    BufferString msg = "Forcing removal of ID: ";
	    msg += ids[0];
	    msg += objects[0]->getClassName();
	    pErrMsg( msg );
	    remove( 0 );
	}

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


void visBase::DataManager::unRef( int id, bool rem )
{
    int idx = getIdx( id );
    if ( idx<0 )
    {
	BufferString msg = "Trying to remove non-existing ID: ";
	msg += id;
	pErrMsg(msg);
	return;
    }

    if ( !refcounts[idx] )
    {
	BufferString msg =  "Decreasing a zero reference on ID: ";
	msg += id;
	pErrMsg(msg) ;
	return;
    }

    refcounts[idx]--;
    if ( !refcounts[idx] && rem )
	remove( idx );
}


void visBase::DataManager::unRef( const DataObject* d, bool rem )
{
    int idx = objects.indexOf( d );
    if ( idx<0 )
    {
	BufferString msg = "Trying to remove non-existing id of type: ";
	msg += d->getClassName();
	pErrMsg(msg);
	return;
    }

    if ( !refcounts[idx] )
    {
	BufferString msg =  "Decreasing a zero reference on ID: ";
	msg += ids[idx];
	pErrMsg(msg) ;
	return;
    }

    refcounts[idx]--;
    if ( !refcounts[idx] && rem )
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
	    const SoNode* objnode = objects[idx]->getInventorNode();
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
	    const SoNode* objnode = objects[idx]->getInventorNode();
	    if ( !objnode ) continue;

	    if ( objnode==node )
		res += ids[idx];
	}
    }
}


void visBase::DataManager::getIds( const std::type_info& ti,
				   TypeSet<int>& res) const
{
    res.erase();

    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( typeid(*objects[idx]) == ti )
	    res += ids[idx];
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
