/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: visdataman.cc,v 1.27 2005-02-04 14:31:34 kristofer Exp $";

#include "visdataman.h"
#include "visdata.h"
#include "visselman.h"
#include "iopar.h"
#include "ptrman.h"
#include "errh.h"

#include "Inventor/SoPath.h"

namespace visBase
{

const char* DataManager::freeidstr = "Free ID";
const char* DataManager::selmanprefix = "SelMan";

DataManager& DM()
{
    static DataManager* manager = 0;

    if ( !manager ) manager = new DataManager;

    return *manager;
}


DataManager::DataManager()
    : freeid( 0 )
    , selman( *new SelectionManager )
    , fact( *new Factory )
    , removeallnotify( this )
{ }


DataManager::~DataManager()
{
    removeAll();
    delete &selman;
    delete &fact;
}


void DataManager::fillPar( IOPar& par, TypeSet<int>& storids ) const
{
    IOPar selmanpar;
    selman.fillPar( selmanpar, storids );
    par.mergeComp( selmanpar, selmanprefix );

    par.set(freeidstr, freeid );

    for ( int idx=0; idx<storids.size(); idx++ )
    {
	IOPar dataobjpar;
	const DataObject* dataobj = getObject( storids[idx] );
	if ( !dataobj ) continue;
	dataobj->fillPar( dataobjpar, storids );

	BufferString idstr = storids[idx];
	par.mergeComp( dataobjpar, idstr );
    }
}


bool DataManager::usePar( const IOPar& par )
{
    removeAll();

    if ( !par.get( freeidstr, freeid ))
	return false;

    TypeSet<int> lefttodo;
    for ( int idx=0; idx<par.size(); idx++ )
    {
	BufferString key = par.getKey( idx );
	char* ptr = strchr(key.buf(),'.');
	if ( !ptr ) continue;
	*ptr++ = '\0';
	const int id = atoi( key.buf() );
	if ( lefttodo.indexOf(id) < 0 ) lefttodo += id;
    }

    sort( lefttodo );

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
	    if ( !obj ) { lefttodo.remove(idx); idx--; continue; }

	    int no = objects.indexOf( obj );
	    obj->setID(lefttodo[idx]);  

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
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id()>maxid ) maxid=objects[idx]->id();
    }

    freeid = maxid+1;

    for ( int idx=0;idx<createdobj.size(); idx++ )
	createdobj[idx]->unRefNoDelete();

    return change;
}


bool DataManager::removeAll(int nriterations)
{
    removeallnotify.trigger();

    bool objectsleft = false;
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( !objects[idx]->nrRefs() )
	{
	    objects[idx]->ref();
	    objects[idx]->unRef();
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
	    msg += objects[0]->id();
	    msg += objects[0]->getClassName();
	    pErrMsg( msg );

	    while ( objects[0]->nrRefs() ) objects[0]->unRef();
	}

	return false;
    }

    return removeAll( nriterations-1 );
}


DataObject* DataManager::getObject( int id ) 
{
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id()==id ) return objects[idx];
    }

    return 0;
}


const DataObject* DataManager::getObject( int id ) const
{ return const_cast<DataManager*>(this)->getObject(id); }


void DataManager::addObject( DataObject* obj )
{
    if ( objects.indexOf(obj)==-1 )
    {
	objects += obj;
	obj->setID(freeid++);
    }
}


void DataManager::getIds( const SoPath* path, TypeSet<int>& res ) const
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

	    if ( objnode==node ) res += objects[idx]->id();
	}
    }
}


void DataManager::getIds( const std::type_info& ti,
				   TypeSet<int>& res) const
{
    res.erase();

    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( typeid(*objects[idx]) == ti )
	    res += objects[idx]->id();
    }
}

void DataManager::removeObject( DataObject* dobj )
{ objects -= dobj; }


}; //namespace
