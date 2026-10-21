/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visvsgdata.h"

#include "visvsgdataman.h"
#include "vsggeoupdater.h"

#include "errmsg.h"
#include "perthreadrepos.h"

#include <vsg/app/Viewer.h>
#include <vsg/core/Value.h>
#include <vsg/nodes/Switch.h>

#if __win__
static float mDefaultPixelDensity = 96.f;
#else
static float mDefaultPixelDensity = 72.f;
#endif

static const char* sKeyVsgID = "ID";
static const char* sKeyVsgName = "Name";

static vsg::observer_ptr<vsg::Viewer>& commonViewer()
{
    static vsg::observer_ptr<vsg::Viewer> viewer;
    return viewer;
}


static vsg::ref_ptr<vsgGeo::Updater>& commonUpdater()
{
    static vsg::ref_ptr<vsgGeo::Updater> updater;
    return updater;
}


namespace dGBVis
{

Threads::ThreadID DataObject::visualizationthread_ = nullptr;

DataObject::DataObject()
    : switch_(vsg::Switch::create())
{
    ref();
    DM().addObject( this );
    unRefNoDelete();
}


DataObject::~DataObject()
{
    DM().removeObject( this );
    releaseNode( switch_.get() );
    releaseNode( node_.get() );
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
    updateVsgNodeData();
}


void DataObject::setName( const char* nm )
{
    uiname_.setEmpty();
    NamedCallBacker::setName( nm );
    updateVsgNodeData();
}


void DataObject::setID( const VisID& newid )
{
    if ( id_.isValid() && id_!=newid )
	DM().updateID( id_, newid );

    id_ = newid;
    updateVsgNodeData();
}


VisID DataObject::getID( const vsg::Node* node )
{
    int objid = -1;
    if ( node && node->getValue(sKeyVsgID,objid) && objid>=0 )
	return VisID( objid );

    return VisID::udf();
}


vsg::Node* DataObject::vsgNode( bool skipswitch )
{
    return skipswitch ? node_.get() : switch_.get();
}


const vsg::Node* DataObject::vsgNode( bool skipswitch ) const
{
    return skipswitch ? node_.get() : switch_.get();
}


bool DataObject::turnOn( bool yn )
{
    const bool wason = ison_;
    if ( wason == yn )
	return wason;

    ison_ = yn;
    applyNodeMask();
    requestSingleRedraw();
    return wason;
}


void DataObject::setPickable( bool actively, bool passively )
{
    activepick_ = actively;
    passivepick_ = passively;
    applyNodeMask();
}


bool DataObject::isPickable( bool actively ) const
{
    return actively ? activepick_ : passivepick_;
}


void DataObject::applyNodeMask()
{
    if ( !switch_ || switch_->children.empty() )
	return;

    vsg::Mask mask = vsg::MASK_OFF;
    if ( ison_ )
    {
	mask = cRenderMask;
	if ( activepick_ )
	    mask |= cActivePickMask;
	if ( passivepick_ )
	    mask |= cPassivePickMask;
    }

    switch_->children[0].mask = mask;
}


void DataObject::setVsgNodeInternal( vsg::Node* newnode )
{
    if ( newnode == node_.get() )
	return;

    releaseNode( node_.get() );
    node_ = newnode;
    switch_->children.clear();
    if ( node_ )
    {
	switch_->addChild( vsg::MASK_OFF, vsg::ref_ptr<vsg::Node>(node_) );
	applyNodeMask();
	compileNode( node_.get() );
    }

    updateVsgNodeData();
    requestSingleRedraw();
}


void DataObject::updateVsgNodeData()
{
    switch_->setValue( sKeyVsgID, id_.asInt() );
    switch_->setValue( sKeyVsgName, getName().buf() );
    if ( node_ )
	node_->setValue( sKeyVsgID, id_.asInt() );
}


void DataObject::setDisplayTransformation( const mDGBVisTrans* trans )
{
    if ( trans!=getDisplayTransformation() )
	pErrMsg("Not implemented");
}


float DataObject::getDefaultPixelDensity()
{
    return mDefaultPixelDensity;
}


void DataObject::setDefaultPixelDensity( float dpi )
{
    mDefaultPixelDensity = dpi;
}


float DataObject::getPixelDensity() const
{
    return getDefaultPixelDensity();
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


void DataObject::setVisualizationThread( Threads::ThreadID thread )
{
    if ( visualizationthread_ )
    {
	pFreeFnErrMsg("Visualization thread set before.");
	return;
    }

    visualizationthread_ = thread;
}


void DataObject::setCommonViewer( vsg::Viewer* viewer )
{
    if ( viewer == commonViewer().ref_ptr().get() )
	return;

    commonViewer() = viewer;
    if ( viewer )
	commonUpdater() = vsgGeo::Updater::install( *viewer );
    else
	commonUpdater() = vsg::ref_ptr<vsgGeo::Updater>();

    commonViewerChanged().trigger();
}


Notifier<DataObject>& DataObject::commonViewerChanged()
{
    mDefineStaticLocalObject( Notifier<DataObject>, theNotif, (nullptr) );
    return theNotif;
}


vsg::Viewer* DataObject::getCommonViewer()
{
    return commonViewer().ref_ptr().get();
}


vsgGeo::Updater* DataObject::getUpdater()
{
    return commonViewer() ? commonUpdater().get() : nullptr;
}


void DataObject::requestSingleRedraw()
{
    auto* requester = dynamic_cast<RedrawRequester*>( getCommonViewer() );
    if ( requester )
	requester->request();
}


void DataObject::compileNode( vsg::Object* obj )
{
    vsgGeo::Updater* updater = getUpdater();
    if ( obj && updater )
	updater->compiler().add( vsg::ref_ptr<vsg::Object>(obj) );
}


void DataObject::releaseNode( vsg::Object* obj )
{
    vsgGeo::Updater* updater = getUpdater();
    if ( obj && updater )
	updater->compiler().release( vsg::ref_ptr<vsg::Object>(obj) );
}

} // namespace dGBVis
