/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvsgdatagroup.h"

#include "visvsgdataman.h"
#include "errmsg.h"

#include <vsg/nodes/Group.h>

mCreateDGBVisFactoryEntry( dGBVis::DataObjectGroup );

namespace dGBVis
{

DataObjectGroup::DataObjectGroup()
    : DataObjectGroup(vsg::Group::create().get())
{
}


DataObjectGroup::DataObjectGroup( vsg::Group* grp )
    : change(this)
    , vsggroup_(grp)
    , pixeldensity_(getDefaultPixelDensity())
{
    setVsgNode( vsggroup_ );
    // Pick traversal uses only the pick bits. A group that is on must
    // keep those bits, or the visitor never reaches the children.
    // turnOn(false) still sets the switch to off.
    setPickable( true, true );
}


DataObjectGroup::~DataObjectGroup()
{
    detachAllNotifiers();
}


int DataObjectGroup::size() const
{
    return objects_.size();
}


void DataObjectGroup::handleNewObj( DataObject* no )
{
    compileNode( no->vsgNode() );
    no->setRightHandSystem( isRightHandSystem() );
    no->setPixelDensity( getPixelDensity() );
    change.trigger();
    requestSingleRedraw();
}


void DataObjectGroup::addObject( DataObject* no )
{
    if ( !no )
	return;

    if ( !isVisualizationThread() )
	{ pErrMsg("Not on the visualization thread"); }

    objects_.add( no );
    vsggroup_->addChild( vsg::ref_ptr<vsg::Node>(no->vsgNode()) );
    handleNewObj( no );
}


void DataObjectGroup::addObject( const VisID& nid )
{
    addObject( DM().getObject(nid) );
}


void DataObjectGroup::insertObject( int insertpos, DataObject* no )
{
    if ( !no )
	return;

    if ( insertpos>=size() )
	return addObject( no );

    if ( !isVisualizationThread() )
	{ pErrMsg("Not on the visualization thread"); }

    const int childidx = mMAX( insertpos, 0 );
    objects_.insertAt( no, childidx );
    vsggroup_->children.insert( vsggroup_->children.begin() + childidx,
				vsg::ref_ptr<vsg::Node>(no->vsgNode()) );
    handleNewObj( no );
}


int DataObjectGroup::getFirstIdx( const VisID& nid ) const
{
    const DataObject* sceneobj = DM().getObject( nid );
    return sceneobj ? getFirstIdx( sceneobj ) : -1;
}


int DataObjectGroup::getFirstIdx( const DataObject* sceneobj ) const
{
    return objects_.indexOf( sceneobj );
}


void DataObjectGroup::removeObject( int idx )
{
    if ( !objects_.validIdx(idx) )
	return;

    if ( !isVisualizationThread() )
	{ pErrMsg("Not on the visualization thread"); }

    const vsg::Node* node = objects_.get(idx)->vsgNode();
    auto& children = vsggroup_->children;
    for ( od_uint64 childidx=0; childidx<children.size(); childidx++ )
    {
	if ( children[childidx].get() != node )
	    continue;

	releaseNode( children[childidx].get() );
	children.erase( children.begin() + childidx );
	break;
    }

    objects_.removeSingle( idx );

    change.trigger();
    requestSingleRedraw();
}


void DataObjectGroup::removeAll()
{
    while ( size() )
	removeObject( size()-1 );
}


DataObject* DataObjectGroup::getObject( int idx )
{
    return objects_.validIdx( idx ) ? objects_.get( idx ) : nullptr;
}


const DataObject* DataObjectGroup::getObject( int idx ) const
{
    return getNonConst(*this).getObject( idx );
}


void DataObjectGroup::setDisplayTransformation( const mDGBVisTrans* nt )
{
    for ( auto* obj : objects_ )
	obj->setDisplayTransformation( nt );
}


const mDGBVisTrans* DataObjectGroup::getDisplayTransformation() const
{
    for ( const auto* obj : objects_ )
	if ( obj->getDisplayTransformation() )
	    return obj->getDisplayTransformation();

    return nullptr;
}


void DataObjectGroup::setRightHandSystem( bool yn )
{
    righthandsystem_ = yn;
    for ( auto* obj : objects_ )
	obj->setRightHandSystem( yn );
}


bool DataObjectGroup::isRightHandSystem() const
{
    return righthandsystem_;
}


void DataObjectGroup::setPixelDensity( float dpi )
{
    DataObject::setPixelDensity( dpi );

    pixeldensity_ = dpi;
    for ( auto* obj : objects_ )
	obj->setPixelDensity( dpi );
}

} // namespace dGBVis
