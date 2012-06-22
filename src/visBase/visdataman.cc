/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUnusedVar = "$Id: visdataman.cc,v 1.57 2012-06-22 08:59:37 cvsjaap Exp $";

#include "visdataman.h"
#include "visdata.h"
#include "visselman.h"
#include "separstr.h"
#include "envvars.h"
#include "errh.h"
#include "iopar.h"
#include "keystrs.h"
#include "ptrman.h"
#include <iostream>

#include <Inventor/SoPath.h>
#include <Inventor/SoDB.h>

#include <osg/Node>

namespace visBase
{

mImplFactory( DataObject, DataManager::factory );

const char* DataManager::sKeyFreeID()		{ return "Free ID"; }
const char* DataManager::sKeySelManPrefix()	{ return "SelMan"; }

DataManager& DM()
{
    static DataManager* manager = 0;

    if ( !manager ) manager = new DataManager;

    return *manager;
}


DataManager::DataManager()
    : freeid_( 0 )
    , selman_( *new SelectionManager )
    , removeallnotify( this )
{ }


DataManager::~DataManager()
{
    removeAll();
    delete &selman_;
}


const char* DataManager::errMsg() const
{ return errmsg_.str(); }


void DataManager::readLockDB()
{ SoDB::readlock(); }
    

void DataManager::readUnLockDB()
{ SoDB::readunlock(); }


void DataManager::writeLockDB()
{ SoDB::writelock(); }

    
void DataManager::writeUnLockDB()
{ SoDB::writeunlock(); }


void DataManager::fillPar( IOPar& par, TypeSet<int>& storids ) const
{
    IOPar selmanpar;
    selman_.fillPar( selmanpar, storids );
    par.mergeComp( selmanpar, sKeySelManPrefix() );

    for ( int idx=0; idx<storids.size(); idx++ )
    {
	IOPar dataobjpar;
	const DataObject* dataobj = getObject( storids[idx] );
	if ( !dataobj ) continue;
	dataobj->fillPar( dataobjpar, storids );
	par.mergeComp( dataobjpar, toString(storids[idx]) );
    }

    sort( storids );
    const int storedfreeid = storids.size() ? storids[storids.size()-1] + 1 : 0;
    par.set( sKeyFreeID(), storedfreeid );
}


int DataManager::usePar( const IOPar& par )
{
    removeAll();

    if ( !par.get( sKeyFreeID(), freeid_ ))
	return false;

    TypeSet<int> lefttodo;
    for ( int idx=0; idx<par.size(); idx++ )
    {
	BufferString key = par.getKey( idx );
	char* ptr = strchr(key.buf(),'.');
	if ( !ptr ) continue;
	*ptr++ = '\0';
	const int id = toInt( key.buf() );
	if ( lefttodo.indexOf(id) < 0 ) lefttodo += id;
    }

    sort( lefttodo );

    ObjectSet<DataObject> createdobj;

    bool change = true;
    bool acceptsincomplete = false;
    BufferStringSet warnings;
    while ( true )
    {
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

		const char* type = iopar->find( sKey::Type() );
		RefMan<DataObject> obj = factory().create( type );
		if ( !obj ) { lefttodo.remove(idx); idx--; continue; }

		obj->setID( lefttodo[idx] );
		const int res = obj->usePar( *iopar );
		if ( GetEnvVarYN("OD_DEBUG_RESTORE_SESSION") )
		{
		    BufferString str( "[" ); str += lefttodo[idx]; str += "]";
		    std::cerr << str << " " << type << '\t' 
			      << "Status: " << res << std::endl;
		}

		if ( res==-1 )
		{
		    BufferString errmsg = obj->errMsg();
		    if ( errmsg.isEmpty() )
		    {
			errmsg =
			    BufferString( toString( lefttodo[idx]), ": ", type);
		    }
		    warnings.add( errmsg );
		    lefttodo.remove(idx);
		    idx--;
		    continue;
		}

		if ( res==1 ||
		     (acceptsincomplete && obj->acceptsIncompletePar() ) )
		{
		    createdobj += obj;
		    obj->ref();

		    lefttodo.remove( idx );
		    idx--;
		    change = true;
		}

	    }
	}

	if ( !change && !acceptsincomplete )
	{
	    change = true;
	    acceptsincomplete = true; 
	    continue;
	}

	break;
    }

    int maxid = -1;
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	if ( objects_[idx]->id()>maxid ) maxid=objects_[idx]->id();
    }

    freeid_ = maxid+1;

    for ( int idx=0;idx<createdobj.size(); idx++ )
	createdobj[idx]->unRefNoDelete();

    FileMultiString err;
    for ( int idx=0; idx<warnings.size(); idx++ )
    {
	if ( !idx )
	    err += "Problem loading scene:";

	err += warnings[idx]->buf();
    }

    if ( warnings.size() )
	errmsg_ = err;

    if ( !change ) return -1;
    return warnings.size() ? 0 : 1;
}


bool DataManager::removeAll(int nriterations)
{
    removeallnotify.trigger();

    bool objectsleft = false;
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	if ( !objects_[idx]->nrRefs() )
	{
	    objects_[idx]->ref();
	    objects_[idx]->unRef();
	    idx--;
	}
	else objectsleft = true;
    }

    if ( !objectsleft ) return true;
    if ( !nriterations )
    {
	pErrMsg("All objects not unreferenced");
	while ( objects_.size() )
	{
	    BufferString msg = "Forcing removal of ID: ";
	    msg += objects_[0]->id();
	    msg += objects_[0]->getClassName();
	    pErrMsg( msg );

	    while ( objects_.size() && objects_[0]->nrRefs() )
		objects_[0]->unRef();
	}

	return false;
    }

    return removeAll( nriterations-1 );
}


int DataManager::highestID() const
{
    int max = 0;

    const int nrobjects = objects_.size();
    for ( int idx=0; idx<nrobjects; idx++ )
    {
	if ( objects_[idx]->id()>max )
	    max = objects_[idx]->id();
    }

    return max;
}


DataObject* DataManager::getObject( int id ) 
{
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	if ( objects_[idx]->id()==id ) return objects_[idx];
    }

    return 0;
}


const DataObject* DataManager::getObject( int id ) const
{ return const_cast<DataManager*>(this)->getObject(id); }


DataObject* DataManager::getObject( const SoNode* node )
{
    if ( !node ) return 0;

    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	if ( objects_[idx]->getInventorNode()==node ) return objects_[idx];
    }

    return 0;
}


const DataObject* DataManager::getObject( const SoNode* node ) const
{ return const_cast<DataManager*>(this)->getObject(node); }


void DataManager::addObject( DataObject* obj )
{
    if ( objects_.indexOf(obj)==-1 )
    {
	objects_ += obj;
	obj->setID(freeid_++);
    }
}


void DataManager::getIds( const SoPath* path, TypeSet<int>& res ) const
{
    res.erase();

    const int nrobjs = objects_.size();

    for ( int pathidx=path->getLength()-1; pathidx>=0; pathidx-- )
    {
	SoNode* node = path->getNode( pathidx );

	for ( int idx=0; idx<nrobjs; idx++ )
	{
	    const SoNode* objnode = objects_[idx]->getInventorNode();
	    if ( !objnode ) continue;

	    if ( objnode==node ) res += objects_[idx]->id();
	}
    }
}


void DataManager::getIds( const std::type_info& ti,
				   TypeSet<int>& res) const
{
    res.erase();

    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	if ( typeid(*objects_[idx]) == ti )
	    res += objects_[idx]->id();
    }
}


void DataManager::removeObject( DataObject* dobj )
{ objects_ -= dobj; }


int DataManager::nrObjects() const
{ return objects_.size(); }


DataObject* DataManager::getIndexedObject( int idx )
{ return idx>=0 && idx<nrObjects() ? objects_[idx] : 0; }


const DataObject* DataManager::getIndexedObject( int idx ) const
{ return const_cast<DataManager*>(this)->getIndexedObject(idx); }


}; //namespace
