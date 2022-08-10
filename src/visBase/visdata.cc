/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

#include "visdata.h"

#include "filepath.h"
#include "keystrs.h"
#include "thread.h"
#include "visdataman.h"
#include "visselman.h"
#include "vismaterial.h"

#include <osg/Switch>
#include <osg/ValueObject>
#include <osgDB/WriteFile>
#include <osgViewer/CompositeViewer>

#if __win__
static float mDefaultPixelDensity = 96.f;
#else
static float mDefaultPixelDensity = 72.f;
#endif

using namespace visBase;

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


const OD::String& visBase::DataObject::name() const
{
    mDeclStaticString( ret );
    ret = getName();
    return ret;
}


BufferString visBase::DataObject::getName() const
{
    return uiname_.isEmpty() ? NamedCallBacker::getName()
			     : BufferString( uiname_.getFullString() );
}


uiString visBase::DataObject::uiName() const
{
    return uiname_.isEmpty() ? toUiString( NamedCallBacker::getName() )
			     : uiname_;
}


void visBase::DataObject::setUiName( const uiString& uinm )
{
    NamedCallBacker::setName( BufferString::empty() );
    uiname_ = uinm;
    if ( osgnode_ )
        osgnode_->setName( toString(uiname_) );
}


void visBase::DataObject::setName( const char* nm )
{
    uiname_.setEmpty();
    NamedCallBacker::setName( nm );
    if ( osgnode_ )
        osgnode_->setName( nm );
}


DataObject::DataObject()
    : enabledmask_( cAllTraversalMask() )
    , osgnode_( 0 )
    , osgoffswitch_( 0 )
    , ison_( true )
{
    DM().addObject( this );
}


DataObject::~DataObject()
{
    DM().removeObject( this );
    while ( nodestates_.size() )
	removeNodeState( nodestates_[0] );

    if ( osgnode_ ) osgnode_->unref();
    if ( osgoffswitch_ ) osgoffswitch_->unref();
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
    return nodestates_.validIdx( idx ) ? nodestates_[idx] : 0;
}


osg::StateSet* DataObject::getStateSet()
{
    return osgnode_ ? osgnode_->getOrCreateStateSet() : 0;
}


void DataObject::setID( VisID newid )
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
	osgoffswitch_->ref();
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
    if ( osgnode ) osgnode->ref();
    if ( osgnode_ ) osgnode_->unref();
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
    {
        commonviewer_->getView(0)->requestRedraw();
    }
}
