/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/


#include "visdata.h"
#include "visdatagroup.h"

#include "filepath.h"
#include "keystrs.h"
#include "mousecursor.h"
#include "staticstring.h"
#include "thread.h"
#include "uistring.h"
#include "visusershowwaitimpl.h"
#include "visdataman.h"
#include "visselman.h"
#include "vismaterial.h"

#include <osg/Switch>
#include <osg/ValueObject>
#include <osgDB/WriteFile>
#include <osgViewer/CompositeViewer>
#include <osgGeo/GLInfo>

#if __win__
#define mDefaultPixelDensity	96
#else
#define mDefaultPixelDensity	72
#endif


Threads::ThreadID visBase::DataObject::visualizationthread_ = 0;
osgViewer::CompositeViewer* visBase::DataObject::commonviewer_ = 0;
Notifier<visBase::DataObject> visBase::DataObject::glinfoavailable_( 0 );
static const std::string osg_idkey( sKey::ID() );
static osg::ref_ptr<osgGeo::GLInfo> glinfo_ = 0;


void visBase::DataObject::enableTraversal( unsigned int tt, bool yn )
{
    enabledmask_ = yn
	? (enabledmask_ | tt )
	: (enabledmask_ & ~tt );

    updateNodemask();
}


bool visBase::DataObject::isTraversalEnabled( unsigned int tt ) const
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
			     : uiname_.getString();
}


void visBase::DataObject::setName( const char* nm )
{
    NamedCallBacker::setName( nm );
    if ( osgnode_ )
	osgnode_->setName( nm );
}


uiString visBase::DataObject::uiName() const
{
    return uiname_.isEmpty() ? toUiString( NamedCallBacker::getName() )
			     : uiname_;
}


void visBase::DataObject::setUiName( const uiString& uinm )
{
    uiname_ = uinm;
    if ( osgnode_ )
	osgnode_->setName( toString(uiname_) );
}


visBase::DataObject::DataObject()
    : id_( -1 )
    , enabledmask_( cAllTraversalMask() )
    , osgnode_( 0 )
    , osgoffswitch_( 0 )
    , ison_( true )
    , parent_( 0 )
{
    DM().addObject( this );
}


visBase::DataObject::~DataObject()
{
    DM().removeObject( this );
    while ( nodestates_.size() )
	removeNodeState( nodestates_[0] );

    if ( osgnode_ ) osgnode_->unref();
    if ( osgoffswitch_ ) osgoffswitch_->unref();
}


visBase::Scene* visBase::DataObject::gtScene() const
{
    return parent_ ? parent_->scene() : 0;
}


void visBase::DataObject::doAddNodeState( NodeState* ns )
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


visBase::NodeState* visBase::DataObject::removeNodeState( NodeState* ns )
{
    const int idx = nodestates_.indexOf( ns );
    if ( nodestates_.validIdx(idx) )
    {
	ns->detachStateSet( getStateSet() );
	nodestates_.removeSingle( idx )->unRef();
    }

    return ns;
}


void visBase::DataObject::setPixelDensity( float dpi )
{
    for ( int idx=0; idx<nodestates_.size(); idx++ )
	nodestates_[idx]->setPixelDensity( dpi );
}


float visBase::DataObject::getDefaultPixelDensity()
{ return mDefaultPixelDensity; }


float visBase::DataObject::getPixelDensity() const
{ return getDefaultPixelDensity(); }


visBase::NodeState* visBase::DataObject::getNodeState( int idx )
{
    return nodestates_.validIdx( idx ) ? nodestates_[idx] : 0;
}


osg::StateSet* visBase::DataObject::getStateSet()
{
    return osgnode_ ? osgnode_->getOrCreateStateSet() : 0;
}


void visBase::DataObject::setID( int nid )
{
    id_ = nid;
    updateOsgNodeData();
}


int visBase::DataObject::getID( const osg::Node* node )
{
    if ( node )
    {
	int objid;
	if ( node->getUserValue(osg_idkey,objid) && objid>=0 )
	    return objid;
    }

    return -1;
}


bool visBase::DataObject::turnOn( bool yn )
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
	osgnode_->ref(); //Ensure it is not accidently deleted

	osg::Node::ParentList parents = osgnode_->getParents();
	for ( unsigned int idx=0; idx<parents.size(); idx++ )
	    parents[idx]->replaceChild( osgnode_, osgoffswitch_ );

	osgoffswitch_->addChild( osgnode_ );

	osgnode_->unref();

    }

    requestSingleRedraw();
    return wason;
}


void visBase::DataObject::updateNodemask()
{
    if ( osgnode_ )
	osgnode_->setNodeMask( enabledmask_ );
}


bool visBase::DataObject::isOn() const
{
    return ison_;
}


void visBase::DataObject::setPickable( bool actively, bool passively )
{
    enableTraversal( cDraggerIntersecTraversalMask(), actively );
    enableTraversal( cActiveIntersecTraversalMask(), actively );
    enableTraversal( cPassiveIntersecTraversalMask(), passively );
}


bool visBase::DataObject::isPickable( bool actively ) const
{
    return actively ? isTraversalEnabled( cActiveIntersecTraversalMask() )
		    : isTraversalEnabled( cPassiveIntersecTraversalMask() );
}


osg::Node* visBase::DataObject::osgNode( bool skipswitch )
{
    return ison_ || skipswitch ? osgnode_ : osgoffswitch_;
}


const osg::Node* visBase::DataObject::osgNode( bool skipswitch ) const
{
    return const_cast<DataObject*>(this)->osgNode( skipswitch );
}


void visBase::DataObject::setOsgNodeInternal( osg::Node* osgnode )
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


void visBase::DataObject::updateOsgNodeData()
{
    if ( osgnode_ )
    {
	osgnode_->setName( name() );
	osgnode_->setUserValue( osg_idkey, id() );
    }

    updateNodemask();
}


void visBase::DataObject::select() const
{ DM().selMan().select( id() ); }


void visBase::DataObject::deSelect() const
{ DM().selMan().deSelect( id() ); }


void visBase::DataObject::updateSel() const
{ DM().selMan().updateSel( id() ); }


bool visBase::DataObject::isSelected() const
{
    return selectable() && DM().selMan().selected().isPresent( id());
}


void visBase::DataObject::setDisplayTransformation( const mVisTrans* trans )
{
    if ( trans != getDisplayTransformation() )
	{ pErrMsg("Not implemented"); }
}


bool visBase::DataObject::serialize( const char* filename, bool binary )
{
    if ( !osgNode() )
	return true;

    File::Path fp( filename );
    fp.setExtension( ".osg", true );
    return osgDB::writeNodeFile( *osgNode(), std::string(fp.fullPath()) );
}


bool visBase::DataObject::isVisualizationThread()
{
    if ( !visualizationthread_ )
	{ pFreeFnErrMsg("Visualization thread not set"); return false; }

    return Threads::currentThread()==visualizationthread_;
}


void visBase::DataObject::setVisualizationThread(Threads::ThreadID thread)
{
    if ( visualizationthread_ )
	{ pFreeFnErrMsg("Visualization thread set before."); return; }

    visualizationthread_ = thread;
}


const osgGeo::GLInfo* visBase::DataObject::getGLInfo()
{
    if ( !glinfo_ )
    {
	glinfo_ = osgGeo::GLInfo::get();
	if ( glinfo_ )
	    glinfoavailable_.trigger();
    }

    return glinfo_;
}



void visBase::DataObject::setCommonViewer( osgViewer::CompositeViewer* nv )
{
    commonviewer_ = nv;
}


osgViewer::CompositeViewer* visBase::DataObject::getCommonViewer()
{
    return commonviewer_;
}


void visBase::DataObject::requestSingleRedraw()
{
    if ( commonviewer_ && commonviewer_->getNumViews() )
        commonviewer_->getView(0)->requestRedraw();
}


visBase::UserShowWaitPosterFactory* visBase::UserShowWaitImpl::factory_ = 0;


visBase::UserShowWait::UserShowWait( const DataObject* obj,
				    const uiString& msg, int sbfld )
    : impl_(new UserShowWaitImpl(obj,msg,sbfld)) {}
void visBase::UserShowWait::setMessage( const uiString& msg )
{ impl_->setMessage( msg ); }
void visBase::UserShowWait::readyNow() { impl_->readyNow(); }


visBase::UserShowWaitImpl::UserShowWaitImpl( const DataObject* obj,
					     const uiString& msg, int sbfld )
    : scene_(obj ? obj->scene() : 0)
    , sbfld_(sbfld)
    , poster_(0)
    , mcc_(0)
{
    setMessage( msg );
}


void visBase::UserShowWaitImpl::setMessage( const uiString& msg )
{
    if ( !factory_ )
    {
	if ( !mcc_ )
	    mcc_ = new MouseCursorChanger( MouseCursor::Wait );
    }
    else
    {
	if ( !poster_ )
	    poster_ = factory_->getPoster( scene_, sbfld_ );
	poster_->post( msg );
    }
}


void visBase::UserShowWaitImpl::readyNow()
{
    delete mcc_; mcc_ = 0;
    delete poster_; poster_ = 0;
}
