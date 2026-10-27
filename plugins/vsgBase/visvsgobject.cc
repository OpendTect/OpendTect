/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvsgobject.h"

#include "visvsgtransform.h"

#include "errmsg.h"
#include "iopar.h"
#include "keystrs.h"

#include <vsg/nodes/Group.h>
#include <vsg/utils/ComputeBounds.h>

namespace dGBVis
{

const char* VisualObjectImpl::sKeyIsOn()	{ return "Is on"; }


// VisualObject

VisualObject::VisualObject( bool issel )
    : isselectable_(issel)
    , selnotifier_(this)
    , deselnotifier_(this)
    , rightclick_(this)
{}


VisualObject::~VisualObject()
{
    detachAllNotifiers();
}


bool VisualObject::getBoundingBox( Coord3& minpos, Coord3& maxpos ) const
{
    const vsg::Node* node = vsgNode( true );
    if ( !node )
	return false;

    vsg::ref_ptr<vsg::ComputeBounds> cb = vsg::ComputeBounds::create();
    node->accept( *cb );
    if ( !cb->bounds.valid() )
	return false;

    const vsg::dbox& bb = cb->bounds;
    const Transformation* trans = getDisplayTransformation();
    for ( int idx=0; idx<8; idx++ )
    {
	const vsg::dvec3 corner( idx&1 ? bb.max.x : bb.min.x,
				 idx&2 ? bb.max.y : bb.min.y,
				 idx&4 ? bb.max.z : bb.min.z );
	Coord3 pos;
	Transformation::transformBack( trans, corner, pos );
	if ( idx==0 )
	    minpos = maxpos = pos;
	else
	{
	    minpos.x_ = mMIN( minpos.x_, pos.x_ );
	    minpos.y_ = mMIN( minpos.y_, pos.y_ );
	    minpos.z_ = mMIN( minpos.z_, pos.z_ );
	    maxpos.x_ = mMAX( maxpos.x_, pos.x_ );
	    maxpos.y_ = mMAX( maxpos.y_, pos.y_ );
	    maxpos.z_ = mMAX( maxpos.z_, pos.z_ );
	}
    }

    return true;
}


// VisualObjectImpl

VisualObjectImpl::VisualObjectImpl( bool issel )
    : VisualObject(issel)
    , vsgroot_(nullptr)
{
    vsg::ref_ptr<vsg::Group> root = vsg::Group::create();
    vsgroot_ = setVsgNode( root.get() );
}


VisualObjectImpl::~VisualObjectImpl()
{
    detachAllNotifiers();
}


void VisualObjectImpl::setGroupNode( vsg::Group* grpnode )
{
    if ( !grpnode || vsgroot_ == grpnode )
	return;

    vsg::ref_ptr<vsg::Group> newgroup( grpnode );
    for ( auto& child : vsgroot_->children )
	newgroup->addChild( child );

    vsgroot_ = setVsgNode( grpnode );
}


int VisualObjectImpl::addChild( vsg::Node* nn )
{
    if ( !nn )
	return -1;

    if ( !isVisualizationThread() )
	{ pErrMsg("Not on the visualization thread"); }

    vsgroot_->addChild( vsg::ref_ptr<vsg::Node>(nn) );
    compileNode( nn );
    requestSingleRedraw();
    return int(vsgroot_->children.size()) - 1;
}


void VisualObjectImpl::insertChild( int pos, vsg::Node* nn )
{
    if ( !nn )
	return;

    if ( !isVisualizationThread() )
	{ pErrMsg("Not on the visualization thread"); }

    auto& children = vsgroot_->children;
    const od_uint64 idx = mMIN( od_uint64(mMAX(pos,0)),
				od_uint64(children.size()) );
    children.insert( children.begin() + idx, vsg::ref_ptr<vsg::Node>(nn) );
    compileNode( nn );
    requestSingleRedraw();
}


void VisualObjectImpl::removeChild( vsg::Node* nn )
{
    const int idx = childIndex( nn );
    if ( idx < 0 )
	return;

    if ( !isVisualizationThread() )
	{ pErrMsg("Not on the visualization thread"); }

    auto& children = vsgroot_->children;
    releaseNode( nn );
    children.erase( children.begin() + idx );
    requestSingleRedraw();
}


int VisualObjectImpl::childIndex( const vsg::Node* nn ) const
{
    const auto& children = vsgroot_->children;
    for ( od_uint64 idx=0; idx<children.size(); idx++ )
    {
	if ( children[idx].get() == nn )
	    return int(idx);
    }

    return -1;
}


bool VisualObjectImpl::usePar( const IOPar& iopar )
{
    bool isonsw;
    if ( iopar.getYN(sKeyIsOn(),isonsw) )
	turnOn( isonsw );

    const BufferString nm = iopar.find( sKey::Name() );
    if ( !nm.isEmpty() )
	setName( nm );

    return true;
}


void VisualObjectImpl::fillPar( IOPar& iopar ) const
{
    iopar.setYN( sKeyIsOn(), isOn() );
}

} // namespace dGBVis
