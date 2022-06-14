/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          Mar 2002
________________________________________________________________________

-*/

#include "uivispartserv.h"

#include "attribsel.h"
#include "binidvalset.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "flatview.h"
#include "iopar.h"
#include "mousecursor.h"
#include "mouseevent.h"
#include "oddirs.h"
#include "seisbuf.h"
#include "separstr.h"
#include "survinfo.h"
#include "zaxistransform.h"

#include "uiattribtransdlg.h"
#include "uitoolbutton.h"
#include "uifiledlg.h"
#include "uimaterialdlg.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uimain.h"
#include "uimpeman.h"
#include "uimapperrangeeditordlg.h"
#include "uiposprovider.h"
#include "uiscenecolorbarmgr.h"
#include "uisurvtopbotimg.h"
#include "uitaskrunner.h"
#include "uivisslicepos3d.h"
#include "uitoolbar.h"
#include "uivispickretriever.h"
#include "uiviszstretchdlg.h"
#include "uivisdirlightdlg.h"

#include "visdataman.h"
#include "visemobjdisplay.h"
#include "visevent.h"
#include "vismpe.h"
#include "vismpeseedcatcher.h"
#include "visobject.h"
#include "visselman.h"
#include "visplanedatadisplay.h"
#include "vispolygonselection.h"
#include "visscenecoltab.h"
#include "vissurvobj.h"
#include "vissurvscene.h"
#include "vistexturechannels.h"
#include "vistransform.h"
#include "vistransmgr.h"
#include "zdomain.h"
#include "od_helpids.h"


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
    , menu_(*new uiMenuHandler(appserv().parent(),-1))
    , toolbar_(0)
    , resetmanipmnuitem_(tr("Reset Manipulation"),cResetManipIdx)
    , changematerialmnuitem_(m3Dots(uiStrings::sProperties()),
			     cPropertiesIdx)
    , resmnuitem_(tr("Resolution"),cResolutionIdx)
    , eventmutex_(*new Threads::Mutex)
    , tracksetupactive_(false)
    , viewmode_(false)
    , workmode_(uiVisPartServer::Interactive)
    , issolomode_(false)
    , eventobjid_(-1)
    , eventattrib_(-1)
    , selattrib_(-1)
    , mouseposstr_("")
    , blockmenus_(false)
    , xytmousepos_(Coord3::udf())
    , zfactor_(1)
    , mpetools_(0)
    , slicepostools_(0)
    , pickretriever_( new uiVisPickRetriever(this) )
    , nrscenesChange(this)
    , keyEvent(this)
    , mouseEvent(this)
    , planeMovedEvent(this)
    , seltype_((int)visBase::PolygonSelection::Off)
    , multirgeditwin_(0)
    , mapperrgeditordisplayid_(-1)
    , mapperrgeditinact_(false)
    , curinterpobjid_(-1)
    , dirlightdlg_(0)
    , mousecursorexchange_(0)
    , objectAdded(this)
    , objectRemoved(this)
    , selectionmode_(Polygon)
    , selectionmodeChange(this)
    , topsetupgroupname_( 0 )
    , sceneeventsrc_(0)
{
    changematerialmnuitem_.iconfnm = "disppars";

    menu_.ref();
    menu_.createnotifier.notify( mCB(this,uiVisPartServer,createMenuCB) );
    menu_.handlenotifier.notify( mCB(this,uiVisPartServer,handleMenuCB) );

    visBase::DM().selMan().selnotifier.notify(
	mCB(this,uiVisPartServer,selectObjCB) );
    visBase::DM().selMan().deselnotifier.notify(
	mCB(this,uiVisPartServer,deselectObjCB) );
    visBase::DM().selMan().updateselnotifier.notify(
	mCB(this,uiVisPartServer,updateSelObjCB) );

    vismgr_ = new uiVisModeMgr(this);
    pickretriever_->ref();
    PickRetriever::setInstance( pickretriever_ );
}


void uiVisPartServer::unlockEvent()
{ eventmutex_.unLock(); }


bool uiVisPartServer::sendVisEvent( int evid )
{
    eventmutex_.lock();
    return sendEvent( evid );
}


uiVisPartServer::~uiVisPartServer()
{
    visBase::DM().selMan().selnotifier.remove(
	    mCB(this,uiVisPartServer,selectObjCB) );
    visBase::DM().selMan().deselnotifier.remove(
	    mCB(this,uiVisPartServer,deselectObjCB) );
    visBase::DM().selMan().updateselnotifier.remove(
	    mCB(this,uiVisPartServer,updateSelObjCB) );

    deleteAllObjects();
    delete vismgr_;

    delete &eventmutex_;
    delete mpetools_;

    menu_.createnotifier.remove( mCB(this,uiVisPartServer,createMenuCB) );
    menu_.handlenotifier.remove( mCB(this,uiVisPartServer,handleMenuCB) );
    menu_.unRef();

    if ( toolbar_ )
    {
	toolbar_->createnotifier.remove(
		mCB(this,uiVisPartServer,addToToolBarCB) );
	toolbar_->handlenotifier.remove(
		mCB(this,uiVisPartServer,handleMenuCB) );
	toolbar_->unRef();
    }

    pickretriever_->unRef();
    delete multirgeditwin_;
    delete dirlightdlg_;

    delete topbotdlg_;
    setMouseCursorExchange( 0 );
}


const char* uiVisPartServer::name() const  { return "Visualization"; }



void uiVisPartServer::setMouseCursorExchange( MouseCursorExchange* mce )
{
    if ( mousecursorexchange_ )
	mousecursorexchange_->notifier.remove(
		mCB(this,uiVisPartServer, mouseCursorCB ) );

    mousecursorexchange_ = mce;

    if ( mousecursorexchange_ )
	mousecursorexchange_->notifier.notify(
		mCB(this,uiVisPartServer, mouseCursorCB ) );
}


void uiVisPartServer::mouseCursorCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( const MouseCursorExchange::Info&, info,
				caller, cb );
    if ( caller==this )
	return;

    setMarkerPos( info.trkv_, -1 );
}


void uiVisPartServer::setUiObjectName( int id, const uiString& nm )
{
    visBase::DataObject* obj = visBase::DM().getObject( id );
    if ( obj )
	obj->setUiName( nm );
}


void uiVisPartServer::setObjectName( int id, const char* nm )
{
    visBase::DataObject* obj = visBase::DM().getObject( id );
    if ( obj )
	obj->setName( nm );
}


uiString uiVisPartServer::getUiObjectName( int id ) const
{
    visBase::DataObject* obj = visBase::DM().getObject( id );
    if ( !obj ) return uiString::emptyString();
    return obj->uiName();
}


Pos::GeomID uiVisPartServer::getGeomID( int id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getGeomID() : Survey::GM().cUndefGeomID();
}


int uiVisPartServer::addScene( visSurvey::Scene* newscene )
{
    if ( !newscene ) newscene = visSurvey::Scene::create();
    newscene->mouseposchange.notify( mCB(this,uiVisPartServer,mouseMoveCB) );
    newscene->keypressed.notify( mCB(this,uiVisPartServer,keyEventCB) );
    newscene->mouseclicked.notify( mCB(this,uiVisPartServer,mouseEventCB) );
    newscene->ref();
    scenes_ += newscene;
    pickretriever_->addScene( newscene );
    if ( isSoloMode() )
    {
	TypeSet<int> dispids;
	displayids_ += dispids;
    }

    nrscenesChange.trigger();
    return newscene->id();
}


void uiVisPartServer::removeScene( int sceneid )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( scene )
    {
	const int typesetidx = scenes_.indexOf(scene);
	if ( displayids_.validIdx(typesetidx) )
	    displayids_.removeSingle( typesetidx );

	scene->mouseposchange.remove( mCB(this,uiVisPartServer,mouseMoveCB) );
	scene->keypressed.remove( mCB(this,uiVisPartServer,keyEventCB) );
	scene->mouseclicked.remove( mCB(this,uiVisPartServer,mouseEventCB) );
	pickretriever_->removeScene( scene );
	scene->unRef();
	scenes_ -= scene;
	nrscenesChange.trigger();
	return;
    }
}


bool uiVisPartServer::clickablesInScene( const char* trackertype,
					 int sceneid ) const
{
    TypeSet<int> sceneobjids;
    getChildIds( sceneid, sceneobjids );
    for ( int idx=0; idx<sceneobjids.size(); idx++ )
    {
	const int objid = sceneobjids[idx];
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
    toolbar_->ref();
    toolbar_->createnotifier.notify( mCB(this,uiVisPartServer,addToToolBarCB) );
    toolbar_->handlenotifier.notify( mCB(this,uiVisPartServer,handleMenuCB) );

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
    if ( scenes_.isEmpty() ) return false;

    const bool res = scenes_[0]->blockMouseSelection( yn );
    for ( int idx=1; idx<scenes_.size(); idx++ )
	scenes_[idx]->blockMouseSelection( yn );

    return res;
}


bool uiVisPartServer::showMenu( int id, int menutype, const TypeSet<int>* path,
				const Coord3& pickedpos )
{
    if ( blockmenus_ )
	return true;

    menu_.setMenuID( id );
    menu_.setPickedPos(pickedpos);
    return menu_.executeMenu(menutype,path);
}


MenuHandler* uiVisPartServer::getMenuHandler()
{ return &menu_; }

MenuHandler* uiVisPartServer::getToolBarHandler()
{ return toolbar_; }


void uiVisPartServer::shareObject( int sceneid, int id )
{
    visSurvey::Scene* scene = getScene( sceneid );
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


void uiVisPartServer::findObject( const std::type_info& ti, TypeSet<int>& res )
{
    visBase::DM().getIDs( ti, res );
}


void uiVisPartServer::findObject( const MultiID& mid, TypeSet<int>& res )
{
    res.erase();
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


int uiVisPartServer::highestID() const
{
    return visBase::DM().highestID();
}


visBase::DataObject* uiVisPartServer::getObject( int id ) const
{
    mDynamicCastGet(visBase::DataObject*,dobj,visBase::DM().getObject(id))
    return dobj;
}


void uiVisPartServer::addObject( visBase::DataObject* dobj, int sceneid,
				 bool saveinsessions )
{
    mDynamicCastGet(visSurvey::Scene*,scene,visBase::DM().getObject(sceneid));
    if ( !scene ) return;

    scene->addObject( dobj );
    objectAdded.trigger( dobj->id() );

    mDynamicCastGet( visSurvey::SurveyObject*, surobj, dobj );
    if ( surobj )
	surobj->setSaveInSessionsFlag( saveinsessions );

    setUpConnections( dobj->id() );
    if ( isSoloMode() )
    {
	const int typesetidx = scenes_.indexOf(scene);
	if ( displayids_.validIdx(typesetidx) )
	    displayids_[typesetidx] += dobj->id();

	turnOn( dobj->id(), true, true );
    }
}


void uiVisPartServer::removeObject( visBase::DataObject* dobj, int sceneid )
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


MultiID uiVisPartServer::getMultiID( int id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getMultiID() : MultiID::udf();
}


int uiVisPartServer::getSelObjectId() const
{
    const TypeSet<int>& sel = visBase::DM().selMan().selected();
    return sel.size() ? sel[0] : -1;
}


int uiVisPartServer::getSelAttribNr() const
{ return selattrib_; }


void uiVisPartServer::setSelObjectId( int id, int attrib )
{
    visBase::DM().selMan().select( id );
    if ( id==-1 )
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


int uiVisPartServer::getSceneID( int visid ) const
{
    TypeSet<int> sceneids;
    getChildIds( -1, sceneids );
    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	TypeSet<int> childids;
	getChildIds( sceneids[idx], childids );
	if ( childids.isPresent(visid) )
	    return sceneids[idx];
    }

    return -1;
}


const ZDomain::Info* uiVisPartServer::zDomainInfo( int sceneid ) const
{
    const visSurvey::Scene* scene = getScene( sceneid );
    return scene ? &scene->zDomainInfo() : 0;
}


void uiVisPartServer::getSceneIds( TypeSet<int>& sceneids ) const
{ getChildIds( -1, sceneids ); }


void uiVisPartServer::getChildIds( int id, TypeSet<int>& childids ) const
{
    childids.erase();

    if ( id==-1 )
    {
	for ( int idx=0; idx<scenes_.size(); idx++ )
	    childids += scenes_[idx]->id();

	return;
    }

    const visSurvey::Scene* scene = getScene( id );
    if ( scene )
    {
	for ( int idx=0; idx<scene->size(); idx++ )
	    childids += scene->getObject( idx )->id();

	return;
    }

    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    if ( so ) so->getChildren( childids );
}


uiVisPartServer::AttribFormat
    uiVisPartServer::getAttributeFormat( int id, int attrib ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    if ( !so ) return None;

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


bool uiVisPartServer::canHaveMultipleAttribs( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->canHaveMultipleAttribs() : false;
}


bool uiVisPartServer::canAddAttrib( int id, int nr ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->canAddAttrib(nr) : false;
}


bool uiVisPartServer::canRemoveAttrib( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->canRemoveAttrib() : false;
}


bool uiVisPartServer::canRemoveDisplay( int id ) const
{
    mDynamicCastGet( visSurvey::SurveyObject*, so, getObject(id) );
    return so ? so->canBeRemoved() : false;
}


int uiVisPartServer::addAttrib( int id )
{
    if ( !canHaveMultipleAttribs(id) )
	return -1;

    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return -1;

    return so->addAttrib() ? so->nrAttribs()-1 : -1;
}


void uiVisPartServer::removeAttrib( int id, int attrib )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return;

    so->removeAttrib( attrib );
    selattrib_ = -1;
}

int uiVisPartServer::getNrAttribs( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return 0;

    return so->nrAttribs();
}


void uiVisPartServer::getAttribPosName(int id, int attrib,
				       uiString& res ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return;

    return so->getChannelName( attrib, res );
}


bool uiVisPartServer::swapAttribs( int id, int attrib0, int attrib1 )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return false;

    return so->swapAttribs( attrib0, attrib1 );
}


void uiVisPartServer::showAttribTransparencyDlg( int id, int attrib )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return;

    uiAttribTransDlg dlg( appserv().parent(), *so, attrib );
    dlg.go();
}


unsigned char uiVisPartServer::getAttribTransparency( int id,int attrib ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return 0;

    return so->getAttribTransparency( attrib );
}


void uiVisPartServer::setAttribTransparency( int id, int attrib,
					     unsigned char nv )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so )
	so->setAttribTransparency( attrib, nv );
}


TrcKeyZSampling uiVisPartServer::getTrcKeyZSampling( int id,
						     int attribid ) const
{
    TrcKeyZSampling res;
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) res = so->getTrcKeyZSampling( attribid );
    return res;
}


DataPack::ID uiVisPartServer::getDataPackID( int id, int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getDataPackID( attrib ) : -1;
}


DataPack::ID uiVisPartServer::getDisplayedDataPackID( int id, int attrib )const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getDisplayedDataPackID( attrib ) : -1;
}


DataPackMgr::ID	uiVisPartServer::getDataPackMgrID( int id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getDataPackMgrID() : -1;
}


int uiVisPartServer::currentVersion( int id, int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    if ( !so || !so->getChannels() ) return -1;
    return so->getChannels()->currentVersion( attrib );
}


bool uiVisPartServer::setDataPackID( int id, int attrib, DataPack::ID dpid )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so )
	return false;

    uiTaskRunner taskrunner( appserv().parent() );
    const bool res = so->setDataPackID( attrib, dpid, &taskrunner );

    if ( res && multirgeditwin_ && id == mapperrgeditordisplayid_ )
	multirgeditwin_->setDataPackID(
		attrib, dpid, so->selectedTexture(attrib) );

    return res;
}


const RegularSeisDataPack* uiVisPartServer::getCachedData(
						    int id, int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getCacheVolume( attrib ) : 0;
}


bool uiVisPartServer::setCubeData( int id, int attrib,
				   const RegularSeisDataPack* attribdata )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so )
	return false;

    MouseCursorChanger cursorlock( MouseCursor::Wait );
    return so->setDataVolume( attrib, attribdata, 0 );
}


bool uiVisPartServer::canHaveMultipleTextures(int id) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) return so->canHaveMultipleTextures();
    return false;
}


int uiVisPartServer::nrTextures( int id, int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) return so->nrTextures( attrib );
    return 0;
}


void uiVisPartServer::selectTexture( int id, int attrib, int textureidx )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so && isAttribEnabled(id,attrib) )
	so->selectTexture( attrib, textureidx );
}


void uiVisPartServer::setTranslation( int id, const Coord3& shift )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) so->setTranslation( shift );
}


Coord3 uiVisPartServer::getTranslation( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getTranslation() : Coord3::udf();
}


int uiVisPartServer::selectedTexture( int id, int attrib ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->selectedTexture( attrib ) : 0;
}


void uiVisPartServer::getRandomPos( int id, DataPointSet& dtps ) const
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return;

    uiTaskRunner taskrunner( appserv().parent() );
    so->getRandomPos( dtps, &taskrunner );
}


void uiVisPartServer::getRandomPosCache( int id, int attrib,
					 DataPointSet& dtps ) const
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) so->getRandomPosCache( attrib, dtps );
}


void uiVisPartServer::setRandomPosData( int id, int attrib,
					const DataPointSet* dtps )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return;

    uiTaskRunner taskrunner( appserv().parent() );
    so->setRandomPosData( attrib, dtps, &taskrunner );
}


void uiVisPartServer::getDataTraceBids( int id, TypeSet<BinID>& bids ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so )
	so->getDataTraceBids( bids );
}


Interval<float> uiVisPartServer::getDataTraceRange( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getDataTraceRange() : Interval<float>(0,0);
}


Coord3 uiVisPartServer::getMousePos() const
{ return xytmousepos_; }


BufferString uiVisPartServer::getMousePosVal() const
{ return mouseposval_; }


BufferString uiVisPartServer::getInteractionMsg( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    return so ? so->getManipulationString() : BufferString("");
}


void uiVisPartServer::getObjectInfo( int id, BufferString& info ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    if ( so ) so->getObjectInfo( info );
}


const ColTab::MapperSetup*
    uiVisPartServer::getColTabMapperSetup( int id, int attrib,
					   int version ) const
{
    mDynamicCastGet( const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getColTabMapperSetup( attrib, version ) : 0;
}


void uiVisPartServer::setColTabMapperSetup( int id, int attrib,
					    const ColTab::MapperSetup& ms )
{
    mDynamicCastGet( visSurvey::SurveyObject*, so, getObject(id) );
    if ( !so ) return;

    so->setColTabMapperSetup( attrib, ms, 0 );
    if ( so->getScene() && so->getScene()->getSceneColTab() )
	so->getScene()->getSceneColTab()->setColTabMapperSetup( ms );

    if ( multirgeditwin_ && id==mapperrgeditordisplayid_ )
    {
	if ( mapperrgeditinact_ )
	    mapperrgeditinact_ = false;
	else
	    multirgeditwin_->setColTabMapperSetup( attrib, ms );
    }
}


const ColTab::Sequence*
    uiVisPartServer::getColTabSequence( int id, int attrib ) const
{
    mDynamicCastGet( const visSurvey::SurveyObject*,so,getObject(id))
    return so ? so->getColTabSequence( attrib ) : 0;
}


bool uiVisPartServer::canHandleColTabSeqTrans( int id, int attrib ) const
{
    mDynamicCastGet( const visSurvey::SurveyObject*,so,getObject(id))
    return so ? so-> canHandleColTabSeqTrans( attrib ) : false;
}


bool uiVisPartServer::canSetColTabSequence( int id ) const
{
    mDynamicCastGet( const visSurvey::SurveyObject*,so,getObject(id))
    return so ? so->canSetColTabSequence() : false;
}


void uiVisPartServer::setColTabSequence( int id, int attrib,
					 const ColTab::Sequence& seq )
{
    mDynamicCastGet( visSurvey::SurveyObject*, so, getObject(id) );
    if ( !so ) return;

    so->setColTabSequence( attrib, seq, 0 );
    if ( so->getScene() && so->getScene()->getSceneColTab() )
	so->getScene()->getSceneColTab()->setColTabSequence( seq );

    if ( multirgeditwin_ && id == mapperrgeditordisplayid_ )
    {
	if ( mapperrgeditinact_ )
	    mapperrgeditinact_ = false;
	else
	    multirgeditwin_->setColTabSeq( attrib, seq );
    }
}


void uiVisPartServer::fillDispPars( int id, int attrib,
				    FlatView::DataDispPars& common,
				    bool wva ) const
{
    const ColTab::MapperSetup* mapper = getColTabMapperSetup( id, attrib );
    const ColTab::Sequence* seq = getColTabSequence( id, attrib );
    if ( !mapper || !seq )
	return;

    FlatView::DataDispPars::Common* compars;
    if ( wva )
	compars = &common.wva_;
    else
	compars = &common.vd_;

    if ( !wva )
    {
	common.vd_.ctab_ = seq->name();
	common.vd_.show_ = true;
    }
    else
	common.wva_.show_ = true;

    compars->mappersetup_ = *mapper;
}


const TypeSet<float>* uiVisPartServer::getHistogram( int id, int attrib ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*, so, getObject(id) );
    return so ? so->getHistogram( attrib ) : 0;
}


int uiVisPartServer::getEventObjId() const { return eventobjid_; }


int uiVisPartServer::getEventAttrib() const { return eventattrib_; }


const TypeSet<Attrib::SelSpec>* uiVisPartServer::getSelSpecs(
						int id, int attrib ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getSelSpecs( attrib ) : 0;
}


const Attrib::SelSpec* uiVisPartServer::getSelSpec( int id, int attrib ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getSelSpec(attrib,selectedTexture(id,attrib)) : 0;
}


bool uiVisPartServer::interpolationEnabled( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->textureInterpolationEnabled() : true;
}


void uiVisPartServer::enableInterpolation( int id, bool yn )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return;

    so->enableTextureInterpolation( yn );
}


bool uiVisPartServer::isAngle( int id, int attrib ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->isAngle( attrib ) : false;
}


void uiVisPartServer::setAngleFlag( int id, int attrib, bool yn )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return;

    so->setAngleFlag( attrib, yn );
}


bool uiVisPartServer::isAttribEnabled( int id, int attrib ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->isAttribEnabled( attrib ) : false;
}


void uiVisPartServer::triggerObjectMoved( int id )
{
    visBase::DataObject* dobj = visBase::DM().getObject( id );
    if ( !dobj )
	return;

    TypeSet<int> sceneids;
    getChildIds( -1, sceneids );
    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	visSurvey::Scene* scene =
	    (visSurvey::Scene*) visBase::DM().getObject( sceneids[idx] );

	if ( scene )
	    scene->objectMoved( dobj );
    }
}


void uiVisPartServer::enableAttrib( int id, int attrib, bool yn )
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


bool uiVisPartServer::hasSingleColorFallback( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->hasSingleColorFallback() : false;
}


bool uiVisPartServer::deleteAllObjects()
{
    mpetools_->deleteVisObjects();

    while ( scenes_.size() )
	removeScene( scenes_[0]->id() );

    objectRemoved.trigger( -1 );
    if ( multirgeditwin_ )
    {
	multirgeditwin_->close();
	deleteAndZeroPtr( multirgeditwin_ );
    }

    deleteAndZeroPtr( topbotdlg_ );

    scenes_.erase();
    nrscenesChange.trigger();
    return true; //visBase::DM().removeAll();
}


void uiVisPartServer::setViewMode( bool yn, bool notify)
{
    if ( yn==viewmode_ ) return;
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


void uiVisPartServer::setSoloMode( bool yn, TypeSet< TypeSet<int> > dispids,
				   int selid )
{
    issolomode_ = yn;
    displayids_ = dispids;
    updateDisplay( issolomode_, selid );
}


void uiVisPartServer::setSelectionMode( uiVisPartServer::SelectionMode mode )
{
    if ( isSelectionModeOn() && mode==Polygon )
	seltype_ = (int) visBase::PolygonSelection::Polygon;
    if ( isSelectionModeOn() && mode==Rectangle )
	seltype_ = (int) visBase::PolygonSelection::Rectangle;

    for ( int sceneidx=0; sceneidx<scenes_.size(); sceneidx++ )
    {
	visSurvey::Scene* scene = scenes_[sceneidx];
	if ( scene->getPolySelection() )
	{
	    scene->getPolySelection()->setSelectionType(
	    (visBase::PolygonSelection::SelectionType) seltype_ );
	}
	if ( getSelObjectId()>=0 )
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
    seltype_ = !yn ? (int) visBase::PolygonSelection::Off
		   : (int) visBase::PolygonSelection::Rectangle; // Dummy

    setSelectionMode( selectionmode_ );
    updateDraggers();

    if ( !yn )
    {
	const int selid = getSelObjectId();
	mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(selid))
	if ( so ) so->clearSelections();
    }
}


uiVisPartServer::SelectionMode uiVisPartServer::getSelectionMode() const
{ return selectionmode_; }


bool uiVisPartServer::isSelectionModeOn() const
{ return seltype_ != (int) visBase::PolygonSelection::Off; }


const Selector<Coord3>* uiVisPartServer::getCoordSelector( int sceneid ) const
{
    const visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return 0;

    return scene->getSelector();
}


int uiVisPartServer::getTypeSetIdx( int selid )
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


void uiVisPartServer::updateDisplay( bool doclean, int selid, int refid )
{
    int typesetidx;
    typesetidx = getTypeSetIdx( selid != -1 ? selid : refid );

    if ( doclean && displayids_.size()>typesetidx )
    {
	for ( int idx=0; idx<displayids_[typesetidx].size(); idx++ )
	    if ( isOn( displayids_[typesetidx][idx] )
		 && displayids_[typesetidx][idx] != selid )
		   // later: include check if displayid is a sub-item of selid
		turnOn(displayids_[typesetidx][idx], false);

	if ( !isOn(selid) && selid >= 0 ) turnOn( selid, true);
	else if ( !isOn(refid) && refid >= 0 ) turnOn( refid, true);
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


void uiVisPartServer::setTopBotImg( int sceneid )
{
    delete topbotdlg_;
    topbotdlg_ = new uiSurvTopBotImageDlg( appserv().parent(),
							  getScene(sceneid) );
    topbotdlg_->setModal( false );
    topbotdlg_->show();
}


void uiVisPartServer::updateDraggers()
{
    const TypeSet<int>& selected = visBase::DM().selMan().selected();

    for ( int sceneidx=0; sceneidx<scenes_.size(); sceneidx++ )
    {
	visSurvey::Scene* scene = scenes_[sceneidx];

	scene->enableTraversal( visBase::cDraggerIntersecTraversalMask(),
				scene->isPickable() &&
				seltype_==(int)visBase::PolygonSelection::Off );

	for ( int objidx=0; objidx<scene->size(); objidx++ )
	{
	    visBase::DataObject* dobj = scene->getObject( objidx );
	    updateManipulatorStatus( dobj, selected.isPresent(dobj->id()) );
	}
    }
}



void uiVisPartServer::setZStretch()
{
    uiZStretchDlg dlg( appserv().parent() );
    dlg.vwallcb = mCB(this,uiVisPartServer,vwAll);
    dlg.homecb = mCB(this,uiVisPartServer,toHome);
    dlg.go();
}


void uiVisPartServer::setZAxisTransform( int sceneid, ZAxisTransform* zat,
					 TaskRunner* taskrunner )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( zat && scene )
	scene->setZAxisTransform( zat, taskrunner );
}


const ZAxisTransform* uiVisPartServer::getZAxisTransform( int sceneid ) const
{
    const visSurvey::Scene* scene = getScene( sceneid );
    return scene ? scene->getZAxisTransform() : 0;
}


visBase::EventCatcher* uiVisPartServer::getEventCatcher( int sceneid )
{
    visSurvey::Scene* scene = getScene( sceneid );
    return scene ? &scene->eventCatcher() : 0;
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

    TypeSet<int> sceneids;
    getChildIds( -1, sceneids );

    for ( int ids=0; ids<sceneids.size(); ids++ )
    {
	int sceneid = sceneids[ids];
	visBase::DataObject* obj = visBase::DM().getObject( sceneid );
	mDynamicCastGet(visSurvey::Scene*,scene,obj)
	if ( scene && scene->zDomainInfo().def_==ZDomain::SI() )
	    scene->setTrcKeyZSampling( SI().sampling(true) );
    }

    return true;
}


void uiVisPartServer::setOnlyAtSectionsDisplay( int id, bool yn )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    if ( so ) so->setOnlyAtSectionsDisplay( yn );
}


bool uiVisPartServer::displayedOnlyAtSections( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
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
	zrg.start = fms.getFValue( 4 ); zrg.stop = fms.getFValue( 5 );
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
	int sceneid = -1;
	BufferString scenekey = sceneidpar.compKey( key.buf(),sKeySceneID() );
	if ( !par.get( scenekey, sceneid ) )
	    continue;

	if ( visBase::DM().getObject( sceneid ) )
	    continue;

	RefMan<visSurvey::Scene> newscene = visSurvey::Scene::create();
	newscene->setID( sceneid );
	addScene( newscene );

	IOPar* scenepar = par.subselect( key.buf() );
	if ( !scenepar )
	    continue;
	newscene->usePar( *scenepar );

	for ( int objidx=0; objidx<newscene->size(); objidx++ )
	{
	    int objid = newscene->getObject(objidx)->id();
	    setUpConnections( objid );
	}

    }

    objectAdded.trigger( -1 );

    mpetools_->initFromDisplay();

    return true;
}


void uiVisPartServer::movePlaneAndCalcAttribs( int id,
	const TrcKeyZSampling& tkzs )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,getObject(id))
    if ( !pdd ) return;

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
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	TypeSet<int> children;
	getChildIds( scenes_[idx]->id(), children );
	for ( int idy=0; idy<children.size(); idy++ )
	{
	    const int childid = children[idy];
	    calculateAllAttribs( childid );
	}
    }
}


void uiVisPartServer::calculateAllAttribs( int id )
{
    visBase::DataObject* dobj = visBase::DM().getObject( id );
    mDynamicCastGet(visSurvey::SurveyObject*,so,dobj);
    if ( !so ) return;

    for ( int attrib=0; attrib<so->nrAttribs(); attrib++ )
	calculateAttrib( id, attrib, false, true );
}


void uiVisPartServer::fillPar( IOPar& par ) const
{
    visBase::DM().fillPar( par );

    par.set( sKeyNumberScenes(), scenes_.size() );
    const TrcKeyZSampling& cs = SI().sampling( true );
    FileMultiString fms;
    fms += cs.hsamp_.start_.inl();
    fms += cs.hsamp_.stop_.inl();
    fms += cs.hsamp_.start_.crl();
    fms += cs.hsamp_.stop_.crl();
    fms += cs.zsamp_.start;
    fms += cs.zsamp_.stop;
    par.set( sKeyWorkArea(), fms );

    par.set( sKeySliceSteps(),
	     sCast(float,slicepostools_->getStep(OD::InlineSlice)),
	     sCast(float,slicepostools_->getStep(OD::CrosslineSlice)),
	     slicepostools_->getZStep() );

    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	IOPar scenepar;
	BufferString key( sceneprefix(), idx );
	scenepar.set( sKeySceneID(), scenes_[idx]->id() );
	scenes_[idx]->fillPar( scenepar );
	par.mergeComp( scenepar, key.buf() );
    }
}


void uiVisPartServer::turnOn( int id, bool yn, bool doclean )
{
    if ( !vismgr_->allowTurnOn(id,doclean) )
	yn = false;

    mDynamicCastGet(visBase::VisualObject*,vo,getObject(id))
    if ( !vo || vo->isOn()==yn )
	return;

    vo->turnOn( yn );
    triggerObjectMoved( id );
}


bool uiVisPartServer::isOn( int id ) const
{
    const visBase::DataObject* dobj = visBase::DM().getObject( id );
    mDynamicCastGet(const visBase::VisualObject*,vo,dobj)
    return vo ? vo->isOn() : false;
}


bool uiVisPartServer::canDuplicate( int id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so && so->canDuplicate();
}


int uiVisPartServer::duplicateObject( int id, int sceneid )
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return -1;

    uiTaskRunner taskrunner( appserv().parent() );

    visSurvey::SurveyObject* newso = so->duplicate( &taskrunner );
    if ( !newso ) return -1;

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

    const TypeSet<int>& sel = visBase::DM().selMan().selected();
    if ( sel.size()!=1 ) return false;

    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(sel[0]));
    if ( !so ) return false;

    return so->isPicking();
}


void uiVisPartServer::getPickingMessage( BufferString& str ) const
{
    str = "";
    const TypeSet<int>& sel = visBase::DM().selMan().selected();
    if ( sel.size()!=1 ) return;
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(sel[0]));
    if ( so && so->isPicking() )
	so->getPickingMessage( str );
}


visSurvey::Scene* uiVisPartServer::getScene( int sceneid )
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	if ( scenes_[idx]->id()==sceneid )
	{
	    return scenes_[idx]; \
	}
    }

    return 0;
}


const visSurvey::Scene* uiVisPartServer::getScene( int sceneid ) const
{
    return const_cast<uiVisPartServer*>(this)->getScene( sceneid );
}


void uiVisPartServer::removeObject( int id, int sceneid )
{
    removeConnections( id );

    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return;

    const int idx = scene->getFirstIdx( id );
    if ( idx!=-1 )
    {
	scene->removeObject( idx );
	objectRemoved.trigger( id );
    }
}


void uiVisPartServer::removeSelection()
{
    TypeSet<int> sceneids;
    getChildIds( -1, sceneids );
    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	const Selector<Coord3>* sel = getCoordSelector( sceneids[idx] );
	if ( !sel ) continue;

	int selobjectid = getSelObjectId();
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


bool uiVisPartServer::hasAttrib( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so && so->getAttributeFormat() != visSurvey::SurveyObject::None;
}


bool uiVisPartServer::selectAttrib( int id, int attrib )
{
    eventmutex_.lock();
    eventobjid_ = id;
    eventattrib_ = attrib;
    return sendEvent( evSelectAttrib() );
}


void uiVisPartServer::setSelSpec( int id, int attrib,
				  const Attrib::SelSpec& myattribspec )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) so->setSelSpec( attrib, myattribspec );
}


void uiVisPartServer::setSelSpecs( int id, int attrib,
				   const TypeSet<Attrib::SelSpec>& selspecs)
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) so->setSelSpecs( attrib, selspecs );
}


void uiVisPartServer::setUserRefs( int id, int attrib,
				   BufferStringSet* newuserrefs )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) so->setUserRefs( attrib, newuserrefs );
}


bool uiVisPartServer::calculateAttrib( int id, int attrib, bool newselect,
				       bool ignorelocked )
{
    if ( !ignorelocked && isLocked(id) )
    {
	resetManipulation(id);
	return true;
    }
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return false;

    if ( so->isManipulated() )
	so->acceptManipulation();

    const Attrib::SelSpec* as = so->getSelSpec( attrib );
    if ( !as ) return false;
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


bool uiVisPartServer::calcManipulatedAttribs( int id )
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


bool uiVisPartServer::hasMaterial( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so && so->allowMaterialEdit();
}


void uiVisPartServer::setMaterial( int id )
{
    mDynamicCastGet(visBase::VisualObject*,vo,getObject(id))
    if ( !hasMaterial(id) || !vo ) return;

    uiPropertiesDlg dlg( appserv().parent(),
			 dynamic_cast<visSurvey::SurveyObject*>(vo) );
    dlg.go();
}


bool uiVisPartServer::hasColor( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    return so && so->hasColor();
}


void uiVisPartServer::setColor( int id, const OD::Color& col )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    if ( so )
	so->setColor( col );
}


bool uiVisPartServer::writeSceneToFile( int id, const uiString& dlgtitle ) const
{
    visBase::DataObject* obj = visBase::DM().getObject( id );
    if ( !obj )
	return false;

    uiFileDialog filedlg( appserv().parent(), false, GetPersonalDir(),
			"*.osg", dlgtitle );
    filedlg.setDefaultExtension( "osg" );

    if ( !filedlg.go() )
	return false;

    const bool res = obj->serialize( filedlg.fileName() );
    if ( !res )
	uiMSG().error( tr("Could not write scene to file.") );

    return res;
}


bool uiVisPartServer::resetManipulation( int id )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) so->resetManipulation();

    eventmutex_.lock();
    eventobjid_ = id;
    sendEvent( evInteraction() );
    eventmutex_.lock();
    sendEvent( evUpdateTree() );

    return so;
}


bool uiVisPartServer::isManipulated( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so && so->isManipulated();
}


void uiVisPartServer::acceptManipulation( int id )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) so->acceptManipulation();
}


void uiVisPartServer::setUpConnections( int id )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    NotifierAccess* na = so ? so->getManipulationNotifier() : 0;
    if ( na ) na->notify( mCB(this,uiVisPartServer,interactionCB) );
    na = so ? so->getLockNotifier() : 0;
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


void uiVisPartServer::removeConnections( int id )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    NotifierAccess* na = so ? so->getManipulationNotifier() : 0;
    if ( na ) na->remove( mCB(this,uiVisPartServer,interactionCB) );
    na = so ? so->getLockNotifier() : 0;
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
    const int id = dataobj ? dataobj->id() : getSelObjectId();
    if ( id==-1 )
	return;

    Coord3 pickedpos = Coord3::udf();
    mDynamicCastGet(visBase::VisualObject*,vo,getObject(id));
    if ( vo && vo->rightClickedEventInfo() )
	pickedpos = vo->rightClickedEventInfo()->worldpickedpos;

    showMenu( id, uiMenuHandler::fromScene(),
	      dataobj ? dataobj->rightClickedPath() : 0, pickedpos );
}


void uiVisPartServer::updateManipulatorStatus( visBase::DataObject* dobj,
					       bool isselected ) const
{
    mDynamicCastGet( visSurvey::SurveyObject*, so, dobj );
    if ( !so )
	return;

    mDynamicCastGet( visSurvey::MPEDisplay*, mpedisp, so );
    if ( mpedisp )
    {
	// Tells the tracker box not to hide in view mode
	mpedisp->setPickable( workmode_==uiVisPartServer::Interactive );
	return;
    }

    const bool showmanipulator =  !so->isLocked() &&
	workmode_==uiVisPartServer::Interactive &&
	isselected;

    if ( showmanipulator!=so->isManipulatorShown() )
	so->showManipulator( showmanipulator );
}


void uiVisPartServer::selectObjCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sel,cb);
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
    mCBCapsuleUnpack( int, id, cb ); \
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
{ mUpdateSelObj( cb, id, dataobj ); }


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
				    int dontsetscene )
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
	scenes_[idx]->setMarkerPos( worldpos, dontsetscene );
}


void uiVisPartServer::mouseMoveCB( CallBacker* cb )
{
    mDynamicCast(visSurvey::Scene*,sceneeventsrc_,cb)
    if ( !sceneeventsrc_ ) return;

    const TrcKeyValue worldpos = sceneeventsrc_->getMousePos();

    xytmousepos_ = sceneeventsrc_->getMousePos( true );
    setMarkerPos( worldpos, sceneeventsrc_->id() );

    MouseCursorExchange::Info info( worldpos );
    mousecursorexchange_->notifier.trigger( info, this );

    eventmutex_.lock();
    mouseposval_ = sceneeventsrc_->getMousePosValue();
    mouseposstr_ = sceneeventsrc_->getMousePosString();
    zfactor_ = sceneeventsrc_->zDomainUserFactor();
    sendEvent( evMouseMove() );
    sceneeventsrc_ = 0;
}


void uiVisPartServer::keyEventCB( CallBacker* cb )
{
    mDynamicCast(visSurvey::Scene*,sceneeventsrc_,cb)
    if ( !sceneeventsrc_ ) return;

    eventmutex_.lock();
    kbevent_ = sceneeventsrc_->getKeyboardEvent();

    const int selid = getSelObjectId();
    if ( kbevent_.key_==OD::KB_V && kbevent_.modifier_==OD::NoButton )
    {
	setOnlyAtSectionsDisplay( selid, !displayedOnlyAtSections(selid) );
    }

    sendEvent( evKeyboardEvent() );
    keyEvent.trigger();
    sceneeventsrc_ = 0;
}


void uiVisPartServer::mouseEventCB( CallBacker* cb )
{
    mDynamicCast(visSurvey::Scene*,sceneeventsrc_,cb)
    if ( !sceneeventsrc_ ) return;

    eventmutex_.lock();
    mouseevent_ = sceneeventsrc_->getMouseEvent();
    sendEvent( evMouseEvent() );
    mouseEvent.trigger();
    sceneeventsrc_ = 0;
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
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(tb->menuID()));
    if ( !so ) return;

    mAddMenuItemCond( tb, &changematerialmnuitem_, true, false,
		      selattrib_==-1 && so->allowMaterialEdit() );
}


void uiVisPartServer::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( !menu ) return;
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(menu->menuID()));
    if ( !so ) return;

    mAddMenuItemCond( menu, &resetmanipmnuitem_,
		      so->isManipulated() && !isLocked(menu->menuID()), false,
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
    if ( mnuid==-1 ) return;

    mDynamicCastGet(MenuHandler*,menu,caller);
    const int id = menu->menuID();
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id))
    if ( !so ) return;

    if ( mnuid==resetmanipmnuitem_.id )
    {
	resetManipulation( id );
	menu->setIsHandled( true );
    }
    else if ( mnuid==changematerialmnuitem_.id )
    {
	setMaterial( id );
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
    TypeSet<int> visids;
    findObject( typeid(visSurvey::EMObjectDisplay), visids );
    for ( int idx=0; idx<visids.size(); idx++ )
    {
	mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,
			getObject(visids[idx]))
	emod->updateFromMPE();
    }

    mpetools_->initFromDisplay();
}


void uiVisPartServer::storeEMObject( bool storeas )
{
    eventmutex_.lock();
    sendEvent( storeas ? evStoreEMObjectAs() : evStoreEMObject() );
}


bool uiVisPartServer::canBDispOn2DViewer( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->canBDispOn2DViewer() : false;
}


bool uiVisPartServer::isVerticalDisp( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->isVerticalPlane() : true;
}

void uiVisPartServer::displayMapperRangeEditForAttrbs( int visid )
{ displayMapperRangeEditForAttribs( visid, -1 ); }


void uiVisPartServer::displayMapperRangeEditForAttribs(
					int visid, int attribid  )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    mapperrgeditordisplayid_ = visid;
    if ( multirgeditwin_ )
    {
	multirgeditwin_->close();
	delete multirgeditwin_;
	multirgeditwin_ = 0;
    }

    const DataPackMgr::ID dpmid = getDataPackMgrID( visid );
    if ( dpmid < 1 )
    {
	uiMSG().error( tr("Cannot display histograms for this type of data") );
	return;
    }

    const int nrattribs = attribid==-1 ? getNrAttribs(visid) : 1;
    multirgeditwin_ = new uiMultiMapperRangeEditWin( 0, nrattribs, dpmid );
    if ( attribid != -1 )
	multirgeditwin_->setActiveAttribID( attribid );
    multirgeditwin_->setDeleteOnClose( false );
    multirgeditwin_->rangeChange.notify(
	    mCB(this,uiVisPartServer,mapperRangeEditChanged) );
    multirgeditwin_->sequenceChange.notify(
	    mCB(this,uiVisPartServer,sequenceEditChanged) );

    for ( int idx=0; idx<nrattribs; idx++ )
    {
	const int dpidx = attribid==-1 ? idx : attribid;
	const int statsidx = attribid==-1 ? idx : 0;

	const DataPack::ID dpid = getDataPackID( visid, dpidx );
	if ( dpid < 1 )
	    continue;

	const int textureidx = selectedTexture( visid, dpidx );
	multirgeditwin_->setDataPackID( statsidx, dpid, textureidx );
	const ColTab::MapperSetup* ms = getColTabMapperSetup( visid, dpidx );
	if ( ms ) multirgeditwin_->setColTabMapperSetup( statsidx, *ms );

	const ColTab::Sequence* ctseq = getColTabSequence( visid, dpidx );
	if ( ctseq ) multirgeditwin_->setColTabSeq( statsidx, *ctseq );
    }

    multirgeditwin_->go();
}


void uiVisPartServer::lock( int id, bool yn )
{
    visBase::DataObject* dobj = getObject(id);
    mDynamicCastGet(visSurvey::SurveyObject*,so,dobj);
    if ( !so ) return;

    const TypeSet<int>& selected = visBase::DM().selMan().selected();
    so->lock( yn );

    updateManipulatorStatus( dobj, selected.isPresent(id) );
}


bool uiVisPartServer::isLocked( int id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return false;

    return so->isLocked();
}


OD::Color uiVisPartServer::getSceneAnnotCol( int sceneidx )
{
    return scenes_[ sceneidx ]->getAnnotColor();
}


void uiVisPartServer::displaySceneColorbar( bool yn )
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	visBase::SceneColTab* scenecoltab = scenes_[idx]->getSceneColTab();
	if ( scenecoltab )
	{
	    scenecoltab->setLegendColor( scenes_[idx]->getAnnotColor() );
	    scenecoltab->turnOn( yn );
	}
    }
}


bool uiVisPartServer::sceneColorbarDisplayed()
{
    if ( scenes_.size()>0 && scenes_[0] )
    {
	visBase::SceneColTab* scenecoltab = scenes_[0]->getSceneColTab();
	return scenecoltab ? scenecoltab->isOn() : false;
    }

    return false;
}


void uiVisPartServer::manageSceneColorbar( int sceneid )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return;

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


void uiVisPartServer::setCurInterObjID( int visid )
{ curinterpobjid_ = visid; }


int uiVisPartServer::getCurInterObjID() const
{ return curinterpobjid_; }


void uiVisPartServer::setMoreObjectsToDoHint( int sceneid, bool yn )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( scene )
	scene->setMoreObjectsToDoHint( yn );
}


bool uiVisPartServer::getMoreObjectsToDoHint( int sceneid ) const
{
    const visSurvey::Scene* scene = getScene( sceneid );
    return scene ? scene->getMoreObjectsToDoHint() : false;
}


uiVisModeMgr::uiVisModeMgr( uiVisPartServer* p )
    : visserv(*p)
{
}


bool uiVisModeMgr::allowTurnOn( int id, bool doclean )
{
    if ( !visserv.issolomode_ )
	return !doclean;

    const TypeSet<int>& selected = visBase::DM().selMan().selected();
    for ( int idx=0; idx<selected.size(); idx++ )
    {
	if ( selected[idx] == id )
	{
	    if ( doclean ) visserv.updateDisplay( doclean, -1, id );
	    return true;
	}
    }

    return false;
}
