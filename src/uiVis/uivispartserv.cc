/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivispartserv.h"

#include "attribsel.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "envvars.h"
#include "flatview.h"
#include "genc.h"
#include "ioman.h"
#include "iopar.h"
#include "mousecursor.h"
#include "mouseevent.h"
#include "od_helpids.h"
#include "oddirs.h"
#include "seisdatapack.h"
#include "separstr.h"
#include "survinfo.h"
#include "zaxistransform.h"
#include "zdomain.h"

#include "uiattribtransdlg.h"
#include "uifiledlg.h"
#include "uimain.h"
#include "uimapperrangeeditordlg.h"
#include "uimaterialdlg.h"
#include "uimpeman.h"
#include "uimsg.h"
#include "uiposprovider.h"
#include "uiscenecolorbarmgr.h"
#include "uiselsurvranges.h"
#include "uisurvtopbotimg.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"
#include "uivisdirlightdlg.h"
#include "uivisslicepos3d.h"
#include "uiviszstretchdlg.h"

#include "visdataman.h"
#include "visevent.h"
#include "vishorizondisplay.h"
#include "vishorizon2ddisplay.h"
#include "vismpeseedcatcher.h"
#include "visobject.h"
#include "visplanedatadisplay.h"
#include "visrandomtrackdisplay.h"
#include "visseis2ddisplay.h"
#include "visscenecoltab.h"
#include "visselman.h"
#include "vistexturechannels.h"
#include "visvolumedisplay.h"


int uiVisPartServer::evUpdateTree()			{ return 0; }
int uiVisPartServer::evSelection()			{ return 1; }
int uiVisPartServer::evDeSelection()			{ return 2; }
int uiVisPartServer::evGetNewData()			{ return 3; }
int uiVisPartServer::evMouseMove()			{ return 4; }
int uiVisPartServer::evInteraction()			{ return 5; }
int uiVisPartServer::evSelectAttrib()			{ return 6; }
int uiVisPartServer::evKeyboardEvent()			{ return 7; }
int uiVisPartServer::evMouseEvent()			{ return 8; }
int uiVisPartServer::evViewAll()			{ return 9; }
int uiVisPartServer::evToHomePos()			{ return 10; }
int uiVisPartServer::evPickingStatusChange()		{ return 11; }
int uiVisPartServer::evViewModeChange()			{ return 12; }
int uiVisPartServer::evShowMPESetupDlg()		{ return 13; }
int uiVisPartServer::evShowMPEParentPath()		{ return 14; }
int uiVisPartServer::evDisableSelTracker()		{ return 16; }
int uiVisPartServer::evColorTableChange()		{ return 17; }
int uiVisPartServer::evStoreEMObject()			{ return 19; }
int uiVisPartServer::evStoreEMObjectAs()		{ return 20; }
int uiVisPartServer::evShowSetupGroupOnTop()		{ return 21; }

const char* uiVisPartServer::sKeyAppVel()	{ return "AppVel"; }
const char* uiVisPartServer::sKeyWorkArea()	{ return "Work Area"; }
static const char* sKeyNumberScenes()		{ return "Number of Scene";}
static const char* sKeySceneID()		{ return "Scene ID"; }
static const char* sKeySliceSteps()		{ return "Slice Steps"; }
static const char* sceneprefix()		{ return "Scene"; }


static const int cResetManipIdx = 800;
static const int cPropertiesIdx = 600;
static const int cResolutionIdx = 500;


uiVisPartServer::uiVisPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , objectAdded(this)
    , objectRemoved(this)
    , keyEvent(this)
    , mouseEvent(this)
    , selectionmodeChange(this)
    , planeMovedEvent(this)
    , menu_(new uiMenuHandler(appserv().parent(),-1))
    , eventmutex_(*new Threads::Mutex)
    , resetmanipmnuitem_(tr("Reset Manipulation"),cResetManipIdx)
    , changematerialmnuitem_(m3Dots(uiStrings::sProperties()),
			     cPropertiesIdx)
    , resmnuitem_(tr("Resolution"),cResolutionIdx)
    , pickretriever_(new uiVisPickRetriever(this))
    , nrscenesChange(this)
{
    changematerialmnuitem_.iconfnm = "disppars";

    mAttachCB( menu_->createnotifier, uiVisPartServer::createMenuCB );
    mAttachCB( menu_->handlenotifier, uiVisPartServer::handleMenuCB );

    mAttachCB( visBase::DM().selMan().selnotifier,
	       uiVisPartServer::selectObjCB );
    mAttachCB( visBase::DM().selMan().deselnotifier,
	       uiVisPartServer::deselectObjCB );
    mAttachCB( visBase::DM().selMan().updateselnotifier,
	       uiVisPartServer::updateSelObjCB );
    mAttachCB( IOM().implUpdated, uiVisPartServer::datasetUpdatedCB );

    vismgr_ = new uiVisModeMgr(this);
    PickRetriever::instance( pickretriever_.ptr() );
}


void uiVisPartServer::unlockEvent()
{
    eventmutex_.unLock();
}


bool uiVisPartServer::sendVisEvent( int evid )
{
    eventmutex_.lock();
    return sendEvent( evid );
}


uiVisPartServer::~uiVisPartServer()
{
    detachAllNotifiers();
    deleteAllObjects();
    delete vismgr_;

    delete &eventmutex_;
    delete mpetools_;
    menu_ = nullptr;
    toolbar_ = nullptr;
    pickretriever_ = nullptr;
    delete multirgeditwin_;
    delete dirlightdlg_;
    delete topbotdlg_;
    mousecursorexchange_ = nullptr;
}


const char* uiVisPartServer::name() const  { return "Visualization"; }


void uiVisPartServer::setMouseCursorExchange( MouseCursorExchange* mce )
{
    if ( mousecursorexchange_ )
	mDetachCB( mousecursorexchange_->notifier,
		   uiVisPartServer::mouseCursorCB );

    mousecursorexchange_ = mce;

    if ( mousecursorexchange_ )
	mAttachCB( mousecursorexchange_->notifier,
		   uiVisPartServer::mouseCursorCB );
}


void uiVisPartServer::mouseCursorCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( const MouseCursorExchange::Info&, info,
				caller, cb );
    if ( caller==this )
	return;

    setMarkerPos( info.trkv_, SceneID::udf() );
}


void uiVisPartServer::setSceneName( const SceneID& id, const uiString& nm )
{
    RefMan<visSurvey::Scene> scene = getScene( id );
    if ( scene )
	scene->setUiName( nm );
}


void uiVisPartServer::setUiObjectName( const VisID& id, const uiString& nm )
{
    visBase::DataObject* obj = visBase::DM().getObject( id );
    if ( obj )
	obj->setUiName( nm );
}


void uiVisPartServer::setObjectName( const VisID& id, const char* nm )
{
    visBase::DataObject* obj = visBase::DM().getObject( id );
    if ( obj )
	obj->setName( nm );
}


uiString uiVisPartServer::getSceneName( const SceneID& id ) const
{
    ConstRefMan<visSurvey::Scene> scene = getScene( id );
    return scene ? scene->uiName() : uiString::empty();
}


uiString uiVisPartServer::getUiObjectName( const VisID& id ) const
{
    const visBase::DataObject* obj = visBase::DM().getObject( id );
    return obj ? obj->uiName() : uiString::empty();
}


Pos::GeomID uiVisPartServer::getGeomID( const VisID& id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getGeomID() : Pos::GeomID::udf();
}


SceneID uiVisPartServer::addScene( visSurvey::Scene* newscene_in )
{
    RefMan<visSurvey::Scene> newscene = newscene_in;
    if ( !newscene )
	newscene = visSurvey::Scene::create();

    mAttachCB( newscene->mouseposchange, uiVisPartServer::mouseMoveCB );
    mAttachCB( newscene->keypressed, uiVisPartServer::keyEventCB );
    mAttachCB( newscene->mouseclicked, uiVisPartServer::mouseEventCB );
    scenes_ += newscene.ptr();
    pickretriever_->addScene( newscene.ptr() );
    if ( isSoloMode() )
	displayids_ += TypeSet<VisID>();

    const SceneID sceneid = newscene->getID();
    newscene = nullptr;
    nrscenesChange.trigger();
    return sceneid;
}


void uiVisPartServer::removeScene( const SceneID& sceneid )
{
    RefMan<visSurvey::Scene> scene = getScene( sceneid );
    if ( !scene )
	return;

    const int typesetidx = scenes_.indexOf( scene.ptr() );
    if ( displayids_.validIdx(typesetidx) )
	displayids_.removeSingle( typesetidx );

    mDetachCB( scene->mouseposchange, uiVisPartServer::mouseMoveCB );
    mDetachCB( scene->keypressed, uiVisPartServer::keyEventCB );
    mDetachCB( scene->mouseclicked, uiVisPartServer::mouseEventCB );
    pickretriever_->removeScene( scene.ptr() );
    scenes_ -= scene.ptr();
    scene = nullptr;
    nrscenesChange.trigger();
}


int uiVisPartServer::nrScenes() const
{
    return scenes_.size();
}


bool uiVisPartServer::clickablesInScene( const char* trackertype,
					 const SceneID& sceneid ) const
{
    TypeSet<VisID> sceneobjids;
    getSceneChildIds( sceneid, sceneobjids );
    for ( int idx=0; idx<sceneobjids.size(); idx++ )
    {
	const VisID objid = sceneobjids[idx];
	if ( visSurvey::MPEClickCatcher::isClickable(trackertype,objid) )
	{
	    if ( !hasAttrib(objid) )
		return true;

	    for ( int attrid=getNrAttribs(objid)-1; attrid>=0; attrid-- )
	    {
		if ( isAttribEnabled(objid,attrid) )
		    return true;
	    }
	}
    }

    uiMSG().warning(tr("The scene does not yet contain any object "
		       "on which seeds\nfor a '%1' can be picked.")
		  .arg(trackertype));
    return false;
}


bool uiVisPartServer::getClickableAttributesInScene(
					TypeSet<Attrib::SelSpec>& attribspecs,
					BufferStringSet& attribnames,
					const char* trackertype,
					const SceneID& sceneid ) const
{
    TypeSet<VisID> sceneobjids;
    getSceneChildIds( sceneid, sceneobjids );
    attribnames.setEmpty();
    attribspecs.setEmpty();
    for ( int idx=0; idx<sceneobjids.size(); idx++ )
    {
	const VisID objid = sceneobjids[idx];
	if ( !visSurvey::MPEClickCatcher::isClickable(trackertype,objid) )
	    continue;

	mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(objid));
	if ( !so )
	    continue;

	for ( int attridx=0; attridx<so->nrAttribs(); attridx++ )
	{
	    if ( !so->isAttribEnabled(attridx) )
		continue;

	    const TypeSet<Attrib::SelSpec>* specs = so->getSelSpecs( attridx );
	    if ( !specs )
		continue;

	    for ( int specidx=0; specidx<specs->size(); specidx++ )
	    {
		const Attrib::SelSpec& spec = specs->get( specidx );
		if ( attribnames.addIfNew(spec.userRef()) )
		    attribspecs.add( spec );
	    }
	}
    }

    return !attribnames.isEmpty();
}


bool uiVisPartServer::selectAttribForTracking()
{
    return mpetools_ && mpetools_->selectAttribForTracking();
}


bool uiVisPartServer::disabMenus( bool yn )
{
    const bool res = blockmenus_;
    blockmenus_ = yn;
    return res;
}


void uiVisPartServer::createToolBars()
{
    mpetools_ = new uiMPEMan( appserv().parent(), this );

    toolbar_ = new uiTreeItemTBHandler( appserv().parent() );
    mAttachCB( toolbar_->createnotifier, uiVisPartServer::addToToolBarCB );
    mAttachCB( toolbar_->handlenotifier, uiVisPartServer::handleMenuCB );
    slicepostools_ = new uiSlicePos3DDisp( appserv().parent(), this );
}


bool uiVisPartServer::disabToolBars( bool yn )
{
    bool res = false;
    if ( slicepostools_ )
    {
	res = !slicepostools_->getToolBar()->sensitive();
	slicepostools_->getToolBar()->setSensitive( !yn );
    }
    return res;
}


bool uiVisPartServer::blockMouseSelection( bool yn )
{
    if ( scenes_.isEmpty() )
	return false;

    const bool res = scenes_.first()->blockMouseSelection( yn );
    for ( int idx=1; idx<nrScenes(); idx++ )
	scenes_[idx]->blockMouseSelection( yn );

    return res;
}


bool uiVisPartServer::showMenu( const VisID& id, int menutype,
				const TypeSet<int>* path,
				const Coord3& pickedpos )
{
    if ( blockmenus_ )
	return true;

    menu_->setMenuID( id.asInt() );
    menu_->setPickedPos( pickedpos );
    return menu_->executeMenu( menutype, path );
}


MenuHandler* uiVisPartServer::getMenuHandler()
{
    return menu_.ptr();
}


MenuHandler* uiVisPartServer::getToolBarHandler()
{
    return toolbar_.ptr();
}


void uiVisPartServer::shareObject( const SceneID& sceneid, const VisID& id )
{
    RefMan<visSurvey::Scene> scene = getScene( sceneid );
    if ( !scene ) return;

    mDynamicCastGet(visBase::DataObject*,dobj,visBase::DM().getObject(id))
    if ( !dobj ) return;

    scene->addObject( dobj );
    objectAdded.trigger( id );
    eventmutex_.lock();
    sendEvent( evUpdateTree() );
}


void uiVisPartServer::triggerTreeUpdate()
{
    eventmutex_.lock();
    sendEvent( evUpdateTree() );
}


void uiVisPartServer::findObject( const std::type_info& ti,
				  TypeSet<VisID>& res) const
{
    visBase::DM().getIDs( ti, res );
}


void uiVisPartServer::findObject( const MultiID& mid,
				  TypeSet<VisID>& res ) const
{
    if ( mid.isUdf() )
	return;

    for ( int idx=visBase::DM().nrObjects()-1; idx>=0; idx-- )
    {
	const visBase::DataObject* datobj = visBase::DM().getIndexedObject(idx);
	mDynamicCastGet( const visSurvey::SurveyObject*, survobj, datobj );
	if ( survobj && mid==survobj->getMultiID() )
	    res += datobj->id();
    }
}


VisID uiVisPartServer::highestID() const
{
    return visBase::DM().highestID();
}


const visBase::DataObject* uiVisPartServer::getObject( const VisID& id ) const
{
    return getNonConst(*this).getObject( id );
}


visBase::DataObject* uiVisPartServer::getObject( const VisID& id )
{
    mDynamicCastGet(visBase::DataObject*,dobj,visBase::DM().getObject(id))
    return dobj;
}


void uiVisPartServer::addObject( visBase::DataObject* dobj,
				 const SceneID& sceneid, bool saveinsessions )
{
    RefMan<visSurvey::Scene> scene = getScene( sceneid );
    if ( !scene )
	return;

    scene->addObject( dobj );
    objectAdded.trigger( dobj->id() );

    mDynamicCastGet( visSurvey::SurveyObject*, surobj, dobj );
    if ( surobj )
	surobj->setSaveInSessionsFlag( saveinsessions );

    setUpConnections( dobj->id() );
    if ( isSoloMode() )
    {
	const int typesetidx = scenes_.indexOf( scene.ptr() );
	if ( displayids_.validIdx(typesetidx) )
	    displayids_[typesetidx] += dobj->id();

	turnOn( dobj->id(), true, true );
    }
}


void uiVisPartServer::removeObject( visBase::DataObject* dobj,
				    const SceneID& sceneid )
{
    if ( !dobj )
	return;

    removeObject( dobj->id(), sceneid );
    objectRemoved.trigger( dobj->id() );
}


NotifierAccess& uiVisPartServer::removeAllNotifier()
{
    return visBase::DM().removeallnotify;
}


MultiID uiVisPartServer::getMultiID( const VisID& id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getMultiID() : MultiID::udf();
}


VisID uiVisPartServer::getSelObjectId() const
{
    const TypeSet<VisID>& sel = visBase::DM().selMan().selected();
    return sel.size() ? sel[0] : VisID::udf();
}


int uiVisPartServer::getSelAttribNr() const
{ return selattrib_; }


void uiVisPartServer::setSelObjectId( const VisID& id, int attrib )
{
    visBase::DM().selMan().select( id );
    if ( !id.isValid() )
	return;

    selattrib_ = attrib;

    eventmutex_.lock();
    eventobjid_ = id;
    sendEvent( evSelection() );
    eventmutex_.lock();
    sendEvent( evPickingStatusChange() );

    mDynamicCastGet(visSurvey::SurveyObject*,so,visBase::DM().getObject(id));
    if ( so && so->getScene() && so->getScene()->getSceneColTab() )
    {
	const ColTab::Sequence* seq = so->getColTabSequence( selattrib_ );
	const ColTab::MapperSetup* ms = so->getColTabMapperSetup( selattrib_ );
	if ( seq )
	    so->getScene()->getSceneColTab()->setColTabSequence( *seq );
	if ( ms )
	    so->getScene()->getSceneColTab()->setColTabMapperSetup( *ms );
    }
}


SceneID uiVisPartServer::getSceneID( const VisID& visid ) const
{
    for ( const auto* scene : scenes_ )
    {
	TypeSet<VisID> childids;
	getSceneChildIds( scene->getID(), childids );
	if ( childids.isPresent(visid) )
	    return scene->getID();
    }

    return SceneID::udf();
}


const ZDomain::Info* uiVisPartServer::zDomainInfo( const SceneID& sceneid) const
{
    const visSurvey::Scene* scene = getScene( sceneid );
    return scene ? &scene->zDomainInfo() : nullptr;
}


void uiVisPartServer::getSceneIds( TypeSet<SceneID>& sceneids ) const
{
    for ( const auto* scene : scenes_ )
	sceneids += scene->getID();
}


void uiVisPartServer::getSceneChildIds( const SceneID& sceneid,
					TypeSet<VisID>& childids ) const
{
    ConstRefMan<visSurvey::Scene> scene = getScene( sceneid );
    if ( !scene )
	return;

    for ( int idx=0; idx<scene->size(); idx++ )
	childids += scene->getObject( idx )->id();
}


void uiVisPartServer::getVisChildIds( const VisID& id,
				      TypeSet<VisID>& childids ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id))
    if ( so )
	so->getChildren( childids );
}


uiVisPartServer::AttribFormat
    uiVisPartServer::getAttributeFormat( const VisID& id, int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id))
    if ( !so )
	return None;

    switch ( so->getAttributeFormat(attrib) )
    {
	case visSurvey::SurveyObject::None		: return None;
	case visSurvey::SurveyObject::Cube		: return Cube;
	case visSurvey::SurveyObject::Traces		: return Traces;
	case visSurvey::SurveyObject::RandomPos		: return RandomPos;
	case visSurvey::SurveyObject::OtherFormat	: return OtherFormat;
    }

    return None;
}


bool uiVisPartServer::canHaveMultipleAttribs( const VisID& id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so && so->canHaveMultipleAttribs();
}


bool uiVisPartServer::canAddAttrib( const VisID& id, int nr ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so && so->canAddAttrib(nr);
}


bool uiVisPartServer::canRemoveAttrib( const VisID& id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so && so->canRemoveAttrib();
}


bool uiVisPartServer::canRemoveDisplay( const VisID& id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so && so->canBeRemoved();
}


int uiVisPartServer::addAttrib( const VisID& id )
{
    if ( !canHaveMultipleAttribs(id) )
	return -1;

    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so )
	return -1;

    return so->addAttrib() ? so->nrAttribs()-1 : -1;
}


void uiVisPartServer::removeAttrib( const VisID& id, int attrib )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so )
	return;

    so->removeAttrib( attrib );
    selattrib_ = -1;
}

int uiVisPartServer::getNrAttribs( const VisID& id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->nrAttribs() : 0;
}


void uiVisPartServer::getAttribPosName( const VisID& id, int attrib,
					uiString& res ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    if ( so )
	so->getChannelName( attrib, res );
}


bool uiVisPartServer::swapAttribs( const VisID& id, int attrib0, int attrib1 )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so && so->swapAttribs( attrib0, attrib1 );
}


void uiVisPartServer::showAttribTransparencyDlg( const VisID& id, int attrib )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so )
	return;

    uiAttribTransDlg dlg( appserv().parent(), *so, attrib );
    dlg.go();
}


unsigned char uiVisPartServer::getAttribTransparency( const VisID& id,
						      int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getAttribTransparency( attrib ) : 0;
}


void uiVisPartServer::setAttribTransparency( const VisID& id, int attrib,
					     unsigned char nv )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so )
	so->setAttribTransparency( attrib, nv );
}


TrcKeyZSampling uiVisPartServer::getTrcKeyZSampling( const VisID& id,
						     int attribid ) const
{
    TrcKeyZSampling res;
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    if ( so )
	res = so->getTrcKeyZSampling( false, attribid );

    return res;
}


int uiVisPartServer::currentVersion( const VisID& id, int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    if ( !so || !so->getChannels() )
	return -1;

    return so->getChannels()->currentVersion( attrib );
}


ConstRefMan<DataPack> uiVisPartServer::getDataPack(
					    const VisID& id, int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getDataPack( attrib ) : nullptr;
}


ConstRefMan<DataPack> uiVisPartServer::getDisplayedDataPack(
					    const VisID& id, int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getDisplayedDataPack( attrib ) : nullptr;
}


bool uiVisPartServer::setRegularSeisDataPack( const VisID& id, int attrib,
					      RegularSeisDataPack* seisdp )
{
    const visBase::DataObject* visobj = getObject( id );
    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,visobj);
    mDynamicCastGet(const visSurvey::Seis2DDisplay*,s2dd,visobj);
    mDynamicCastGet(const visSurvey::VolumeDisplay*,vdd,visobj);
    if ( !pdd && !s2dd && !vdd )
	return false;

    return setVolumeDataPack( id, attrib, seisdp );
}


bool uiVisPartServer::setRandomSeisDataPack( const VisID& id, int attrib,
					     RandomSeisDataPack* randsdp )
{
    const visBase::DataObject* visobj = getObject( id );
    mDynamicCastGet(const visSurvey::RandomTrackDisplay*,rtd,visobj);
    return rtd ? setVolumeDataPack( id, attrib, randsdp ) : false;
}


bool uiVisPartServer::setVolumeDataPack( const VisID& id, int attrib,
					 VolumeDataPack* voldp )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so )
	return false;

    uiTaskRunner taskrunner( appserv().parent() );
    const bool res = so->setVolumeDataPack( attrib, voldp, &taskrunner );
/*
   enable only in release branches:
    DataPackMgr& seisdpmgr = DPM( DataPackMgr::SeisID() );
    if ( !OD::InDebugMode() && voldp && !seisdpmgr.isPresent(voldp->id()) )
	seisdpmgr.add( voldp );
*/
    const DataPackID dpid = voldp ? voldp->id() : DataPack::cNoID();
    if ( res && multirgeditwin_ && !multirgeditwin_->isHidden()
	 && id==mapperrgeditordisplayid_ )
    {
	multirgeditwin_->setDataPack( attrib, voldp,
				      so->selectedTexture(attrib) );
    }

    return res;
}


ConstRefMan<FlatDataPack> uiVisPartServer::getFlatDataPack(
					    const VisID& id, int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getFlatDataPack( attrib ) : nullptr;
}


ConstRefMan<VolumeDataPack> uiVisPartServer::getVolumeDataPack(
					    const VisID& id, int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getVolumeDataPack( attrib ) : nullptr;
}


ConstRefMan<VolumeDataPack> uiVisPartServer::getDisplayedVolumeDataPack(
					    const VisID& id, int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getDisplayedVolumeDataPack( attrib ) : nullptr;
}


bool uiVisPartServer::canHaveMultipleTextures( const VisID& id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so && so->canHaveMultipleTextures();
}


int uiVisPartServer::nrTextures( const VisID& id, int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->nrTextures( attrib ) : 0;
}


void uiVisPartServer::selectTexture( const VisID& id, int attrib,
				     int textureidx )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so && isAttribEnabled(id,attrib) )
	so->selectTexture( attrib, textureidx );
}


void uiVisPartServer::setTranslation( const VisID& id, const Coord3& shift )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so )
	so->setTranslation( shift );
}


Coord3 uiVisPartServer::getTranslation( const VisID& id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getTranslation() : Coord3(0,0,0);
}


int uiVisPartServer::selectedTexture( const VisID& id, int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->selectedTexture( attrib ) : 0;
}


bool uiVisPartServer::setPointDataPack( const VisID& id, int attrib,
					PointDataPack* pointdp )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so )
	return false;

    uiTaskRunner taskrunner( appserv().parent() );
    return so->setPointDataPack( attrib, pointdp, &taskrunner );
}


bool uiVisPartServer::setRandomPosData( const VisID& id, int attrib,
					const DataPointSet* dtps )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so )
	return false;

    uiTaskRunner taskrunner( appserv().parent() );
    return so->setRandomPosData( attrib, dtps, &taskrunner );
}


ConstRefMan<PointDataPack> uiVisPartServer::getPointDataPack(
					    const VisID& id, int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getPointDataPack( attrib ) : nullptr;
}


bool uiVisPartServer::getRandomPos( const VisID& id, DataPointSet& dtps ) const
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    if ( !so )
	return false;

    uiTaskRunner taskrunner( appserv().parent() );
    return so->getRandomPos( dtps, &taskrunner );
}


bool uiVisPartServer::getRandomPosCache( const VisID& id, int attrib,
					 DataPointSet& dtps ) const
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so && so->getRandomPosCache( attrib, dtps );
}


Interval<float> uiVisPartServer::getDataTraceRange( const VisID& id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getDataTraceRange() : Interval<float>(0,0);
}


uiString uiVisPartServer::getInteractionMsg( const VisID& id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id))
    return so ? so->getManipulationString() : uiString::empty();
}


void uiVisPartServer::getObjectInfo( const VisID& id, uiString& info ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id))
    if ( so )
	so->getObjectInfo( info );
}


const ColTab::MapperSetup*
    uiVisPartServer::getColTabMapperSetup( const VisID& id, int attrib,
					   int version ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getColTabMapperSetup( attrib, version ) : nullptr;
}


void uiVisPartServer::setColTabMapperSetup( const VisID& id, int attrib,
					    const ColTab::MapperSetup& ms )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so )
	return;

    so->setColTabMapperSetup( attrib, ms, nullptr );
    if ( so->getScene() && so->getScene()->getSceneColTab() )
	so->getScene()->getSceneColTab()->setColTabMapperSetup( ms );

    if ( multirgeditwin_ && !multirgeditwin_->isHidden()
	 && id==mapperrgeditordisplayid_ )
    {
	if ( mapperrgeditinact_ )
	    mapperrgeditinact_ = false;
	else
	    multirgeditwin_->setColTabMapperSetup( attrib, ms );
    }
}


const ColTab::Sequence*
    uiVisPartServer::getColTabSequence( const VisID& id, int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id))
    return so ? so->getColTabSequence( attrib ) : nullptr;
}


bool uiVisPartServer::canHandleColTabSeqTrans( const VisID& id,
					       int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id))
    return so && so-> canHandleColTabSeqTrans( attrib );
}


bool uiVisPartServer::canSetColTabSequence( const VisID& id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id))
    return so && so->canSetColTabSequence();
}


void uiVisPartServer::setColTabSequence( const VisID& id, int attrib,
					 const ColTab::Sequence& seq )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so )
	return;

    so->setColTabSequence( attrib, seq, nullptr );
    if ( so->getScene() && so->getScene()->getSceneColTab() )
	so->getScene()->getSceneColTab()->setColTabSequence( seq );

    if ( multirgeditwin_ && !multirgeditwin_->isHidden()
	 && id==mapperrgeditordisplayid_ )
    {
	if ( mapperrgeditinact_ )
	    mapperrgeditinact_ = false;
	else
	    multirgeditwin_->setColTabSeq( attrib, seq );
    }
}


void uiVisPartServer::fillDispPars( const VisID& id, int attrib,
				    FlatView::DataDispPars& common,
				    bool wva ) const
{
    const auto dest = FlatView::Viewer::getDest( wva, !wva );
    fillDispPars( id, attrib, common, dest );
}


void uiVisPartServer::fillDispPars( const VisID& id, int attrib,
				    FlatView::DataDispPars& common,
				    FlatView::Viewer::VwrDest dest ) const
{
    const bool wva =	dest == FlatView::Viewer::WVA ||
			dest == FlatView::Viewer::Both;
    const bool vd =	dest == FlatView::Viewer::VD ||
			dest == FlatView::Viewer::Both;

    const ColTab::MapperSetup* mapper = getColTabMapperSetup( id, attrib );
    const ColTab::Sequence* seq = getColTabSequence( id, attrib );
    if ( !mapper || !seq )
	return;

    common.wva_.show_ = false;
    common.vd_.show_ = false;
    if ( wva )
    {
	common.wva_.mappersetup_ = *mapper;
	common.wva_.show_ = true;
    }
    if ( vd )
    {
	common.vd_.mappersetup_ = *mapper;
	common.vd_.ctab_ = seq->name();
	common.vd_.show_ = true;
    }
}


const TypeSet<float>* uiVisPartServer::getHistogram( const VisID& id,
						     int attrib) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getHistogram( attrib ) : nullptr;
}


VisID uiVisPartServer::getEventObjId() const
{ return eventobjid_; }


int uiVisPartServer::getEventAttrib() const
{ return eventattrib_; }


const TypeSet<Attrib::SelSpec>* uiVisPartServer::getSelSpecs( const VisID& id,
							      int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getSelSpecs( attrib ) : nullptr;
}


const Attrib::SelSpec* uiVisPartServer::getSelSpec( const VisID& id,
						    int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getSelSpec(attrib,selectedTexture(id,attrib)) : nullptr;
}


bool uiVisPartServer::interpolationEnabled( const VisID& id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->textureInterpolationEnabled() : true;
}


void uiVisPartServer::enableInterpolation( const VisID& id, bool yn )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so )
	so->enableTextureInterpolation( yn );
}


bool uiVisPartServer::isAngle( const VisID& id, int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so && so->isAngle( attrib );
}


void uiVisPartServer::setAngleFlag( const VisID& id, int attrib, bool yn )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so )
	so->setAngleFlag( attrib, yn );
}


bool uiVisPartServer::isAttribEnabled( const VisID& id, int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so && so->isAttribEnabled( attrib );
}


void uiVisPartServer::triggerObjectMoved( const VisID& id )
{
    visBase::DataObject* dobj = visBase::DM().getObject( id );
    if ( !dobj )
	return;

    for ( auto* scene : scenes_ )
    {
	if ( scene )
	    scene->objectMoved( dobj );
    }
}


void uiVisPartServer::enableAttrib( const VisID& id, int attrib, bool yn )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so )
    {
	const bool wasanyattribenabled = so->isAnyAttribEnabled();
	so->enableAttrib( attrib, yn );

	if ( wasanyattribenabled != so->isAnyAttribEnabled() )
	    triggerObjectMoved( id );
    }
}


bool uiVisPartServer::hasSingleColorFallback( const VisID& id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so && so->hasSingleColorFallback();
}


bool uiVisPartServer::deleteAllObjects()
{
    mpetools_->deleteVisObjects();

    NotifyStopper ns( nrscenesChange );
    while ( !scenes_.isEmpty() )
	removeScene( scenes_.first()->getID() );

    closeAndNullPtr( multirgeditwin_ );
    closeAndNullPtr( topbotdlg_ );
    objectRemoved.trigger( VisID::udf() );
    ns.enableNotification();
    nrscenesChange.trigger();
    return true; //visBase::DM().removeAll();
}


void uiVisPartServer::setViewMode( bool yn, bool notify)
{
    if ( yn==viewmode_ )
	return;

    viewmode_ = yn;
    workmode_ = viewmode_ ? uiVisPartServer::View
			  : uiVisPartServer::Interactive;
    updateDraggers();
    if ( notify )
    {
	eventmutex_.lock();
	sendEvent(evViewModeChange());
    }
}


bool uiVisPartServer::isViewMode() const { return viewmode_; }


void uiVisPartServer::setWorkMode( uiVisPartServer::WorkMode wm, bool notify )
{
    if ( wm==workmode_ )
	return;

    workmode_ = wm;
    viewmode_ = workmode_ == uiVisPartServer::View;
    updateDraggers();
    if ( notify )
    {
	eventmutex_.lock();
	sendEvent( evViewModeChange() );
    }
}


uiVisPartServer::WorkMode uiVisPartServer::getWorkMode() const
{ return workmode_; }


bool uiVisPartServer::isSoloMode() const { return issolomode_; }


void uiVisPartServer::setSoloMode( bool yn, TypeSet< TypeSet<VisID> > dispids,
				   const VisID& selid )
{
    issolomode_ = yn;
    displayids_ = dispids;
    updateDisplay( issolomode_, selid );
}


void uiVisPartServer::setSelectionMode( uiVisPartServer::SelectionMode mode )
{
    if ( isSelectionModeOn() && mode==Polygon )
	seltype_ = visBase::PolygonSelection::Polygon;
    if ( isSelectionModeOn() && mode==Rectangle )
	seltype_ = visBase::PolygonSelection::Rectangle;

    for ( int sceneidx=0; sceneidx<nrScenes(); sceneidx++ )
    {
	RefMan<visSurvey::Scene> scene = scenes_[sceneidx];
	if ( scene->getPolySelection() )
	    scene->getPolySelection()->setSelectionType( seltype_ );

	if ( getSelObjectId().isValid() )
	{
	    mDynamicCastGet(
	      visSurvey::SurveyObject*, so, getObject(getSelObjectId()) );
	    if ( so )
	    {
		for ( int idx=0; idx<scene->size(); idx++ )
		{
		    mDynamicCastGet(
		       visSurvey::SurveyObject*,everyso,scene->getObject(idx) );
		    if ( everyso )
			everyso->turnOnSelectionMode( false );
		}
		so->turnOnSelectionMode( isSelectionModeOn() );
	    }
	}
    }

    selectionmode_ = mode;
    selectionmodeChange.trigger();
}


void uiVisPartServer::turnSelectionModeOn( bool yn )
{
    seltype_ = !yn ? visBase::PolygonSelection::Off
		   : visBase::PolygonSelection::Rectangle; // Dummy

    setSelectionMode( selectionmode_ );
    updateDraggers();

    if ( !yn )
    {
	const VisID selid = getSelObjectId();
	mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(selid))
	if ( so ) so->clearSelections();
    }
}


uiVisPartServer::SelectionMode uiVisPartServer::getSelectionMode() const
{
    return selectionmode_;
}


bool uiVisPartServer::isSelectionModeOn() const
{
    return seltype_ != visBase::PolygonSelection::Off;
}


const Selector<Coord3>*
	uiVisPartServer::getCoordSelector( const SceneID& sceneid ) const
{
    ConstRefMan<visSurvey::Scene> scene = getScene( sceneid );
    return scene ? scene->getSelector() : nullptr;
}


int uiVisPartServer::getTypeSetIdx( const VisID& selid )
{
    int typesetidx=0;
    bool found = false;
    for ( int idx=0; idx<displayids_.size(); idx++ )
    {
	for ( int idy=0; idy<displayids_[idx].size(); idy++ )
	{
	    if ( displayids_[idx][idy] == selid )
	    {
		typesetidx = idx;
		found = true;
		break;
	    }
	}
	if ( found )
	    break;
    }

    return typesetidx;
}


void uiVisPartServer::updateDisplay( bool doclean, const VisID& selid,
				     const VisID& refid )
{
    int typesetidx;
    typesetidx = getTypeSetIdx( selid.isValid() ? selid : refid );

    if ( doclean && displayids_.size()>typesetidx )
    {
	for ( int idx=0; idx<displayids_[typesetidx].size(); idx++ )
	    if ( isOn( displayids_[typesetidx][idx] )
		 && displayids_[typesetidx][idx] != selid )
		   // later: include check if displayid is a sub-item of selid
		turnOn(displayids_[typesetidx][idx], false);

	if ( !isOn(selid) && selid.isValid() )
	    turnOn( selid, true);
	else if ( !isOn(refid) && refid.isValid() )
	    turnOn( refid, true);
    }
    else
    {
	bool isselchecked = false;
	for ( int ids=0; ids<displayids_.size(); ids++ )
	{
	    for ( int idx=0; idx<displayids_[ids].size(); idx++ )
	    {
		turnOn(displayids_[ids][idx], true);
		if ( displayids_[ids][idx] == selid )
		    isselchecked = true;
	    }
	}

	if ( !isselchecked ) turnOn(selid, false);
    }
}


void uiVisPartServer::setTopBotImg( const SceneID& sceneid )
{
    RefMan<visSurvey::Scene> scene = getScene( sceneid );
    if ( !scene )
	return;

    delete topbotdlg_;
    topbotdlg_ = new uiSurvTopBotImageDlg( appserv().parent(), *scene.ptr() );
    topbotdlg_->setModal( false );
    topbotdlg_->show();
}


void uiVisPartServer::updateDraggers()
{
    const TypeSet<VisID>& selected = visBase::DM().selMan().selected();
    for ( int sceneidx=0; sceneidx<nrScenes(); sceneidx++ )
    {
	RefMan<visSurvey::Scene> scene = scenes_[sceneidx];
	scene->enableTraversal( visBase::cDraggerIntersecTraversalMask(),
				scene->isPickable() &&
				seltype_ == visBase::PolygonSelection::Off );

	for ( int objidx=0; objidx<scene->size(); objidx++ )
	{
	    visBase::DataObject* dobj = scene->getObject( objidx );
	    updateManipulatorStatus( dobj, selected.isPresent(dobj->id()) );
	}
    }
}


void uiVisPartServer::setZStretch()
{
    uiZStretchDlg dlg( appserv().parent(), this );
    dlg.vwallcb = mCB(this,uiVisPartServer,vwAll);
    dlg.homecb = mCB(this,uiVisPartServer,toHome);
    dlg.go();
}


void uiVisPartServer::setZAxisTransform( const SceneID& sceneid,
					 ZAxisTransform* zat,
					 TaskRunner* taskrunner )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( zat && scene )
	scene->setZAxisTransform( zat, taskrunner );
}


const ZAxisTransform* uiVisPartServer::getZAxisTransform(
						const SceneID& sceneid) const
{
    const visSurvey::Scene* scene = getScene( sceneid );
    return scene ? scene->getZAxisTransform() : nullptr;
}


visBase::EventCatcher* uiVisPartServer::getEventCatcher( const SceneID& sceneid)
{
    visSurvey::Scene* scene = getScene( sceneid );
    return scene ? &scene->eventCatcher() : nullptr;
}

// Directional light-related
void uiVisPartServer::setDirectionalLight()
{
    if ( !dirlightdlg_ )
	dirlightdlg_ = new uiDirLightDlg( appserv().parent(), this );

    dirlightdlg_->show();
}


void uiVisPartServer::vwAll( CallBacker* )
{ eventmutex_.lock(); sendEvent( evViewAll() ); }

void uiVisPartServer::toHome( CallBacker* )
{ eventmutex_.lock(); sendEvent( evToHomePos() ); }


bool uiVisPartServer::setWorkingArea()
{
    return setWorkingArea( SI().sampling(true) );
}


bool uiVisPartServer::setWorkingArea( const TrcKeyZSampling& newworkarea )
{
    uiPosProvider::Setup su( false, false, true );
    su.useworkarea(false);
    uiPosProvDlg dlg( parent(), su, tr("Set Work Area") );
    dlg.setSampling( newworkarea );
    dlg.setHelpKey( mODHelpKey(mWorkAreaDlgHelpID) );
    if ( !dlg.go() ) return false;

    TrcKeyZSampling tkzs;
    dlg.getSampling( tkzs );
    const_cast<SurveyInfo&>(SI()).setWorkRange( tkzs );

    for ( auto* scene : scenes_ )
    {
	if ( scene && scene->zDomainInfo().def_==ZDomain::SI() )
	    scene->setTrcKeyZSampling( SI().sampling(true), true );
    }

    return true;
}


bool uiVisPartServer::setWorkingArea( const SceneID& sceneid )
{
    RefMan<visSurvey::Scene> scene = getScene( sceneid );
    if ( !scene )
	return false;

    if ( scene->zDomainInfo().def_==ZDomain::SI() )
	return setWorkingArea();

    const TrcKeyZSampling& tkzs = scene->getTrcKeyZSampling( false );
    const TrcKeyZSampling& workarea = scene->getTrcKeyZSampling( true );
    const char* zdomkey = scene->zDomainKey();
    uiDialog dlg( parent(), uiDialog::Setup(tr("Set Work Area"),mNoHelpKey));
    auto* subvolfld = new uiSelSubvol( &dlg, false, zdomkey );
    subvolfld->setSampling( workarea );
    subvolfld->setLimits( tkzs );
    if ( !dlg.go() )
	return false;

    TrcKeyZSampling newtkzs = subvolfld->getSampling();
    scene->setTrcKeyZSampling( newtkzs, true );
    return true;
}


void uiVisPartServer::setOnlyAtSectionsDisplay( const VisID& id, bool yn )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    if ( so )
	so->setOnlyAtSectionsDisplay( yn );
}


bool uiVisPartServer::displayedOnlyAtSections( const VisID& id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id))
    return so && so->displayedOnlyAtSections();
}


bool uiVisPartServer::usePar( const IOPar& par )
{
    float step0=-1, step1=-1, step2=-1;
    par.get( sKeySliceSteps(), step0, step1, step2 );
    slicepostools_->setSteps( mNINT32(step0), mNINT32(step1), step2 );

    if ( !visBase::DM().usePar( par ) )
	return false;

    BufferString res;
    if ( par.get( sKeyWorkArea(), res ) )
    {
	FileMultiString fms(res);
	TrcKeyZSampling cs;
	TrcKeySampling& hs = cs.hsamp_; StepInterval<float>& zrg = cs.zsamp_;
	hs.start_.inl() = fms.getIValue(0); hs.stop_.inl() = fms.getIValue(1);
	hs.start_.crl() = fms.getIValue(2); hs.stop_.crl() = fms.getIValue(3);
	zrg.start_ = fms.getFValue( 4 ); zrg.stop_ = fms.getFValue( 5 );
	const_cast<SurveyInfo&>(SI()).setRange( cs, true );
    }
    else
	return false;

    int nrscenes( 0 );
    if ( !par.get(sKeyNumberScenes(),nrscenes) )
	return false;

    for ( int idx=0; idx<nrscenes; idx++ )
    {
	IOPar sceneidpar;
	BufferString key( sceneprefix(), idx );
	SceneID sceneid;
	BufferString scenekey = sceneidpar.compKey( key.buf(),sKeySceneID() );
	if ( !par.get(scenekey,sceneid) )
	    continue;

	const VisID scenevisid( sceneid.asInt() );
	if ( visBase::DM().getObject(scenevisid) )
	    continue;

	RefMan<visSurvey::Scene> newscene = visSurvey::Scene::create();
	newscene->setID( VisID( sceneid.asInt() ) );
	addScene( newscene.ptr() );

	PtrMan<IOPar> scenepar = par.subselect( key.buf() );
	if ( !scenepar )
	    continue;

	newscene->usePar( *scenepar );
	for ( int objidx=0; objidx<newscene->size(); objidx++ )
	{
	    const VisID objid = newscene->getObject(objidx)->id();
	    setUpConnections( objid );
	}
    }

    objectAdded.trigger( VisID::udf() );

    mpetools_->initFromDisplay();

    return true;
}


void uiVisPartServer::movePlaneAndCalcAttribs( const VisID& id,
					       const TrcKeyZSampling& tkzs )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,getObject(id))
    if ( !pdd )
	return;

    pdd->annotateNextUpdateStage( true );
    pdd->setTrcKeyZSampling( tkzs );
    pdd->resetManipulation();
    pdd->annotateNextUpdateStage( true );
    calculateAllAttribs( id );
    pdd->annotateNextUpdateStage( false );
    triggerTreeUpdate();
    planeMovedEvent.trigger();
}


void uiVisPartServer::calculateAllAttribs()
{
    for ( const auto* scene : scenes_ )
    {
	TypeSet<VisID> children;
	getSceneChildIds( scene->getID(), children );
	for ( const auto& childid : children )
	    calculateAllAttribs( childid );
    }
}


void uiVisPartServer::calculateAllAttribs( const VisID& id )
{
    visBase::DataObject* dobj = visBase::DM().getObject( id );
    mDynamicCastGet(visSurvey::SurveyObject*,so,dobj);
    if ( !so )
	return;

    for ( int attrib=0; attrib<so->nrAttribs(); attrib++ )
	calculateAttrib( id, attrib, false, true );
}


void uiVisPartServer::fillPar( IOPar& par ) const
{
    visBase::DM().fillPar( par );

    par.set( sKeyNumberScenes(), nrScenes() );
    const TrcKeyZSampling& cs = SI().sampling( true );
    FileMultiString fms;
    fms += cs.hsamp_.start_.inl();
    fms += cs.hsamp_.stop_.inl();
    fms += cs.hsamp_.start_.crl();
    fms += cs.hsamp_.stop_.crl();
    fms += cs.zsamp_.start_;
    fms += cs.zsamp_.stop_;
    par.set( sKeyWorkArea(), fms );

    par.set( sKeySliceSteps(),
	     sCast(float,slicepostools_->getStep(OD::SliceType::Inline)),
	     sCast(float,slicepostools_->getStep(OD::SliceType::Crossline)),
	     slicepostools_->getZStep() );

    for ( int idx=0; idx<nrScenes(); idx++ )
    {
	IOPar scenepar;
	BufferString key( sceneprefix(), idx );
	scenepar.set( sKeySceneID(), scenes_[idx]->id() );
	scenes_[idx]->fillPar( scenepar );
	par.mergeComp( scenepar, key.buf() );
    }
}


void uiVisPartServer::turnOn( const VisID& id, bool yn, bool doclean )
{
    if ( !vismgr_->allowTurnOn(id,doclean) )
	yn = false;

    mDynamicCastGet(visBase::VisualObject*,vo,getObject(id))
    if ( !vo || vo->isOn()==yn )
	return;

    vo->turnOn( yn );
    triggerObjectMoved( id );
}


bool uiVisPartServer::isOn( const VisID& id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObject( id );
    mDynamicCastGet(const visBase::VisualObject*,vo,dobj)
    return vo && vo->isOn();
}


bool uiVisPartServer::canDuplicate( const VisID& id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so && so->canDuplicate();
}


VisID uiVisPartServer::duplicateObject( const VisID& id, const SceneID& sceneid)
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    if ( !so )
	return VisID::udf();

    uiTaskRunner taskrunner( appserv().parent() );

    visSurvey::SurveyObject* newso = so->duplicate( &taskrunner );
    if ( !newso )
	return VisID::udf();

    mDynamicCastGet(visBase::DataObject*,doobj,newso)
    addObject( doobj, sceneid, true );

    return doobj->id();
}


bool uiVisPartServer::showSetupGroupOnTop( const char* grpnm )
{
    eventmutex_.lock();
    topsetupgroupname_ = grpnm;
    return sendEvent( evShowSetupGroupOnTop() );
}


const char* uiVisPartServer::getTopSetupGroupName() const
{ return topsetupgroupname_; }


void uiVisPartServer::reportTrackingSetupActive( bool yn )
{ tracksetupactive_ = yn; }


bool uiVisPartServer::isTrackingSetupActive() const
{ return tracksetupactive_; }


void uiVisPartServer::turnSeedPickingOn( bool yn )
{
    mpetools_->turnSeedPickingOn( yn );
}


bool uiVisPartServer::isPicking() const
{
    if ( isViewMode() )
	return false;

    if ( mpetools_ && mpetools_->isSeedPickingOn() )
	return true;

    const TypeSet<VisID>& sel = visBase::DM().selMan().selected();
    if ( sel.size()!=1 )
	return false;

    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(sel[0]));
    return so && so->isPicking();
}


void uiVisPartServer::getPickingMessage( BufferString& str ) const
{
    str = "";
    const TypeSet<VisID>& sel = visBase::DM().selMan().selected();
    if ( sel.size()!=1 )
	return;

    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(sel[0]));
    if ( so && so->isPicking() )
	so->getPickingMessage( str );
}


visSurvey::Scene* uiVisPartServer::getScene( const SceneID& sceneid )
{
    for ( auto* scene : scenes_ )
	if ( scene->id() == sceneid )
	    return scene;

    return nullptr;
}


const visSurvey::Scene* uiVisPartServer::getScene( const SceneID& sceneid) const
{
    return getNonConst(*this).getScene( sceneid );
}


void uiVisPartServer::removeObject( const VisID& id, const SceneID& sceneid )
{
    RefMan<visSurvey::Scene> scene = getScene( sceneid );
    if ( !scene )
	return;

    removeConnections( id );
    const int idx = scene->getFirstIdx( id );
    if ( idx==-1 )
	return;

    if ( id == mapperrgeditordisplayid_ )
    {
	closeAndNullPtr( multirgeditwin_ );
	mapperrgeditordisplayid_ = VisID::udf();
    }

    scene->removeObject( idx );
    objectRemoved.trigger( id );
}


void uiVisPartServer::removeSelection()
{
    TypeSet<SceneID> sceneids;
    getSceneIds( sceneids );
    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	const Selector<Coord3>* sel = getCoordSelector( sceneids[idx] );
	if ( !sel ) continue;

	VisID selobjectid = getSelObjectId();
	mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(selobjectid));
	if ( !so || !so->canRemoveSelection() )
	    continue;

	uiString msg = tr("Are you sure you want to \n"
			  "remove selected part of %1?")
		     .arg(getUiObjectName( selobjectid ));

	if ( uiMSG().askContinue(msg) )
	{
	    uiTaskRunner taskrunner( appserv().parent() );
	    so->removeSelections( &taskrunner );
	}
    }
}


bool uiVisPartServer::hasAttrib( const VisID& id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so && so->getAttributeFormat() != visSurvey::SurveyObject::None;
}


bool uiVisPartServer::selectAttrib( const VisID& id, int attrib )
{
    eventmutex_.lock();
    eventobjid_ = id;
    eventattrib_ = attrib;
    return sendEvent( evSelectAttrib() );
}


void uiVisPartServer::setSelSpec( const VisID& id, int attrib,
				  const Attrib::SelSpec& myattribspec )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so )
	so->setSelSpec( attrib, myattribspec );
}


void uiVisPartServer::setSelSpecs( const VisID& id, int attrib,
				   const TypeSet<Attrib::SelSpec>& selspecs)
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so )
	so->setSelSpecs( attrib, selspecs );
}


void uiVisPartServer::setUserRefs( const VisID& id, int attrib,
				   BufferStringSet* newuserrefs )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so )
	so->setUserRefs( attrib, newuserrefs );
}


bool uiVisPartServer::calculateAttrib( const VisID& id, int attrib,
				       bool newselect,
				       bool ignorelocked )
{
    if ( !ignorelocked && isLocked(id) )
    {
	resetManipulation(id);
	return true;
    }

    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so )
	return false;

    if ( so->isManipulated() )
	so->acceptManipulation();

    const Attrib::SelSpec* as = so->getSelSpec( attrib );
    if ( !as )
	return false;

    if ( as->id().asInt()==Attrib::SelSpec::cNoAttrib().asInt() )
	return true;

    if ( newselect ||
	 (as->id().asInt()==Attrib::SelSpec::cAttribNotSel().asInt()) )
    {
	if ( !selectAttrib( id, attrib ) )
	    return false;
    }

    eventmutex_.lock();
    eventobjid_ = id;
    eventattrib_ = attrib;

    if ( sendEvent(evGetNewData()) )
	return true;

    if ( so->getChannels() )
	so->getChannels()->unfreezeOldData( attrib );

    return false;
}


bool uiVisPartServer::calcManipulatedAttribs( const VisID& id )
{
    mDynamicCastGet( visSurvey::SurveyObject*, so, getObject(id) );
    if ( !so || !so->isManipulated() || so->isLocked() )
	return false;

    so->annotateNextUpdateStage( true );
    so->acceptManipulation();
    so->annotateNextUpdateStage( true );

    for ( int attrib=0; attrib<so->nrAttribs(); attrib++ )
	calculateAttrib( id, attrib, false );

    so->annotateNextUpdateStage( false );
    triggerTreeUpdate();
    return true;
}


bool uiVisPartServer::hasMaterial( const VisID& id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so && so->allowMaterialEdit();
}


void uiVisPartServer::setMaterial( const VisID& id )
{
    mDynamicCastGet(visBase::VisualObject*,vo,getObject(id))
    if ( !hasMaterial(id) || !vo )
	return;

    uiPropertiesDlg dlg( appserv().parent(),
			 dynamic_cast<visSurvey::SurveyObject*>(vo) );
    dlg.go();
}


bool uiVisPartServer::hasColor( const VisID& id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id))
    return so && so->hasColor();
}


void uiVisPartServer::setColor( const VisID& id, const OD::Color& col )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    if ( so )
	so->setColor( col );
}


bool uiVisPartServer::writeSceneToFile( const SceneID& sceneid,
					const uiString& dlgtitle ) const
{
    ConstRefMan<visSurvey::Scene> scene = getScene( sceneid );
    if ( !scene )
	return false;

    uiFileDialog filedlg( appserv().parent(), false, GetPersonalDir(),
			"*.osg", dlgtitle );
    filedlg.setDefaultExtension( "osg" );

    if ( filedlg.go() != uiDialog::Accepted )
	return false;

    const bool res = scene.getNonConstPtr()->serialize( filedlg.fileName() );
    if ( !res )
	uiMSG().error( tr("Could not write scene to file.") );

    return res;
}


bool uiVisPartServer::resetManipulation( const VisID& id )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so )
	so->resetManipulation();

    eventmutex_.lock();
    eventobjid_ = id;
    sendEvent( evInteraction() );
    eventmutex_.lock();
    sendEvent( evUpdateTree() );

    return so;
}


bool uiVisPartServer::isManipulated( const VisID& id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so && so->isManipulated();
}


void uiVisPartServer::acceptManipulation( const VisID& id )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so )
	so->acceptManipulation();
}


void uiVisPartServer::setUpConnections( const VisID& id )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    NotifierAccess* na = so ? so->getManipulationNotifier() : nullptr;
    if ( na ) na->notify( mCB(this,uiVisPartServer,interactionCB) );
    na = so ? so->getLockNotifier() : nullptr;
    if ( na ) na->notify( mCB(mpetools_,uiMPEMan,visObjectLockedCB) );

    mDynamicCastGet(visBase::VisualObject*,vo,getObject(id))
    if ( vo && vo->rightClicked() )
	vo->rightClicked()->notify(mCB(this,uiVisPartServer,rightClickCB));

    if ( so )
    {
	/*
	for ( int attrib=so->nrAttribs()-1; attrib>=0; attrib-- )
	{
	    mDynamicCastGet(visBase::VisColorTab*,coltab,
			    getObject(getColTabId(id,attrib)));
	    if ( coltab )
		coltab->sequencechange.notify(mCB(this,uiVisPartServer,
						  colTabChangeCB));
	}
	*/
    }
}


void uiVisPartServer::removeConnections( const VisID& id )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    NotifierAccess* na = so ? so->getManipulationNotifier() : nullptr;
    if ( na ) na->remove( mCB(this,uiVisPartServer,interactionCB) );
    na = so ? so->getLockNotifier() : nullptr;
    if ( na ) na->remove( mCB(mpetools_,uiMPEMan,visObjectLockedCB) );

    mDynamicCastGet(visBase::VisualObject*,vo,getObject(id));
    if ( vo && vo->rightClicked() )
	vo->rightClicked()->remove( mCB(this,uiVisPartServer,rightClickCB) );
    if ( !so ) return;

	/*
    for ( int attrib=so->nrAttribs()-1; attrib>=0; attrib-- )
    {
	mDynamicCastGet(visBase::VisColorTab*,coltab,
			getObject(getColTabId(id,attrib)));
	if ( coltab )
	    coltab->sequencechange.remove(
				    mCB(this,uiVisPartServer,colTabChangeCB) );
    }
				    */
}


void uiVisPartServer::rightClickCB( CallBacker* cb )
{
    mDynamicCastGet(visBase::DataObject*,dataobj,cb);
    const VisID id = dataobj ? dataobj->id() : getSelObjectId();
    if ( !id.isValid() )
	return;

    Coord3 pickedpos = Coord3::udf();
    mDynamicCastGet(visBase::VisualObject*,vo,getObject(id));
    if ( vo && vo->rightClickedEventInfo() )
	pickedpos = vo->rightClickedEventInfo()->worldpickedpos;

    TypeSet<int> intpath;
    const auto* path = dataobj ? dataobj->rightClickedPath() : nullptr;
    if ( path )
    {
	for ( const auto& pathid : *path )
	    intpath += pathid.asInt();
    }

    showMenu( id, uiMenuHandler::fromScene(), &intpath, pickedpos );
}


void uiVisPartServer::updateManipulatorStatus( visBase::DataObject* dobj,
					       bool isselected ) const
{
    mDynamicCastGet( visSurvey::SurveyObject*, so, dobj );
    if ( !so )
	return;

    const bool showmanipulator =  !so->isLocked() &&
	workmode_==uiVisPartServer::Interactive &&
	isselected;

    if ( showmanipulator!=so->isManipulatorShown() )
	so->showManipulator( showmanipulator );
}


void uiVisPartServer::selectObjCB( CallBacker* cb )
{
    mCBCapsuleUnpack(VisID,sel,cb);
    visBase::DataObject* dobj = visBase::DM().getObject( sel );

    updateManipulatorStatus( dobj, true );

    selattrib_ = -1;

    eventmutex_.lock();
    eventobjid_ = sel;
    sendEvent( evSelection() );

    eventmutex_.lock();
    sendEvent( evPickingStatusChange() );
}


#define mUpdateSelObj( cb, id, dataobj ) \
    mCBCapsuleUnpack( VisID, id, cb ); \
    visBase::DataObject* dataobj = visBase::DM().getObject( id ); \
    mDynamicCastGet( visSurvey::SurveyObject*, so, dataobj ) \
    if ( so ) \
    { \
	if ( so->isManipulated() && !calcManipulatedAttribs(id) ) \
	    resetManipulation( id ); \
    }

void uiVisPartServer::deselectObjCB( CallBacker* cb )
{
    mUpdateSelObj( cb, oldsel, dataobj );
    updateManipulatorStatus( dataobj, false );

    eventmutex_.lock();
    eventobjid_ = oldsel;
    selattrib_ = -1;
    sendEvent( evDeSelection() );

    eventmutex_.lock();
    sendEvent( evPickingStatusChange() );
}


void uiVisPartServer::updateSelObjCB( CallBacker* cb )
{
    mUpdateSelObj( cb, id, dataobj );
}


void uiVisPartServer::datasetUpdatedCB( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    static bool disableupdates = GetEnvVarYN( "OD_NO_VIS_AUTOUPDATES" );
    if ( disableupdates )
	return;

    mCBCapsuleUnpack(const MultiID&,mid,cb);
    if ( mid.isUdf() )
	return;

    const IOObjContext::StdDirData* stddata = nullptr;
    for ( int idx=0; idx<IOObjContext::totalNrStdDirs(); idx++ )
    {
	const IOObjContext::StdSelType seltyp = (IOObjContext::StdSelType)idx;
	stddata = IOObjContext::getStdDirData( seltyp );
	if ( stddata && stddata->id_.groupID() == mid.groupID() )
	    break;
    } if ( !stddata ) return;

    TypeSet<SceneID> sceneids;
    getSceneIds( sceneids );
    if ( sceneids.isEmpty() )
	return;

    TypeSet<VisID> todoids;
    for ( const auto& sceneid : sceneids )
    {
	TypeSet<VisID> scenevisids;
	getSceneChildIds( sceneid, scenevisids );
	for ( const auto& visid : scenevisids )
	{
	    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(visid))
	    if ( !so )
		continue;

	    const MultiID vismid = so->getMultiID();
	    if ( !vismid.isUdf() && vismid == mid )
	    { // Valid for most data types: EarthModel objects, wells, pre-stack
		todoids += visid;
		continue;
	    }

	    for ( int attrib=0; attrib<so->nrAttribs(); attrib++ )
	    {
		const visSurvey::SurveyObject::AttribFormat format =
					    so->getAttributeFormat( attrib );
		if ( format != visSurvey::SurveyObject::Cube &&
		     format != visSurvey::SurveyObject::Traces )
		    continue;

		const Attrib::SelSpec* selspec = so->getSelSpec( attrib );
		if ( !selspec || !selspec->isStored() ||
		     selspec->getStoredMultiID() != mid )
		{   //TODO support attribs?
		    continue;
		}

		calculateAttrib( visid, attrib, false, true );
	    }
	}
    }

    /*TODO: send todoids using a trigger?
      TODO: use a similar technique to remove tree items
      on IOM().entryRemoved / IOM().entriesRemoved */
}


void uiVisPartServer::interactionCB( CallBacker* cb )
{
    mDynamicCastGet(visBase::DataObject*,dataobj,cb)
    if ( dataobj )
    {
	eventmutex_.lock();
	eventobjid_ = dataobj->id();
	sendEvent( evInteraction() );
    }
}


void uiVisPartServer::setMarkerPos( const TrcKeyValue& worldpos,
				    const SceneID& dontsetscene )
{
    for ( auto* scene : scenes_ )
	scene->setMarkerPos( worldpos, dontsetscene );
}


void uiVisPartServer::mouseMoveCB( CallBacker* cb )
{
    mDynamicCast(visSurvey::Scene*,sceneeventsrc_,cb)
    if ( !sceneeventsrc_ )
	return;

    const TrcKeyValue worldpos = sceneeventsrc_->getMousePos();

    xytmousepos_ = sceneeventsrc_->getMousePos( true );
    setMarkerPos( worldpos, sceneeventsrc_->getID() );

    MouseCursorExchange::Info info( worldpos );
    mousecursorexchange_->notifier.trigger( info, this );

    eventmutex_.lock();
    mousescene_ = sceneeventsrc_->getID();
    mouseposval_ = sceneeventsrc_->getMousePosValue();
    mouseposstr_ = sceneeventsrc_->getMousePosString();
    zfactor_ = sceneeventsrc_->zDomainUserFactor();
    sendEvent( evMouseMove() );
    sceneeventsrc_ = nullptr;
}


void uiVisPartServer::keyEventCB( CallBacker* cb )
{
    mDynamicCast(visSurvey::Scene*,sceneeventsrc_,cb)
    if ( !sceneeventsrc_ )
	return;

    eventmutex_.lock();
    kbevent_ = sceneeventsrc_->getKeyboardEvent();

    const VisID selid = getSelObjectId();
    if ( kbevent_.key_==OD::KB_V && kbevent_.modifier_==OD::NoButton )
    {
	setOnlyAtSectionsDisplay( selid, !displayedOnlyAtSections(selid) );
    }

    sendEvent( evKeyboardEvent() );
    keyEvent.trigger();
    sceneeventsrc_ = nullptr;
}


void uiVisPartServer::mouseEventCB( CallBacker* cb )
{
    mDynamicCast(visSurvey::Scene*,sceneeventsrc_,cb)
    if ( !sceneeventsrc_ )
	return;

    eventmutex_.lock();
    mouseevent_ = sceneeventsrc_->getMouseEvent();
    sendEvent( evMouseEvent() );
    mouseEvent.trigger();
    sceneeventsrc_ = nullptr;
}


void uiVisPartServer::setSceneEventHandled()
{
    if ( sceneeventsrc_ )
	sceneeventsrc_->setEventHandled();
}


void uiVisPartServer::addToToolBarCB( CallBacker* cb )
{
    mDynamicCastGet(uiTreeItemTBHandler*,tb,cb);
    if ( !tb ) return;
    mDynamicCastGet(visSurvey::SurveyObject*,so,
		    getObject(VisID(tb->menuID())))
    if ( !so ) return;

    mAddMenuItemCond( tb, &changematerialmnuitem_, true, false,
		      selattrib_==-1 && so->allowMaterialEdit() );
}


void uiVisPartServer::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( !menu )
	return;

    const VisID visid( menu->menuID() );
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(visid))
    if ( !so )
	return;

    mAddMenuItemCond( menu, &resetmanipmnuitem_,
		      so->isManipulated() && !isLocked(visid), false,
		      so->canResetManipulation() );

    MenuItemHolder* baseitem = menu->findItem( "Display" );
    if ( !baseitem ) baseitem = menu;
    mAddMenuItemCond( baseitem, &changematerialmnuitem_, true, false,
		      so->allowMaterialEdit() );

    resmnuitem_.removeItems();
    if ( so->nrResolutions()>1 )
    {
	BufferStringSet resolutions;
	for ( int idx=0; idx<so->nrResolutions(); idx++ )
	    resolutions.add( so->getResolutionName(idx) );

	resmnuitem_.createItems( resolutions );
	for ( int idx=0; idx<resmnuitem_.nrItems(); idx++ )
	    resmnuitem_.getItem(idx)->checkable = true;
	resmnuitem_.getItem(so->getResolution())->checked = true;
	mAddMenuItem( baseitem, &resmnuitem_, true, false );
    }
    else
	mResetMenuItem( &resmnuitem_ );
}


void uiVisPartServer::handleMenuCB(CallBacker* cb)
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    if ( mnuid==-1 )
	return;

    mDynamicCastGet(MenuHandler*,menu,caller);
    const VisID visid( menu->menuID() );
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(visid))
    if ( !so )
	return;

    if ( mnuid==resetmanipmnuitem_.id )
    {
	resetManipulation( visid );
	menu->setIsHandled( true );
    }
    else if ( mnuid==changematerialmnuitem_.id )
    {
	setMaterial( visid );
	menu->setIsHandled( true );
    }
    else if ( resmnuitem_.id!=-1 && resmnuitem_.itemIndex(mnuid)!=-1 )
    {
	uiTaskRunner taskrunner( appserv().parent() );
	so->setResolution( resmnuitem_.itemIndex(mnuid), &taskrunner );
	menu->setIsHandled( true );
    }
}


void uiVisPartServer::colTabChangeCB( CallBacker* )
{
    triggerTreeUpdate();
}


void uiVisPartServer::initMPEStuff()
{
    TypeSet<VisID> visids;
    findObject( typeid(visSurvey::HorizonDisplay), visids );
    findObject( typeid(visSurvey::Horizon2DDisplay), visids );
    if ( visids.isEmpty() )
	return;

    for ( const auto& id : visids )
    {
	mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,getObject(id))
	if ( emod )
	    emod->updateFromMPE();
	else
	{
	    pErrMsg( "Error: Some EM related VisIDs, as stored in session, "
		     "are not present in visBase::DataManager." );
	}
    }

    mpetools_->initFromDisplay();
}


void uiVisPartServer::storeEMObject( bool storeas )
{
    eventmutex_.lock();
    sendEvent( storeas ? evStoreEMObjectAs() : evStoreEMObject() );
}


bool uiVisPartServer::canBDispOn2DViewer( const VisID& id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so && so->canBDispOn2DViewer();
}


bool uiVisPartServer::isVerticalDisp( const VisID& id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->isVerticalPlane() : true;
}


void uiVisPartServer::displayMapperRangeEditForAttribs( const VisID& visid,
							int attribid )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    mapperrgeditordisplayid_ = visid;
    closeAndNullPtr( multirgeditwin_ );

    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(visid));
    if ( !so || !so->usesDataPacks() )
    {
	uiMSG().error( tr("Cannot display histograms for this type of data") );
	return;
    }

    const int nrattribs = attribid==-1 ? getNrAttribs(visid) : 1;
    multirgeditwin_ = new uiMultiMapperRangeEditWin( appserv().parent(),
						     nrattribs );
    if ( attribid != -1 )
	multirgeditwin_->setActiveAttribID( attribid );

    multirgeditwin_->setDeleteOnClose( false );
    mAttachCB( multirgeditwin_->rangeChange,
	       uiVisPartServer::mapperRangeEditChanged );
    mAttachCB( multirgeditwin_->sequenceChange,
	       uiVisPartServer::sequenceEditChanged );

    for ( int idx=0; idx<nrattribs; idx++ )
    {
	const int dpidx = attribid==-1 ? idx : attribid;
	const int statsidx = attribid==-1 ? idx : 0;

	ConstRefMan<DataPack> dp = getDataPack( visid, dpidx );
	if ( !dp )
	    continue;

	const int textureidx = selectedTexture( visid, dpidx );
	multirgeditwin_->setDataPack( statsidx, dp.ptr(), textureidx );
	const ColTab::MapperSetup* ms = getColTabMapperSetup( visid, dpidx );
	if ( ms )
	    multirgeditwin_->setColTabMapperSetup( statsidx, *ms );

	const ColTab::Sequence* ctseq = getColTabSequence( visid, dpidx );
	if ( ctseq )
	    multirgeditwin_->setColTabSeq( statsidx, *ctseq );
    }

    multirgeditwin_->go();
}


void uiVisPartServer::lock( const VisID& id, bool yn )
{
    visBase::DataObject* dobj = getObject(id);
    mDynamicCastGet(visSurvey::SurveyObject*,so,dobj);
    if ( !so )
	return;

    const TypeSet<VisID>& selected = visBase::DM().selMan().selected();
    so->lock( yn );

    updateManipulatorStatus( dobj, selected.isPresent(id) );
}


bool uiVisPartServer::isLocked( const VisID& id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so && so->isLocked();
}


OD::Color uiVisPartServer::getSceneAnnotCol( int sceneidx )
{
    return scenes_[ sceneidx ]->getAnnotColor();
}


void uiVisPartServer::displaySceneColorbar( bool yn )
{
    for ( auto* scene : scenes_ )
    {
	visBase::SceneColTab* scenecoltab = scene->getSceneColTab();
	if ( scenecoltab )
	{
	    scenecoltab->setLegendColor( scene->getAnnotColor() );
	    scenecoltab->turnOn( yn );
	}
    }
}


bool uiVisPartServer::sceneColorbarDisplayed()
{
    if ( !scenes_.isEmpty() )
    {
	visBase::SceneColTab* scenecoltab = scenes_[0]->getSceneColTab();
	return scenecoltab ? scenecoltab->isOn() : false;
    }

    return false;
}


void uiVisPartServer::manageSceneColorbar( const SceneID& sceneid )
{
    RefMan<visSurvey::Scene> scene = getScene( sceneid );
    if ( !scene )
	return;

    uiSceneColorbarMgr dlg( appserv().parent(), scene->getSceneColTab() );
    dlg.go();
}


void uiVisPartServer::mapperRangeEditChanged( CallBacker* cb )
{
    mapperrgeditinact_ = true;

    mDynamicCastGet(uiMultiMapperRangeEditWin*,obj,cb);
    setColTabMapperSetup( mapperrgeditordisplayid_, obj->activeAttrbID(),
			  obj->activeMapperSetup() );
    eventmutex_.lock();
    eventobjid_ = mapperrgeditordisplayid_;
    eventattrib_ = obj->activeAttrbID();
    sendEvent( evColorTableChange() );
}


void uiVisPartServer::sequenceEditChanged( CallBacker* cb )
{
    mapperrgeditinact_ = true;

    mDynamicCastGet(uiMultiMapperRangeEditWin*,obj,cb);
    setColTabSequence( mapperrgeditordisplayid_, obj->activeAttrbID(),
		       obj->activeSequence() );
    eventmutex_.lock();
    eventobjid_ = mapperrgeditordisplayid_;
    eventattrib_ = obj->activeAttrbID();
    sendEvent( evColorTableChange() );
    triggerTreeUpdate();
}


void uiVisPartServer::setCurInterObjID( const VisID& visid )
{ curinterpobjid_ = visid; }


VisID uiVisPartServer::getCurInterObjID() const
{ return curinterpobjid_; }


void uiVisPartServer::setMoreObjectsToDoHint( const SceneID& sceneid, bool yn )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( scene )
	scene->setMoreObjectsToDoHint( yn );
}


bool uiVisPartServer::getMoreObjectsToDoHint( const SceneID& sceneid ) const
{
    const visSurvey::Scene* scene = getScene( sceneid );
    return scene && scene->getMoreObjectsToDoHint();
}


mStartAllowDeprecatedSection

bool uiVisPartServer::setDataPackID( const VisID& id, int attrib,
				     const DataPackID& dpid )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so )
	return false;

    uiTaskRunner taskrunner( appserv().parent() );
    const bool res = so->setDataPackID( attrib, dpid, &taskrunner );
    if ( res && multirgeditwin_ && !multirgeditwin_->isHidden()
	 && id==mapperrgeditordisplayid_ )
    {
	const DataPackMgr::MgrID dpmid = getDataPackMgrID( id );
	multirgeditwin_->setDataPackID( attrib, dpid, dpmid,
					so->selectedTexture(attrib) );
    }

    return res;
}


DataPackID uiVisPartServer::getDataPackID( const VisID& id, int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    ConstRefMan<DataPack> dp = so ? so->getDataPack( attrib ) : nullptr;
    return dp ? dp->id() : DataPackID::udf();
}


DataPackID uiVisPartServer::getDisplayedDataPackID( const VisID& id,
						    int attrib )const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    ConstRefMan<DataPack> dp = so ? so->getDisplayedDataPack( attrib ):nullptr;
    return dp ? dp->id() : DataPackID::udf();
}


DataPackMgr::MgrID uiVisPartServer::getDataPackMgrID( const VisID& id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getDataPackMgrID() : DataPack::MgrID::udf();
}

mStopAllowDeprecatedSection


// uiVisModeMgr

uiVisModeMgr::uiVisModeMgr( uiVisPartServer* p )
    : visserv(*p)
{
}


uiVisModeMgr::~uiVisModeMgr()
{
}


bool uiVisModeMgr::allowTurnOn( const VisID& id, bool doclean )
{
    if ( !visserv.issolomode_ )
	return !doclean;

    const TypeSet<VisID>& selected = visBase::DM().selMan().selected();
    for ( int idx=0; idx<selected.size(); idx++ )
    {
	if ( selected[idx] == id )
	{
	    if ( doclean )
		visserv.updateDisplay( doclean, VisID::udf(), id );

	    return true;
	}
    }

    return false;
}
