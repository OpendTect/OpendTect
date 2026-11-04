/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodviewer2dmgr.h"

#include "uiflatviewer.h"
#include "uiflatviewwin.h"
#include "uimsg.h"
#include "uiodviewer2d.h"

#include "commondefs.h"
#include "ioman.h"
#include "seisdatapack.h"
#include "view2ddata.h"
#include "view2dfault.h"
#include "view2dfaultss2d.h"
#include "view2dfaultss3d.h"
#include "view2dhorizon2d.h"
#include "view2dhorizon3d.h"
#include "view2dpickset.h"
#include "view2dseismic.h"

using OD2DViewer::ObjectKind2DV;
using OD2DViewer::Object2DVInfo;
using OD2DViewer::Viewer2DInfo;
using OD2DViewer::OpenResult2DV;
using OD2DViewer::OpenRequestStatus2DV;

/*! \brief Adapts native viewer operations to the common backend interface.

    The manager owns both this adapter and the native viewers. Opening calls
    the manager's private native methods directly, avoiding recursion through
    its public dispatch methods.
*/
class uiODNative2DViewerBackend final : public uiOD2DViewerBackend
{
public:
    explicit		uiODNative2DViewerBackend(uiODViewer2DMgr&);

    OpenResult2DV	open(const VisID&,int,
			     FlatView::Viewer::VwrDest) override;
    OpenResult2DV	open(const VolumeDataPack&,const Attrib::SelSpec&,
			     const FlatView::DataDispPars::VD&,
			     FlatView::Viewer::VwrDest) override;
    OpenResult2DV	open(Viewer2DPosDataSel&,
			     FlatView::Viewer::VwrDest,float,float) override;

    void		getViewerIDs(TypeSet<Viewer2DID>&) const override;
    bool		getViewerInfo(Viewer2DID,Viewer2DInfo&) const override;
    void		getViewerObjects(Viewer2DID,
				 TypeSet<Vis2DID>&) const override;
    bool		getObjectInfo(Vis2DID,Object2DVInfo&) const override;
    bool		setPosition(Viewer2DID,const TrcKeyZSampling&) override;
    bool		updateSeismic(Viewer2DID,const VolumeDataPack&,
				      const Attrib::SelSpec&,
				      const FlatView::DataDispPars::VD&,
				      FlatView::Viewer::VwrDest) override;

    bool		prepareClose(const TypeSet<Viewer2DID>&,bool) override;
    void		commitClose(const TypeSet<Viewer2DID>&) override;
    void		cancelClosePreparation() override;

    bool		addObjects(ObjectKind2DV,const TypeSet<EM::ObjectID>&,
				   bool) override;
    void		removeObject(ObjectKind2DV,
			     const EM::ObjectID&) override;
    void		addPickSets(const TypeSet<MultiID>&) override;
    void		removePickSet(const MultiID&) override;

private:
    static OpenResult2DV result(Viewer2DID);

    uiODViewer2DMgr&	mgr_;
};


uiODNative2DViewerBackend::uiODNative2DViewerBackend( uiODViewer2DMgr& mgr )
    : mgr_(mgr)
{}


OpenResult2DV uiODNative2DViewerBackend::open( const VisID& id,int attr,
					    FlatView::Viewer::VwrDest dest )
{
    mgr_.nativeOpen( id, attr, dest );
    const auto* vwr = mgr_.find2DViewer( id );
    return result( vwr ? vwr->ID() : Viewer2DID::udf() );
}


OpenResult2DV uiODNative2DViewerBackend::open( const VolumeDataPack& dp,
	const Attrib::SelSpec& ss,const FlatView::DataDispPars::VD& pars,
	FlatView::Viewer::VwrDest dest )
{
    return result( mgr_.nativeOpen(dp,ss,pars,dest) );
}


OpenResult2DV uiODNative2DViewerBackend::open( Viewer2DPosDataSel& sel,
	FlatView::Viewer::VwrDest dest,float x1,float x2 )
{
    return result( mgr_.nativeOpen(sel,dest,x1,x2) );
}


void uiODNative2DViewerBackend::getViewerIDs( TypeSet<Viewer2DID>& ids ) const
{
    for ( const auto* vwr : mgr_.viewers2d_ )
	ids.addIfNew( vwr->ID() );
}


bool uiODNative2DViewerBackend::getViewerInfo( Viewer2DID id,
					     Viewer2DInfo& info ) const
{
    const auto* vwr = mgr_.find2DViewer( id );
    if ( !vwr || !vwr->viewwin() )
	return false;

    info.id_ = id;
    info.sourceid_ = vwr->visID();
    info.sceneid_ = vwr->getSyncSceneID();
    info.sampling_ = vwr->getTrcKeyZSampling();
    info.geomid_ = vwr->geomID();
    info.randomlineid_ = vwr->getRandomLineID();
    info.selspec_ = vwr->selSpec( false );
    info.disppars_ = vwr->viewwin()->viewer().appearance().ddpars_.vd_;
    return true;
}


void uiODNative2DViewerBackend::getViewerObjects( Viewer2DID id,
						 TypeSet<Vis2DID>& ids ) const
{
    const auto* vwr = mgr_.find2DViewer( id );
    if ( vwr )
	vwr->getVwr2DObjIDs( ids );
}


bool uiODNative2DViewerBackend::getObjectInfo( Vis2DID id,
					     Object2DVInfo& info ) const
{
    const auto* vwr = mgr_.getParent2DViewer( id );
    const auto* obj = vwr ? vwr->getObject(id) : nullptr;
    if ( !obj )
	return false;

    info = Object2DVInfo();
    info.id_ = id;
    info.viewerid_ = vwr->ID();
    mDynamicCastGet(const View2D::PickSet*,pickset,obj)
    if ( dCast(const View2D::Seismic*,obj) )
	info.kind_ = ObjectKind2DV::Seismic;
    else if ( dCast(const View2D::Horizon3D*,obj) )
	info.kind_ = ObjectKind2DV::Horizon3D;
    else if ( dCast(const View2D::Horizon2D*,obj) )
	info.kind_ = ObjectKind2DV::Horizon2D;
    else if ( dCast(const View2D::Fault*,obj) )
	info.kind_ = ObjectKind2DV::Fault;
    else if ( dCast(const View2D::FaultSS2D*,obj) )
	info.kind_ = ObjectKind2DV::FaultSS2D;
    else if ( dCast(const View2D::FaultSS3D*,obj) )
	info.kind_ = ObjectKind2DV::FaultSS3D;
    else if ( pickset )
    {
	info.kind_ = ObjectKind2DV::PickSet;
	info.multiid_ = pickset->pickSetID();
    }

    mDynamicCastGet(const View2D::EMDataObject*,emobj,obj)
    if ( emobj )
	info.emid_ = emobj->getEMObjectID();

    return true;
}


bool uiODNative2DViewerBackend::setPosition( Viewer2DID id,
					   const TrcKeyZSampling& sampling )
{
    auto* vwr = mgr_.find2DViewer( id );
    if ( !vwr )
	return false;

    vwr->setPos( sampling );
    return true;
}


bool uiODNative2DViewerBackend::updateSeismic( Viewer2DID id,
	const VolumeDataPack& dp,const Attrib::SelSpec& ss,
	const FlatView::DataDispPars::VD& pars,FlatView::Viewer::VwrDest dest )
{
    auto* vwr = mgr_.find2DViewer( id );
    if ( !vwr || !vwr->viewwin() )
	return false;

    mDynamicCastGet(const RegularSeisDataPack*,regular,&dp)
    if ( regular )
	vwr->setTrcKeyZSampling( regular->sampling() );

    vwr->setSelSpec( &ss, dest );
    vwr->makeUpView( vwr->createFlatDataPackRM(dp,0).ptr(), dest );
    auto& viewer = vwr->viewwin()->viewer();
    viewer.appearance().ddpars_.vd_ = pars;
    viewer.handleChange( FlatView::Viewer::DisplayPars );
    return true;
}


bool uiODNative2DViewerBackend::prepareClose( const TypeSet<Viewer2DID>&,bool )
{
    return true;
}


void uiODNative2DViewerBackend::commitClose( const TypeSet<Viewer2DID>& ids )
{
    for ( const auto& id : ids )
	mgr_.remove2DViewer( id.asInt(), false );
}


void uiODNative2DViewerBackend::cancelClosePreparation()
{}


bool uiODNative2DViewerBackend::addObjects( ObjectKind2DV kind,
	const TypeSet<EM::ObjectID>& ids,bool )
{
    // Native semantic methods retain their historic scene side effects.
    switch ( kind )
    {
    case ObjectKind2DV::Horizon3D:
	mgr_.addHorizon3Ds( ids );
	break;
    case ObjectKind2DV::Horizon2D:
	mgr_.addHorizon2Ds( ids );
	break;
    case ObjectKind2DV::Fault:
	mgr_.addFaults( ids );
	break;
    case ObjectKind2DV::FaultSS2D:
	mgr_.addFaultSS2Ds( ids );
	break;
    case ObjectKind2DV::FaultSS3D:
	mgr_.addFaultSSs( ids );
	break;
    default:
	return false;
    }

    return true;
}


void uiODNative2DViewerBackend::removeObject( ObjectKind2DV kind,
					    const EM::ObjectID& id )
{
    switch ( kind )
    {
    case ObjectKind2DV::Horizon3D:
	mgr_.removeHorizon3D( id );
	break;
    case ObjectKind2DV::Horizon2D:
	mgr_.removeHorizon2D( id );
	break;
    case ObjectKind2DV::Fault:
	mgr_.removeFault( id );
	break;
    case ObjectKind2DV::FaultSS2D:
	mgr_.removeFaultSS2D( id );
	break;
    case ObjectKind2DV::FaultSS3D:
	mgr_.removeFaultSS( id );
	break;
    default:
	break;
    }
}


void uiODNative2DViewerBackend::addPickSets( const TypeSet<MultiID>& ids )
{
    mgr_.addPickSets( ids );
}


void uiODNative2DViewerBackend::removePickSet( const MultiID& id )
{
    mgr_.removePickSet( id );
}


OpenResult2DV uiODNative2DViewerBackend::result( Viewer2DID id )
{
    OpenResult2DV res;
    res.id_ = id;
    if ( id.isValid() )
    {
	res.status_ = OpenRequestStatus2DV::Opened;
	res.message_.setEmpty();
    }
    else
    {
	res.status_ = OpenRequestStatus2DV::InvalidData;
	res.message_ = toUiString( "No usable seismic data is available." );
    }

    return res;
}


void uiODViewer2DMgr::initBackends()
{
    nativebackend_ = new uiODNative2DViewerBackend( *this );
    mAttachCB( IOM().afterSurveyChange, uiODViewer2DMgr::afterSurveyChangeCB );
}


void uiODViewer2DMgr::destroyBackends()
{
    if ( advancedbackend_ )
    {
	TypeSet<Viewer2DID> ids;
	advancedbackend_->getViewerIDs( ids );
	advancedbackend_->prepareClose( ids, true );
	advancedbackend_->commitClose( ids );
	unregisterBackend( *advancedbackend_ );
    }

    delete nativebackend_;
    nativebackend_ = nullptr;
}


void uiODViewer2DMgr::registerBackend( uiOD2DViewerBackend& backend )
{
    if ( advancedbackend_==&backend || advancedbackend_ )
	return;

    advancedbackend_ = &backend;
    mAttachCB( backend.objectAdded, uiODViewer2DMgr::backendObjectAdded );
    mAttachCB( backend.objectToBeRemoved,
	       uiODViewer2DMgr::backendObjectRemoved );
    mAttachCB( backend.positionChanged,
	       uiODViewer2DMgr::backendPositionChanged );
    mAttachCB( backend.viewerClosed, uiODViewer2DMgr::backendViewerClosed );
}


void uiODViewer2DMgr::unregisterBackend( uiOD2DViewerBackend& backend )
{
    if ( advancedbackend_!=&backend )
	return;

    mDetachCB( backend.objectAdded, uiODViewer2DMgr::backendObjectAdded );
    mDetachCB( backend.objectToBeRemoved,
	       uiODViewer2DMgr::backendObjectRemoved );
    mDetachCB( backend.positionChanged,
	       uiODViewer2DMgr::backendPositionChanged );
    mDetachCB( backend.viewerClosed, uiODViewer2DMgr::backendViewerClosed );
    advancedbackend_ = nullptr;
}


void uiODViewer2DMgr::setAdvancedRequired( bool required )
{
    if ( required==advancedrequired_ )
	return;

    // Authority changes occur at startup or after committed survey teardown.
    // Registration alone can never enable the native fallback.
    advancedrequired_ = required;
}


uiOD2DViewerBackend* uiODViewer2DMgr::selectedBackend() const
{
    return advancedrequired_ ? advancedbackend_ : nativebackend_;
}


uiOD2DViewerBackend* uiODViewer2DMgr::readyBackend() const
{
    return dispatchsuspended_ ? nullptr : selectedBackend();
}


void uiODViewer2DMgr::suspendDispatch()
{
    dispatchsuspended_ = true;
}


void uiODViewer2DMgr::resumeDispatch()
{
    dispatchsuspended_ = false;
}


void uiODViewer2DMgr::afterSurveyChangeCB( CallBacker* )
{
    CallBack::addToMainThread( mCB(this,uiODViewer2DMgr,resumeDispatchCB) );
}


void uiODViewer2DMgr::resumeDispatchCB( CallBacker* )
{
    resumeDispatch();
}


static OpenResult2DV unavailableResult()
{
    OpenResult2DV res;
    res.status_ = OpenRequestStatus2DV::NoBackend;
    res.message_ = toUiString( "The selected 2D viewer is unavailable or is "
			      "being initialized." );
    return res;
}


OpenResult2DV uiODViewer2DMgr::openViewer( const VisID& id,int attr,
					 FlatView::Viewer::VwrDest dest )
{
    auto* backend = readyBackend();
    return backend ? backend->open(id,attr,dest) : unavailableResult();
}


OpenResult2DV uiODViewer2DMgr::openViewer( const VolumeDataPack& dp,
	const Attrib::SelSpec& ss,const FlatView::DataDispPars::VD& pars,
	FlatView::Viewer::VwrDest dest )
{
    auto* backend = readyBackend();
    return backend ? backend->open(dp,ss,pars,dest) : unavailableResult();
}


OpenResult2DV uiODViewer2DMgr::openViewer( Viewer2DPosDataSel& sel,
	FlatView::Viewer::VwrDest dest,float x1,float x2 )
{
    auto* backend = readyBackend();
    return backend ? backend->open(sel,dest,x1,x2) : unavailableResult();
}


void uiODViewer2DMgr::reportResult( const OpenResult2DV& result ) const
{
    // usePar supplies one aggregate Pro layout-restoration notice.
    if ( advancedrequired_ && appl_.isRestoringSession() )
	return;

    if ( !result.isOK() && result.status_!=OpenRequestStatus2DV::Cancelled &&
	 !result.message_.isEmpty() )
	uiMSG().error( result.message_ );
}


void uiODViewer2DMgr::getViewerIDs( TypeSet<Viewer2DID>& ids ) const
{
    const auto* backend = selectedBackend();
    if ( backend )
	backend->getViewerIDs( ids );
}


bool uiODViewer2DMgr::getViewerInfo( Viewer2DID id,Viewer2DInfo& info ) const
{
    const auto* backend = selectedBackend();
    if ( !backend || !backend->getViewerInfo(id,info) )
	return false;

    const int idx = associatedviewers_.indexOf( id );
    if ( !info.sourceid_.isValid() && idx>=0 )
	info.sourceid_ = associatedsources_[idx];

    return true;
}


void uiODViewer2DMgr::associateViewerWithSource( Viewer2DID id,
					       const VisID& source )
{
    Viewer2DInfo info;
    if ( !id.isValid() || !source.isValid() || !getViewerInfo(id,info) ||
	 info.sourceid_.isValid() )
	return;

    associatedviewers_ += id;
    associatedsources_ += source;
}


void uiODViewer2DMgr::forgetViewerSource( Viewer2DID id )
{
    const int idx = associatedviewers_.indexOf( id );
    if ( idx<0 )
	return;

    associatedviewers_.removeSingle( idx );
    associatedsources_.removeSingle( idx );
}


void uiODViewer2DMgr::getViewerObjects( Viewer2DID id,
				       TypeSet<Vis2DID>& ids ) const
{
    const auto* backend = selectedBackend();
    if ( backend )
	backend->getViewerObjects( id, ids );
}


bool uiODViewer2DMgr::getObjectInfo( Vis2DID id,Object2DVInfo& info ) const
{
    const auto* backend = selectedBackend();
    return backend && backend->getObjectInfo( id, info );
}


bool uiODViewer2DMgr::setViewerPosition( Viewer2DID id,
				       const TrcKeyZSampling& sampling )
{
    auto* backend = readyBackend();
    return backend && backend->setPosition( id, sampling );
}


bool uiODViewer2DMgr::updateViewerSeismic( Viewer2DID id,
	const VolumeDataPack& dp,const Attrib::SelSpec& ss,
	const FlatView::DataDispPars::VD& pars,FlatView::Viewer::VwrDest dest )
{
    auto* backend = readyBackend();
    return backend && backend->updateSeismic( id, dp, ss, pars, dest );
}


bool uiODViewer2DMgr::closeViewer( Viewer2DID id )
{
    auto* backend = selectedBackend();
    if ( !backend )
	return false;

    TypeSet<Viewer2DID> ids;
    ids += id;
    if ( !backend->prepareClose(ids,false) )
    {
	backend->cancelClosePreparation();
	return false;
    }

    backend->commitClose( ids );
    return true;
}


void uiODViewer2DMgr::getSourcesViewers( const TypeSet<VisID>& sources,
	TypeSet<Viewer2DID>& ids,const uiOD2DViewerBackend& backend ) const
{
    TypeSet<Viewer2DID> all;
    backend.getViewerIDs( all );
    for ( const auto& id : all )
    {
	Viewer2DInfo info;
	if ( !backend.getViewerInfo(id,info) )
	    continue;

	const int idx = associatedviewers_.indexOf( id );
	if ( !info.sourceid_.isValid() && idx>=0 )
	    info.sourceid_ = associatedsources_[idx];

	if ( sources.isPresent(info.sourceid_) )
	    ids += id;
    }
}


bool uiODViewer2DMgr::prepareCloseViewers( const TypeSet<Viewer2DID>& ids,
					  bool saved )
{
    auto* backend = selectedBackend();
    if ( !backend || backend->prepareClose(ids,saved) )
	return true;

    backend->cancelClosePreparation();
    return false;
}


bool uiODViewer2DMgr::prepareCloseForSource( const VisID& source )
{
    TypeSet<VisID> sources;
    sources += source;
    return prepareCloseForSources( sources );
}


bool uiODViewer2DMgr::prepareCloseForSources( const TypeSet<VisID>& sources )
{
    for ( auto* backend : {nativebackend_,advancedbackend_} )
    {
	if ( !backend )
	    continue;

	TypeSet<Viewer2DID> ids;
	getSourcesViewers( sources, ids, *backend );
	if ( backend->prepareClose(ids,false) )
	    continue;

	cancelClosePreparation();
	return false;
    }

    return true;
}


bool uiODViewer2DMgr::prepareCloseAll( bool saved )
{
    // Explicit Advanced actions can precede generic Pro dispatch activation.
    // Preflight every owned window before any backend commits teardown.
    for ( auto* backend : {nativebackend_,advancedbackend_} )
    {
	if ( !backend )
	    continue;

	TypeSet<Viewer2DID> ids;
	backend->getViewerIDs( ids );
	if ( backend->prepareClose(ids,saved) )
	    continue;

	cancelClosePreparation();
	return false;
    }

    return true;
}


void uiODViewer2DMgr::commitCloseForSource( const VisID& source )
{
    TypeSet<VisID> sources;
    sources += source;
    commitCloseForSources( sources );
}


void uiODViewer2DMgr::commitCloseForSources( const TypeSet<VisID>& sources )
{
    for ( auto* backend : {nativebackend_,advancedbackend_} )
    {
	if ( !backend )
	    continue;

	TypeSet<Viewer2DID> ids;
	getSourcesViewers( sources, ids, *backend );
	backend->commitClose( ids );
    }
}


void uiODViewer2DMgr::commitCloseAll()
{
    // Both collections must be empty before unloading feature code or surveys.
    for ( auto* backend : {nativebackend_,advancedbackend_} )
    {
	if ( !backend )
	    continue;

	TypeSet<Viewer2DID> ids;
	backend->getViewerIDs( ids );
	backend->commitClose( ids );
    }
}


void uiODViewer2DMgr::cancelClosePreparation()
{
    for ( auto* backend : {nativebackend_,advancedbackend_} )
    {
	if ( backend )
	    backend->cancelClosePreparation();
    }
}


void uiODViewer2DMgr::backendObjectAdded( CallBacker* cb )
{
    mCBCapsuleUnpack(Vis2DID,id,cb);
    if ( advancedrequired_ )
	vw2dObjAdded.trigger( id );
}


void uiODViewer2DMgr::backendObjectRemoved( CallBacker* cb )
{
    mCBCapsuleUnpack(Vis2DID,id,cb);
    vw2dObjToBeRemoved.trigger( id );
}


void uiODViewer2DMgr::backendPositionChanged( CallBacker* cb )
{
    mCBCapsuleUnpack(Viewer2DID,id,cb);
    viewerPositionChanged.trigger( id );
}


void uiODViewer2DMgr::backendViewerClosed( CallBacker* cb )
{
    mCBCapsuleUnpack(Viewer2DID,id,cb);
    forgetViewerSource( id );
    viewerClosed.trigger( id );
}


void uiODViewer2DMgr::nativePositionChanged( CallBacker* cb )
{
    mDynamicCastGet(const uiODViewer2D*,vwr,cb)
    if ( vwr && !advancedrequired_ )
	viewerPositionChanged.trigger( vwr->ID() );
}


void uiODViewer2DMgr::getBackendObjectIDs( ObjectKind2DV kind,
	const EM::ObjectID& emid,TypeSet<Vis2DID>& result ) const
{
    TypeSet<Viewer2DID> viewers;
    getViewerIDs( viewers );
    for ( const auto& viewer : viewers )
    {
	TypeSet<Vis2DID> ids;
	getViewerObjects( viewer, ids );
	for ( const auto& id : ids )
	{
	    Object2DVInfo info;
	    if ( getObjectInfo(id,info) && info.kind_==kind &&
		 info.emid_==emid )
		result.addIfNew( id );
	}
    }
}


void uiODViewer2DMgr::getBackendLoaded( ObjectKind2DV kind,
				       TypeSet<EM::ObjectID>& result ) const
{
    TypeSet<Viewer2DID> viewers;
    getViewerIDs( viewers );
    for ( const auto& viewer : viewers )
    {
	TypeSet<Vis2DID> ids;
	getViewerObjects( viewer, ids );
	for ( const auto& id : ids )
	{
	    Object2DVInfo info;
	    if ( getObjectInfo(id,info) && info.kind_==kind &&
		 info.emid_.isValid() )
		result.addIfNew( info.emid_ );
	}
    }
}
