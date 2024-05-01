/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visdata.h"

#include "filepath.h"
#include "keystrs.h"
#include "thread.h"
#include "visdataman.h"
#include "vismaterial.h"
#include "visselman.h"

#include <osg/Switch>
#include <osg/ValueObject>
#include <osgDB/WriteFile>
#include <osgViewer/CompositeViewer>

#if __win__
static float mDefaultPixelDensity = 96.f;
#else
static float mDefaultPixelDensity = 72.f;
#endif

namespace visBase
{

const void* DataObject::visualizationthread_ = nullptr;
osgViewer::CompositeViewer* DataObject::commonviewer_ = nullptr;


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


const OD::String& DataObject::name() const
{
    mDeclStaticString( ret );
    ret = getName();
    return ret;
}


BufferString DataObject::getName() const
{
    return uiname_.isEmpty() ? NamedCallBacker::getName()
			     : BufferString( uiname_.getFullString() );
}


uiString DataObject::uiName() const
{
    return uiname_.isEmpty() ? toUiString( NamedCallBacker::getName() )
			     : uiname_;
}


void DataObject::setUiName( const uiString& uinm )
{
    NamedCallBacker::setName( BufferString::empty() );
    uiname_ = uinm;
    if ( osgnode_ )
        osgnode_->setName( toString(uiname_) );
}


void DataObject::setName( const char* nm )
{
    uiname_.setEmpty();
    NamedCallBacker::setName( nm );
    if ( osgnode_ )
        osgnode_->setName( nm );
}


DataObject::DataObject()
    : enabledmask_(cAllTraversalMask())
{
    ref();
    DM().addObject( this );
    unRefNoDelete();
}


DataObject::~DataObject()
{
    DM().removeObject( this );
    osg::ref_ptr<osg::StateSet> stateset = getStateSet();
    for ( auto* nodestate : nodestates_ )
	nodestate->detachStateSet( stateset );

    stateset = nullptr;
    unRefOsgPtr( osgnode_ );
    unRefOsgPtr( osgoffswitch_ );
}


void DataObject::doAddNodeState( NodeState* ns )
{
    nodestates_ += ns;
    osg::ref_ptr<osg::StateSet> stateset = getStateSet();
    if ( !stateset )
    {
	pErrMsg("Setting nodestate on class without stateset.");
	return;
    }

    ns->attachStateSet( stateset );
}


NodeState* DataObject::removeNodeState( NodeState* ns )
{
    const int idx = nodestates_.indexOf( ns );
    if ( nodestates_.validIdx(idx) )
    {
	ns->detachStateSet( getStateSet() );
	nodestates_ -= ns;
    }

    return ns;
}


void DataObject::setPixelDensity( float dpi )
{
    for ( int idx=0; idx<nodestates_.size(); idx++ )
	nodestates_[idx]->setPixelDensity( dpi );
}


float DataObject::getDefaultPixelDensity()
{ return mDefaultPixelDensity; }

void DataObject::setDefaultPixelDensity( float dpi )
{ mDefaultPixelDensity = dpi; }


float DataObject::getPixelDensity() const
{ return getDefaultPixelDensity(); }


NodeState* DataObject::getNodeState( int idx )
{
    return nodestates_.validIdx( idx ) ? nodestates_[idx] : nullptr;
}


osg::StateSet* DataObject::getStateSet()
{
    return osgnode_ ? osgnode_->getOrCreateStateSet() : nullptr;
}


void DataObject::setID( const VisID& newid )
{
    id_ = newid;
    updateOsgNodeData();
}

std::string idstr( sKey::ID() );

VisID DataObject::getID( const osg::Node* node )
{
    if ( node )
    {
	int objid;
	if ( node->getUserValue(idstr,objid) && objid>=0 )
	    return VisID(objid);
    }

    return VisID::udf();
}


bool DataObject::turnOn( bool yn )
{
    const bool wason = ison_;

    if ( wason == yn )
	return wason;

    ison_ = yn;

    if ( !osgoffswitch_ )
    {
	osgoffswitch_ = new osg::Switch;
	refOsgPtr( osgoffswitch_ );
	osgoffswitch_->setAllChildrenOff();
    }

    if ( ison_ )
    {
	osgoffswitch_->removeChildren( 0, osgoffswitch_->getNumChildren() );

	osg::Node::ParentList parents = osgoffswitch_->getParents();
	for ( unsigned int idx=0; idx<parents.size(); idx++ )
	{
	    if ( osgnode_ )
		parents[idx]->replaceChild( osgoffswitch_, osgnode_ );
	    else
		parents[idx]->removeChild( osgoffswitch_ );
	}
    }
    else if ( osgnode_ )
    {
	osg::Node::ParentList parents = osgnode_->getParents();
	for ( unsigned int idx=0; idx<parents.size(); idx++ )
	    parents[idx]->replaceChild( osgnode_, osgoffswitch_ );

	osgoffswitch_->addChild( osgnode_ );

    }

    requestSingleRedraw();
    return wason;
}


void DataObject::updateNodemask()
{
    if ( osgnode_ )
	osgnode_->setNodeMask( enabledmask_ );
}


bool DataObject::isOn() const
{
    return ison_;
}


void DataObject::setPickable( bool actively, bool passively )
{
    enableTraversal( cDraggerIntersecTraversalMask(), actively );
    enableTraversal( cActiveIntersecTraversalMask(), actively );
    enableTraversal( cPassiveIntersecTraversalMask(), passively );
}


bool DataObject::isPickable( bool actively ) const
{
    return actively ? isTraversalEnabled( cActiveIntersecTraversalMask() )
		    : isTraversalEnabled( cPassiveIntersecTraversalMask() );
}


osg::Node* DataObject::osgNode( bool skipswitch )
{
    return ison_ || skipswitch ? osgnode_ : osgoffswitch_;
}


const osg::Node* DataObject::osgNode( bool skipswitch ) const
{
    return const_cast<DataObject*>(this)->osgNode( skipswitch );
}


void DataObject::setOsgNodeInternal( osg::Node* osgnode )
{
    //Do this reverse order as osgnode may be a child of osgnode_
    refOsgPtr( osgnode );
    unRefOsgPtr( osgnode_ );
    osgnode_ = osgnode;
    updateOsgNodeData();

    if ( osgoffswitch_ )
    {
	osgoffswitch_->removeChildren( 0, osgoffswitch_->getNumChildren() );
	if ( !ison_ && osgnode_ )
	    osgoffswitch_->addChild( osgnode_ );
    }
}


void DataObject::updateOsgNodeData()
{
    if ( osgnode_ )
    {
	osgnode_->setName( name_ );
	osgnode_->setUserValue( idstr, id().asInt() );
    }

    updateNodemask();
}


void DataObject::select() const
{ DM().selMan().select( id() ); }


void DataObject::deSelect() const
{ DM().selMan().deSelect( id() ); }


void DataObject::updateSel() const
{ DM().selMan().updateSel( id() ); }


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

    FilePath fp( filename );
    fp.setExtension( ".osg", true );
    return osgDB::writeNodeFile( *osgNode(), std::string(fp.fullPath()) );
}


bool DataObject::isVisualizationThread()
{
    if ( !visualizationthread_ )
    {
	pFreeFnErrMsg("Visualization thread not set");
	return false;
    }

    return Threads::currentThread()==visualizationthread_;
}


void DataObject::setVisualizationThread(const void* thread)
{
    if ( visualizationthread_ )
    {
	pFreeFnErrMsg("Visualization thread set before.");
	return;
    }

    visualizationthread_ = thread;
}



void DataObject::setCommonViewer( osgViewer::CompositeViewer* nv )
{
    commonviewer_ = nv;
}


osgViewer::CompositeViewer* DataObject::getCommonViewer()
{
    return commonviewer_;
}


void DataObject::requestSingleRedraw()
{
    if ( commonviewer_ && commonviewer_->getNumViews() )
        commonviewer_->getView(0)->requestRedraw();
}

} // namespace visBase
