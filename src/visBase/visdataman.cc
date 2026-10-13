/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visdataman.h"
#include "visdata.h"
#include "visselman.h"

#include "iopar.h"
#include "ptrman.h"

#include <iostream>
#include <typeinfo>

namespace visBase
{

mImplFactory( DataObject, DataManager::factory );

const char* DataManager::sKeyFreeID()		{ return "Free ID"; }
const char* DataManager::sKeySelManPrefix()	{ return "SelMan"; }

static Threads::Lock& DataManagerLock()
{
    static Threads::Lock lock;
    return lock;
}

DataManager& DM()
{
    static PtrMan<DataManager> manager = new DataManager();
    return *manager.ptr();
}


DataManager::DataManager()
    : removeallnotify(this)
    , selman_(*new SelectionManager)
{
    objidxmap_[nullptr] = -1;
}


DataManager::~DataManager()
{
    {
	Threads::Locker locker( DataManagerLock() );
	objidxmap_.clear();
	visidobjmap_.clear();
#ifdef __debug__
	const RefCount::WeakPtrSetBase::CleanupBlocker cleanupblock( objects_ );
	for ( int idx=0; idx<objects_.size(); idx++ )
	{
	    ConstRefMan<DataObject> obj = objects_[idx];
	    if ( !obj )
		continue;

	    std::cerr << "Vis object " << obj->name().buf() << " ("
		      << obj->id().asInt() << ") is still referenced.\n";
	}
#endif
    }

    delete &selman_;
}


const char* DataManager::errMsg() const
{ return errmsg_.str(); }


VisID DataManager::highestID() const
{
    Threads::Locker locker( DataManagerLock() );
    int max = 0;

    const int sz = nrObjects();
    for ( int idx=0; idx<sz; idx++ )
    {
	ConstRefMan<DataObject> dataobj = objects_[idx];
	if ( !dataobj )
	    continue;

	const int objid = dataobj->id().asInt();
	if ( objid > max )
	    max = objid;
    }

    return VisID( max );
}


void DataManager::updateID( const VisID& oldid, const VisID& newid )
{
    Threads::Locker locker( DataManagerLock() );
    auto it = visidobjmap_.find( oldid.asInt() );
    if ( it != visidobjmap_.end() )
    {
	visidobjmap_[newid.asInt()] = it->second;
	visidobjmap_.erase( it );
    }
}


const DataObject* DataManager::getObject( const VisID& id ) const
{
    return getNonConst(*this).getObject( id );
}


DataObject* DataManager::getObject( const VisID& id )
{
    Threads::Locker locker( DataManagerLock() );
    const auto it = visidobjmap_.find( id.asInt() );
    return it==visidobjmap_.end() ? nullptr : it->second;
}


void DataManager::fillPar( IOPar& par ) const
{
    Threads::Locker locker( DataManagerLock() );
    par.set( sKeyFreeID(), freeid_ );
}


bool DataManager::usePar( const IOPar& par )
{
    Threads::Locker locker( DataManagerLock() );
    return par.get( sKeyFreeID(), freeid_ );
}


VisID DataManager::getID( const osg::Node* node ) const
{
    if ( !node )
	return VisID::udf();

    Threads::Locker locker( DataManagerLock() );
    const int sz = nrObjects();
    for ( int idx=0; idx<sz; idx++ )
    {
	ConstRefMan<DataObject> dataobj = objects_[idx];
	if ( dataobj->osgNode(true) == node )
	    return dataobj->id();
    }

    return VisID::udf();
}


void DataManager::addObject( DataObject* obj )
{
    Threads::Locker locker( DataManagerLock() );
    objects_ += obj;
    const VisID visid( freeid_++ );
    obj->setID( visid );
    const int idasint = visid.asInt();
    visidobjmap_[idasint] = obj;
    const int objidx = objects_.size() - 1;
    objidxmap_[obj] = objidx;
}


void DataManager::getIDs( const std::type_info& ti, TypeSet<VisID>& res ) const
{
    Threads::Locker locker( DataManagerLock() );
    const std::size_t tihash = ti.hash_code();
    const int sz = nrObjects();
    for ( int idx=0; idx<sz; idx++ )
    {
	ConstRefMan<DataObject> dataobj = objects_[idx];
	if ( !dataobj )
	    continue;

	const DataObject* cdataobj = dataobj.ptr();
	const std::type_info& objinfo = typeid( *cdataobj );
	if ( objinfo.hash_code() == tihash )
	    res += dataobj->id();
    }
}


void DataManager::runCleanup()
{
    Threads::Locker locker( DataManagerLock() );
    objidxmap_.clear();
    visidobjmap_.clear();
    objects_.cleanupNullPtrs();
    objidxmap_[nullptr] = -1;
    for ( int idx=0; idx<objects_.size(); idx++ )
    {
	RefMan<DataObject> obj = objects_[idx];
	if ( !obj || objidxmap_.find(obj.ptr())!=objidxmap_.end() )
	    continue;

	objidxmap_[obj.ptr()] = idx;
	visidobjmap_[obj->id().asInt()] = obj.ptr();
    }
}


void DataManager::removeObject( DataObject* dobj )
{
    Threads::Locker locker( DataManagerLock() );
    const RefCount::WeakPtrSetBase::CleanupBlocker cleanupblock( objects_ );
    const auto it = objidxmap_.find( dobj );
    if ( it==objidxmap_.end() )
	return;

    const int objidx = it->second;
    if ( objects_.validIdx(objidx) )
    {
	objects_[objidx] = nullptr;
	const int idasint = dobj->id().asInt();
	visidobjmap_.erase( idasint );
	objidxmap_.erase( dobj );
    }
}


int DataManager::nrObjects() const
{
    Threads::Locker locker( DataManagerLock() );
    const RefCount::WeakPtrSetBase::CleanupBlocker cleanupblock( objects_ );
    return objects_.size();
}


const DataObject* DataManager::getIndexedObject( int idx ) const
{
    return getNonConst(*this).getIndexedObject( idx );
}


DataObject* DataManager::getIndexedObject( int idx )
{
    Threads::Locker locker( DataManagerLock() );
    if ( !objects_.validIdx(idx) )
	return nullptr;

    RefMan<DataObject> dataobj = objects_[idx];
    return dataobj.ptr();
}

} // namespace visBase
