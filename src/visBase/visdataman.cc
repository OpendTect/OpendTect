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
    objidxmap_.clear();
    visidobjmap_.clear();
    delete &selman_;
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


const char* DataManager::errMsg() const
{ return errmsg_.str(); }


VisID DataManager::highestID() const
{
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
    auto it = visidobjmap_.find( id.asInt() );
    if ( it != visidobjmap_.end() )
	return visidobjmap_[id.asInt()];

    return nullptr;
}


void DataManager::fillPar( IOPar& par ) const
{
    par.set( sKeyFreeID(), freeid_ );
}


bool DataManager::usePar( const IOPar& par )
{
    return par.get( sKeyFreeID(), freeid_ );
}


VisID DataManager::getID( const osg::Node* node ) const
{
    if ( !node )
	return VisID::udf();

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
    objects_ += obj;
    const VisID visid( freeid_++ );
    obj->setID( visid );
    visidobjmap_[visid.asInt()] = obj;
    objidxmap_[obj] = objects_.size() - 1;
}


void DataManager::getIDs( const std::type_info& ti, TypeSet<VisID>& res ) const
{
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
    const RefCount::WeakPtrSetBase::CleanupBlocker cleanupblock( objects_ );
    const int idx = objidxmap_[dobj];
    if ( objects_.validIdx(idx) )
    {
	objects_.removeSingle( idx );
	visidobjmap_.erase( dobj->id().asInt() );
	objidxmap_.erase( dobj );
    }
}


int DataManager::nrObjects() const
{
    const RefCount::WeakPtrSetBase::CleanupBlocker cleanupblock( objects_ );
    return objects_.size();
}


const DataObject* DataManager::getIndexedObject( int idx ) const
{
    return getNonConst(*this).getIndexedObject( idx );
}


DataObject* DataManager::getIndexedObject( int idx )
{
    if ( !objects_.validIdx(idx) )
	return nullptr;

    RefMan<DataObject> dataobj = objects_[idx];
    return dataobj.ptr();
}

} // namespace visBase
