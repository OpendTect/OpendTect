/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visdatagroup.h"
#include "visdataman.h"
#include "iopar.h"

#include <osg/Group>

mCreateFactoryEntry( visBase::DataObjectGroup );

namespace visBase
{

DataObjectGroup::DataObjectGroup()
    : osggroup_(new osg::Group)
    , pixeldensity_(getDefaultPixelDensity())
    , change(this)
{
    setOsgNode( osggroup_ );
}


DataObjectGroup::~DataObjectGroup()
{
}


int DataObjectGroup::size() const
{
    return objects_.size();
}


void DataObjectGroup::handleNewObj( DataObject* no )
{
    no->setRightHandSystem( isRightHandSystem() );
    no->setPixelDensity( getPixelDensity() );
    change.trigger();
    requestSingleRedraw();
}


void DataObjectGroup::addObject( DataObject* no )
{
    objects_.add( no );

    if ( osggroup_ && no->osgNode() )
	osggroup_->addChild( no->osgNode() );

    handleNewObj( no );
}


void DataObjectGroup::setDisplayTransformation( const mVisTrans* nt )
{
    for ( auto* obj : objects_ )
	obj->setDisplayTransformation( nt );
}


const mVisTrans* DataObjectGroup::getDisplayTransformation() const
{
    for ( const auto* obj : objects_ )
	if ( obj->getDisplayTransformation() )
	    return obj->getDisplayTransformation();

    return nullptr;
}


void DataObjectGroup::setRightHandSystem( bool yn )
{
    righthandsystem_ = yn;

    yn = isRightHandSystem();

    for ( auto* obj : objects_ )
	obj->setRightHandSystem( yn );
}


void DataObjectGroup::setPixelDensity( float dpi )
{
    DataObject::setPixelDensity( dpi );

    pixeldensity_ = dpi;
    for ( auto* obj : objects_ )
	obj->setPixelDensity( dpi );
}


bool DataObjectGroup::isRightHandSystem() const
{
    return righthandsystem_;
}


void DataObjectGroup::addObject( const VisID& nid )
{
    mDynamicCastGet(DataObject*,no,DM().getObject(nid));
    if ( !no )
	return;

    addObject( no );
}


void DataObjectGroup::insertObject( int insertpos, DataObject* no )
{
    if ( insertpos>=size() )
	return addObject( no );

    objects_.insertAt( no, insertpos );

    if ( no->osgNode() )
	osggroup_->insertChild( insertpos, no->osgNode() );

    handleNewObj( no );
}


int DataObjectGroup::getFirstIdx( const VisID& nid ) const
{
    const DataObject* sceneobj =
	(const DataObject*) DM().getObject(nid);

    if ( !sceneobj )
	return -1;

    return getFirstIdx( sceneobj );
}


int DataObjectGroup::getFirstIdx( const DataObject* sceneobj ) const
{
    return objects_.indexOf( sceneobj );
}


void DataObjectGroup::removeObject( int idx )
{
    if ( !objects_.validIdx(idx) )
	return;

    DataObject* sceneobject = objects_[idx];
    osggroup_->removeChild( sceneobject->osgNode() );
    objects_.removeSingle( idx );

    change.trigger();
    requestSingleRedraw();
}


void DataObjectGroup::removeAll()
{
    while ( size() )
	removeObject( 0 );
}


DataObject* DataObjectGroup::getObject( int idx )
{
    return objects_.validIdx( idx ) ? objects_.get( idx ) : nullptr;
}


const DataObject* DataObjectGroup::getObject( int idx ) const
{
    return mSelf().getObject( idx );
}

} // namespace visBase
