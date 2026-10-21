/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvsgdataman.h"

#include "visvsgaxes.h"
#include "visvsgcamera.h"
#include "visvsgdata.h"
#include "visvsgdatagroup.h"
#include "visvsgevent.h"
#include "visvsgplanedragger.h"
#include "visvsgscenecoltab.h"
#include "visvsgthumbwheel.h"
#include "visvsgtext.h"
#include "visvsgtransform.h"

#include "errmsg.h"
#include "iopar.h"
#include "ptrman.h"

namespace dGBVis
{

mImplFactory( DataObject, DataManager::factory );

const char* DataManager::sKeyFreeID()		{ return "Free ID"; }

DataManager& DM()
{
    static PtrMan<DataManager> manager = new DataManager();
    return *manager.ptr();
}


void initStdClasses()
{
    mIfNotFirstTime( return );

    DataObjectGroup::initClass();
    Transformation::initClass();
    Text2::initClass();
    EventCatcher::initClass();
    DepthTabPlaneDragger::initClass();
    Axes::initClass();
    Camera::initClass();
    ThumbWheel::initClass();
    SceneColTab::initClass();
}


DataManager::DataManager()
    : removeallnotify(this)
    , selman_(*new SelectionManager)
{}


DataManager::~DataManager()
{
    delete &selman_;
#ifdef __debug__
    for ( const auto* obj : objects_ )
    {
	BufferString msg( "dGBVis object " );
	msg.add( obj->name() ).add( " (" ).add( obj->id().asInt() )
	   .add( ") is still referenced." );
	pErrMsg( msg );
    }
#endif
}


VisID DataManager::highestID() const
{
    int max = 0;
    for ( const auto* obj : objects_ )
    {
	const int objid = obj->id().asInt();
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
	DataObject* obj = it->second;
	visidobjmap_.erase( it );
	visidobjmap_[newid.asInt()] = obj;
    }
}


const DataObject* DataManager::getObject( const VisID& id ) const
{
    return getNonConst(*this).getObject( id );
}


DataObject* DataManager::getObject( const VisID& id )
{
    auto it = visidobjmap_.find( id.asInt() );
    return it != visidobjmap_.end() ? it->second : nullptr;
}


VisID DataManager::getID( const vsg::Node* node ) const
{
    if ( !node )
	return VisID::udf();

    for ( const auto* obj : objects_ )
    {
	if ( obj->vsgNode(true)==node || obj->vsgNode(false)==node )
	    return obj->id();
    }

    return VisID::udf();
}


void DataManager::getIDs( const std::type_info& ti, TypeSet<VisID>& res ) const
{
    const std::size_t tihash = ti.hash_code();
    for ( const auto* obj : objects_ )
    {
	if ( typeid(*obj).hash_code() == tihash )
	    res += obj->id();
    }
}


int DataManager::nrObjects() const
{
    return objects_.size();
}


const DataObject* DataManager::getIndexedObject( int idx ) const
{
    return getNonConst(*this).getIndexedObject( idx );
}


DataObject* DataManager::getIndexedObject( int idx )
{
    return objects_.validIdx(idx) ? objects_.get( idx ) : nullptr;
}


void DataManager::fillPar( IOPar& par ) const
{
    par.set( sKeyFreeID(), freeid_ );
}


bool DataManager::usePar( const IOPar& par )
{
    return par.get( sKeyFreeID(), freeid_ );
}


void DataManager::addObject( DataObject* obj )
{
    objects_ += obj;
    const VisID visid( freeid_++ );
    visidobjmap_[visid.asInt()] = obj;
    obj->setID( visid );
}


void DataManager::removeObject( DataObject* obj )
{
    const int objidx = objects_.indexOf( obj );
    if ( !objects_.validIdx(objidx) )
	return;

    selman_.drop( obj->id() );
    objects_.removeSingle( objidx );
    visidobjmap_.erase( obj->id().asInt() );
}

} // namespace dGBVis
