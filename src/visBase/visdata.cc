/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "visdata.h"

#include "keystrs.h"
#include "thread.h"
#include "visdataman.h"
#include "visselman.h"
#include "vismaterial.h"

#include <osg/Node>
#include <osg/ValueObject>
#include <osgDB/WriteFile>
#include <osgViewer/CompositeViewer>

using namespace visBase;

const void* DataObject::visualizationthread_ = 0;
osgViewer::CompositeViewer* DataObject::commonviewer_ = 0;


void DataObject::enableTraversal( unsigned int tt, bool yn )
{
    enabledmask_ = yn
	? (enabledmask_ | tt )
	: (enabledmask_ & ~tt );

    updateNodemask();
}


bool DataObject::isTraversalEnabled( unsigned int tt ) const
{
    return enabledmask_ & tt;
}


FixedString DataObject::name() const
{
    osg::ref_ptr<const osg::Node> osgnode = osgNode();
    if ( osgnode ) return osgnode->getName().c_str();

    return !name_ || name_->isEmpty() ? 0 : name_->buf();
}


void DataObject::setName( const char* nm )
{
    if ( osgnode_ )
	osgnode_->setName( nm );
    else if ( nm )
    {
	if ( !name_ )
	    name_ = new BufferString;
    }

    if ( name_ ) (*name_) = nm;
}


DataObject::DataObject()
    : id_( -1 )
    , name_( 0 )
    , enabledmask_( cAllTraversalMask() )
    , osgnode_( 0 )
    , ison_( true )
{
    DM().addObject( this );
}


DataObject::~DataObject()
{
    DM().removeObject( this );
    delete name_;
    if ( osgnode_ ) osgnode_->unref();
    while ( nodestates_.size() )
	removeNodeState( nodestates_[0] );
}


void DataObject::doAddNodeState(visBase::NodeState* ns)
{
    ns->ref();
    nodestates_ += ns;
    osg::ref_ptr<osg::StateSet> stateset = getStateSet();
    if ( !stateset )
    {
	pErrMsg("Setting nodestate on class without stateset.");
    }
    else
	ns->attachStateSet( stateset );
}


visBase::NodeState* DataObject::removeNodeState( visBase::NodeState* ns )
{
    const int idx = nodestates_.indexOf( ns );
    if ( nodestates_.validIdx(idx) )
    {
	ns->detachStateSet( getStateSet() );
	nodestates_.removeSingle( idx )->unRef();
    }

    return ns;
}


NodeState* DataObject::getNodeState( int idx )
{
    return idx<=nodestates_.size() ? nodestates_[idx] : 0;
}


osg::StateSet* DataObject::getStateSet()
{
    return osgNode() ? osgNode()->getOrCreateStateSet() : 0;
}


void DataObject::setID( int nid )
{
    id_ = nid;
    updateOsgNodeData();
}

std::string idstr( sKey::ID() );

int DataObject::getID( const osg::Node* node )
{
    if ( node )
    {
	int objid;
	if ( node->getUserValue(idstr, objid) && objid>=0 )
	    return objid;
    }

    return -1;
}



bool DataObject::turnOn( bool yn )
{
    const bool res = isOn();
    ison_ = yn;

    updateNodemask();


    return res;
}


void DataObject::updateNodemask()
{
    if ( osgNode() )
	osgNode()->setNodeMask( ison_ ? enabledmask_ :  cNoTraversalMask() );

}


bool DataObject::isOn() const
{
    return ison_;
}


void DataObject::setPickable( bool yn )
{
    enableTraversal( cEventTraversalMask(), yn );
}


bool DataObject::isPickable() const
{
    return isTraversalEnabled( cEventTraversalMask() );
}


void DataObject::setOsgNodeInternal( osg::Node* osgnode )
{
    //Do this reverse order as osgnode may be a child of osgnode_
    if ( osgnode ) osgnode->ref();
    if ( osgnode_ ) osgnode_->unref();
    osgnode_ = osgnode;
    updateOsgNodeData();
}


void DataObject::updateOsgNodeData()
{
    if ( osgnode_ )
    {
	osgnode_->setName( name_ ? name_->buf() : sKey::EmptyString().str() );
	osgnode_->setUserValue( idstr, id() );
    }

    updateNodemask();
}


void DataObject::select() const
{ DM().selMan().select( id() ); }


void DataObject::deSelect() const
{ DM().selMan().deSelect( id() ); }


bool DataObject::isSelected() const
{ return selectable() && DM().selMan().selected().isPresent( id()); }


void DataObject::setDisplayTransformation( const mVisTrans* trans )
{   
    if ( trans!=getDisplayTransformation() )
	{ pErrMsg("Not implemented"); }
}   
    

bool DataObject::serialize( const char* filename, bool binary )
{
    if ( !osgNode() )
	return true;

    return osgDB::writeNodeFile( *osgNode(), std::string( filename ) );
}


bool DataObject::isVisualizationThread()
{
    if ( !visualizationthread_ )
    {
	pFreeFnErrMsg("Visualization thread not set",
		      "isVisualizationThread");
	return false;
    }

    return Threads::currentThread()==visualizationthread_;
}


void DataObject::setVisualizationThread(const void* thread)
{
    if ( visualizationthread_ )
    {
	pFreeFnErrMsg("Visualization thread set before.",
		      "setVisualizationThread");
	return;
    }

    visualizationthread_ = thread;
}



void DataObject::setCommonViewer( osgViewer::CompositeViewer* nv )
{
    commonviewer_ = nv;
}


void DataObject::requestSingleRedraw()
{
    if ( commonviewer_ && commonviewer_->getNumViews() )
    {
        commonviewer_->getView(0)->requestRedraw();
    }
}




