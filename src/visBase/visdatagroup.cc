/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Jan 2002
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
    : osggroup_( new osg::Group )
    , separate_( true )
    , change( this )
    , righthandsystem_( true )
    , pixeldensity_( getDefaultPixelDensity() )
{
    setOsgNode( osggroup_ );
}


DataObjectGroup::~DataObjectGroup()
{
    mObjectSetApplyToAll( objects_, objects_[idx]->setParent( 0 ));

    deepUnRef( objects_ );
}


int DataObjectGroup::size() const
{ return objects_.size(); }


#define mNewObjectOperations \
no->ref(); \
no->setRightHandSystem( isRightHandSystem() ); \
no->setPixelDensity( getPixelDensity() ); \
no->setParent( this ); \
change.trigger(); \
requestSingleRedraw();

void DataObjectGroup::addObject( DataObject* no )
{
    objects_ += no;

    if ( osggroup_ && no->osgNode() ) osggroup_->addChild( no->osgNode() );

    mNewObjectOperations;
}


void DataObjectGroup::setDisplayTransformation( const mVisTrans* nt )
{
    for ( int idx=0; idx<objects_.size(); idx++ )
	objects_[idx]->setDisplayTransformation(nt);
}


const mVisTrans* DataObjectGroup::getDisplayTransformation() const
{
    for ( int idx=0; idx<objects_.size(); idx++ )
	if ( objects_[idx]->getDisplayTransformation() )
	    return objects_[idx]->getDisplayTransformation();

    return 0;
}


void DataObjectGroup::setRightHandSystem( bool yn )
{
    righthandsystem_ = yn;

    yn = isRightHandSystem();

    for ( int idx=0; idx<objects_.size(); idx++ )
	objects_[idx]->setRightHandSystem( yn );
}


void DataObjectGroup::setPixelDensity( float dpi )
{
    DataObject::setPixelDensity( dpi );

    pixeldensity_ = dpi;

    for ( int idx=0; idx<objects_.size(); idx++ )
	objects_[idx]->setPixelDensity( dpi );
}


bool DataObjectGroup::isRightHandSystem() const
{ return righthandsystem_; }


void DataObjectGroup::addObject(int nid)
{
    DataObject* no =
	dynamic_cast<DataObject*>( DM().getObject(nid) );

    if ( !no ) return;
    addObject( no );
}


void DataObjectGroup::insertObject( int insertpos, DataObject* no )
{
    if ( insertpos>=size() ) return addObject( no );

    objects_.insertAt( no, insertpos );

    if ( no->osgNode() ) osggroup_->insertChild( insertpos, no->osgNode() );
    mNewObjectOperations;
}


int DataObjectGroup::getFirstIdx( int nid ) const
{
    const DataObject* sceneobj =
	(const DataObject*) DM().getObject(nid);

    if ( !sceneobj ) return -1;

    return getFirstIdx( sceneobj );
}


int DataObjectGroup::getFirstIdx( const DataObject* sceneobj ) const
{ return objects_.indexOf(sceneobj); }


void DataObjectGroup::removeObject( int idx )
{
    if ( idx< 0 )  return;
    DataObject* sceneobject = objects_[idx];
    osggroup_->removeChild( sceneobject->osgNode() );


    objects_.removeSingle( idx );

    sceneobject->setParent( 0 );
    sceneobject->unRef();
    change.trigger();
    requestSingleRedraw();
}


void DataObjectGroup::removeAll()
{
    while ( size() ) removeObject( 0 );
}


}; // namespace visBase
