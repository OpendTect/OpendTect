/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/


#include "visdataman.h"
#include "visdata.h"
#include "visselman.h"
#include "separstr.h"
#include "envvars.h"
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
    mDefineStaticLocalObject( DataManager, manager, );
    return manager;
}


DataManager::DataManager()
    : freeid_( 0 )
    , selman_( *new SelectionManager )
    , removeallnotify( this )
    , prevobjectidx_(0)
{ }


DataManager::~DataManager()
{
    delete &selman_;
}


const char* DataManager::errMsg() const
{ return errmsg_.str(); }


VisID DataManager::highestID() const
{
    int max = 0;

    const int nrobjects = objects_.size();
    for ( int idx=0; idx<nrobjects; idx++ )
    {
	if ( objects_[idx]->id().asInt()>max )
	    max = objects_[idx]->id().asInt();
    }

    return VisID(max);
}


#define mSmartLinearSearch( reversecondition, foundcondition, foundactions ) \
    const int sz = objects_.size(); \
    int idx = prevobjectidx_; \
    const int dir = idx<sz && reversecondition ? -1 : 1; \
    \
    for ( int count=0; count<sz; count++, idx+=dir ) \
    { \
	if ( idx < 0 )	 idx = sz-1; \
	if ( idx >= sz ) idx = 0; \
	\
	if ( foundcondition ) \
	{ \
	    prevobjectidx_ = idx; \
	    foundactions; \
	} \
    }


DataObject* DataManager::getObject( VisID id )
{
    mSmartLinearSearch( id.asInt() < objects_[idx]->id().asInt(),
			objects_[idx]->id()==id,
			return objects_[idx] );
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



const DataObject* DataManager::getObject( VisID id ) const
{ return const_cast<DataManager*>(this)->getObject(id); }


VisID DataManager::getID( const osg::Node* node ) const
{
    if ( node )
    {
	mSmartLinearSearch( false,
			    objects_[idx]->osgNode(true)==node,
			    return objects_[idx]->id() );
    }

    return VisID::udf();
}


void DataManager::addObject( DataObject* obj )
{
    if ( objects_.indexOf(obj)==-1 )
    {
	objects_ += obj;
	obj->setID( VisID(freeid_++) );
    }
}


void DataManager::getIDs( const std::type_info& ti, TypeSet<VisID>& res ) const
{
    res.erase();

    const std::size_t tihash = ti.hash_code();
    for ( const auto* obj : objects_ )
    {
	const std::type_info& objinfo = typeid(*obj);
	if ( objinfo.hash_code() == tihash )
	    res += obj->id();
    }
}


void DataManager::removeObject( DataObject* dobj )
{
    mSmartLinearSearch( dobj->id().asInt() < objects_[idx]->id().asInt(),
			objects_[idx]==dobj,
			objects_.removeSingle(idx); return );
}


int DataManager::nrObjects() const
{ return objects_.size(); }


DataObject* DataManager::getIndexedObject( int idx )
{ return idx>=0 && idx<nrObjects() ? objects_[idx] : 0; }


const DataObject* DataManager::getIndexedObject( int idx ) const
{ return const_cast<DataManager*>(this)->getIndexedObject(idx); }

} // namespace visBase
