/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          Mar 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
#include "uimpeman.h"
#include "uimapperrangeeditordlg.h"
#include "uiselsurvranges.h"
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
#include "vismpeseedcatcher.h"
#include "visobject.h"
#include "visselman.h"
#include "vispolygonselection.h"
#include "visscenecoltab.h"
#include "vissurvobj.h"
#include "vissurvscene.h"
#include "vistransform.h"
#include "vistransmgr.h"
#include "zdomain.h"


int uiVisPartServer::evUpdateTree()		    { return 0; }
int uiVisPartServer::evSelection()		    { return 1; }
int uiVisPartServer::evDeSelection()	            { return 2; }
int uiVisPartServer::evGetNewData()		    { return 3; }
int uiVisPartServer::evMouseMove()		    { return 4; }
int uiVisPartServer::evInteraction()		    { return 5; }
int uiVisPartServer::evSelectAttrib()	   	    { return 6; }
int uiVisPartServer::evViewAll()		    { return 9; }
int uiVisPartServer::evToHomePos()	   	    { return 10; }
int uiVisPartServer::evPickingStatusChange() 	    { return 11; }
int uiVisPartServer::evViewModeChange()	  	    { return 12; }
int uiVisPartServer::evShowSetupDlg()	  	    { return 13; }
int uiVisPartServer::evLoadPostponedData()   	    { return 14; }
int uiVisPartServer::evToggleBlockDataLoad() 	    { return 15; }
int uiVisPartServer::evDisableSelTracker()  	    { return 16; }
int uiVisPartServer::evColorTableChange()	    { return 17; }
int uiVisPartServer::evLoadAttribDataInMPEServ()    { return 18; }
int uiVisPartServer::evPostponedLoadingData()	    { return 19; }
int uiVisPartServer::evFromMPEManStoreEMObject()    { return 20; }
int uiVisPartServer::evGetHeadOnIntensity()	    { return 21; }
int uiVisPartServer::evSetHeadOnIntensity()	    { return 22; }



const char* uiVisPartServer::sKeyAppVel()		{ return "AppVel"; }
const char* uiVisPartServer::sKeyWorkArea()		{ return "Work Area"; }


static const int cResetManipIdx = 800;
static const int cPropertiesIdx = 600;
static const int cResolutionIdx = 500;


uiVisPartServer::uiVisPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , menu_(*new uiMenuHandler(appserv().parent(),-1))
    , toolbar_(*new uiTreeItemTBHandler(appserv().parent()))
    , resetmanipmnuitem_("R&eset Manipulation",cResetManipIdx)
    , changematerialmnuitem_("&Properties ...")
    , resmnuitem_("&Resolution",cResolutionIdx)
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
    , itemtools_(0)
    , pickretriever_( new uiVisPickRetriever(this) )
    , nrsceneschange_( this )
    , seltype_( (int) visBase::PolygonSelection::Off )
    , multirgeditwin_(0)
    , mapperrgeditordisplayid_(-1)
    , mapperrgeditinact_(false)	  
    , dirlightdlg_(0)			  
    , mousecursorexchange_( 0 )
    , objectaddedremoved(this)
    , selectionmode_( Polygon )
    , selectionmodechange(this)
{
    changematerialmnuitem_.iconfnm = "disppars";

    menu_.ref();
    menu_.createnotifier.notify( mCB(this,uiVisPartServer,createMenuCB) );
    menu_.handlenotifier.notify( mCB(this,uiVisPartServer,handleMenuCB) );

    toolbar_.ref();
    toolbar_.createnotifier.notify( mCB(this,uiVisPartServer,addToToolBarCB) );
    toolbar_.handlenotifier.notify( mCB(this,uiVisPartServer,handleMenuCB) );

    visBase::DM().selMan().selnotifier.notify( 
	mCB(this,uiVisPartServer,selectObjCB) );
    visBase::DM().selMan().deselnotifier.notify( 
	mCB(this,uiVisPartServer,deselectObjCB) );

    vismgr_ = new uiVisModeMgr(this);
    pickretriever_->ref();
    PickRetriever::setInstance( pickretriever_ );
}


void uiVisPartServer::unlockEvent()
{ eventmutex_.unLock(); }


uiVisPartServer::~uiVisPartServer()
{
    visBase::DM().selMan().selnotifier.remove(
	    mCB(this,uiVisPartServer,selectObjCB) );
    visBase::DM().selMan().deselnotifier.remove(
	    mCB(this,uiVisPartServer,deselectObjCB) );

    deleteAllObjects();
    delete vismgr_;

    delete &eventmutex_;
    delete mpetools_;
    menu_.unRef();
    toolbar_.unRef();
    pickretriever_->unRef();
    delete multirgeditwin_;
    delete dirlightdlg_;

    setMouseCursorExchange( 0 );
}


const char* uiVisPartServer::name() const  { return "Visualisation"; }



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

    setMarkerPos( info.surveypos_, -1 );
}


void uiVisPartServer::setObjectName( int id, const char* nm )
{
    visBase::DataObject* obj = visBase::DM().getObject( id );
    if ( obj ) obj->setName( nm );
}


const char* uiVisPartServer::getObjectName( int id ) const
{
    visBase::DataObject* obj = visBase::DM().getObject( id );
    if ( !obj ) return 0;
    return obj->name();
}


int uiVisPartServer::addScene( visSurvey::Scene* newscene )
{
    if ( !newscene ) newscene = visSurvey::Scene::create();
    newscene->mouseposchange.notify( mCB(this,uiVisPartServer,mouseMoveCB) );
    newscene->ref();
    scenes_ += newscene;
    newscene->getPolySelection()->setSelectionType(
	    (visBase::PolygonSelection::SelectionType) seltype_ );
    pickretriever_->addScene( newscene );
    nrsceneschange_.trigger();
    return newscene->id();
}


void uiVisPartServer::removeScene( int sceneid )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( scene )
    {
	scene->mouseposchange.remove( mCB(this,uiVisPartServer,mouseMoveCB) );
	pickretriever_->removeScene( scene );
	scene->unRef();
	scenes_ -= scene;
	nrsceneschange_.trigger();
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
    
    uiMSG().warning( "The scene yet contains no object on which seeds\n"
		     "for a '", trackertype, "' can be picked." );
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
    getTrackTB()->display( false );
    slicepostools_ = new uiSlicePos3DDisp( appserv().parent(), this );
}


bool uiVisPartServer::disabToolBars( bool yn )
{
    bool res = false;
    if ( mpetools_ )
    {
	res = !getTrackTB()->sensitive();
	getTrackTB()->setSensitive( !yn );
    }

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
{ return &toolbar_; }


void uiVisPartServer::shareObject( int sceneid, int id )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return;

    mDynamicCastGet(visBase::DataObject*,dobj,visBase::DM().getObject(id))
    if ( !dobj ) return;

    scene->addObject( dobj );
    objectaddedremoved.trigger();
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
    visBase::DM().getIds( ti, res );
}


void uiVisPartServer::findObject( const MultiID& mid, TypeSet<int>& res )
{
    res.erase();
    if ( mid == MultiID(-1) )
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
				 bool saveinsessions  )
{
    mDynamicCastGet(visSurvey::Scene*,scene,visBase::DM().getObject(sceneid))
    scene->addObject( dobj );
    objectaddedremoved.trigger();
    dobj->doSaveInSessions( saveinsessions );

    setUpConnections( dobj->id() );
    if ( isSoloMode() )
    {
	int typesetidx = scenes_.indexOf(scene);
	displayids_[typesetidx] += dobj->id();
	turnOn( dobj->id(), true, true );
    }
}


void uiVisPartServer::removeObject( visBase::DataObject* dobj, int sceneid )
{
    if ( !dobj )
	return;

    removeObject( dobj->id(), sceneid );
    objectaddedremoved.trigger();
}


NotifierAccess& uiVisPartServer::removeAllNotifier()
{
    return visBase::DM().removeallnotify;
}


MultiID uiVisPartServer::getMultiID( int id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) return so->getMultiID();

    return MultiID(-1);
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
    if ( so && so->getScene() )
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
	if ( childids.indexOf(visid) > -1 )
	    return sceneids[idx];
    }

    return -1;
}


const ZDomain::Info* uiVisPartServer::zDomainInfo( int sceneid ) const
{
    const visSurvey::Scene* scene = getScene( sceneid );
    return scene ? &scene->zDomainInfo() : 0;
}


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


CubeSampling uiVisPartServer::getCubeSampling( int id, int attribid ) const
{
    CubeSampling res;
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) res = so->getCubeSampling( attribid );
    return res;
}


DataPack::ID uiVisPartServer::getDataPackID( int id, int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getDataPackID( attrib ) : -1;
}


DataPackMgr::ID	uiVisPartServer::getDataPackMgrID( int id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getDataPackMgrID() : -1;
}


bool uiVisPartServer::setDataPackID( int id, int attrib, DataPack::ID dpid )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so )
	return false;

    MouseCursorChanger cursorlock( MouseCursor::Wait );
    const bool res = so->setDataPackID( attrib, dpid, 0 );

    if ( res && multirgeditwin_ && id == mapperrgeditordisplayid_ )
	multirgeditwin_->setDataPackID( attrib, dpid );

    return res;
}


const Attrib::DataCubes* uiVisPartServer::getCachedData(
						    int id, int attrib ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getCacheVolume( attrib ) : 0;
}


bool uiVisPartServer::setCubeData( int id, int attrib, 
				   const Attrib::DataCubes* attribdata )
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
    if ( so )
    {
	if ( isAttribEnabled(id,attrib) )
	    so->selectTexture( attrib, textureidx );
    }
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
    if ( so ) so->getRandomPos( dtps, 0 );
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
    if ( so ) so->setRandomPosData( attrib, dtps, 0 );
}


void uiVisPartServer::createAndDispDataPack( int id, int attrib,
					     const DataPointSet* dtps )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) so->createAndDispDataPack( attrib, dtps, 0 );
}


void uiVisPartServer::getDataTraceBids( int id, TypeSet<BinID>& bids ) const
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so )
	so->getDataTraceBids( bids );
}
	

Interval<float> uiVisPartServer::getDataTraceRange( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getDataTraceRange() : Interval<float>(0,0);
}


void uiVisPartServer::setTraceData( int id, int attrib, SeisTrcBuf& data )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so )
	so->setTraceData( attrib, data, 0 );
    else
	data.deepErase();
}


Coord3 uiVisPartServer::getMousePos(bool xyt) const
{ return xyt ? xytmousepos_ : inlcrlmousepos_; }


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
    if ( so->getScene() )
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


bool uiVisPartServer:: canHandleColTabSeqTrans( int id, int attrib ) const
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
    if ( so->getScene() )
	so->getScene()->getSceneColTab()->setColTabSequence( seq );

    if ( multirgeditwin_ && id == mapperrgeditordisplayid_ )
	multirgeditwin_->setColTabSeq( attrib, seq );
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


const Attrib::SelSpec* uiVisPartServer::getSelSpec( int id, int attrib ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so ? so->getSelSpec( attrib ) : 0;
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


void uiVisPartServer::enableAttrib( int id, int attrib, bool yn )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( so ) so->enableAttrib( attrib, yn );
}


bool uiVisPartServer::deleteAllObjects()
{
    mpetools_->deleteVisObjects();

    while ( scenes_.size() )
	removeScene( scenes_[0]->id() );

    scenes_.erase();
    nrsceneschange_.trigger();
    return visBase::DM().removeAll();
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


void uiVisPartServer::setWorkMode( uiVisPartServer::WorkMode wm, 
	bool notify )
{
    if ( wm==workmode_ ) return;
    workmode_ = wm;
    viewmode_ = ( workmode_ == uiVisPartServer::View ) 
	? true : false;
    updateDraggers();
    if ( notify )
    {
	eventmutex_.lock();
	sendEvent(evViewModeChange());
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
	scene->getPolySelection()->setSelectionType(
		    (visBase::PolygonSelection::SelectionType) seltype_ );
    }

    selectionmode_ = mode;
    selectionmodechange.trigger();
}


void uiVisPartServer::turnSelectionModeOn( bool yn )
{
    seltype_ = !yn ? (int) visBase::PolygonSelection::Off
		   : (int) visBase::PolygonSelection::Rectangle; // Dummy

    setSelectionMode( selectionmode_ );
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
    uiSurvTopBotImageDlg dlg( appserv().parent(), getScene(sceneid) );
    dlg.go();
}


void uiVisPartServer::updateDraggers()
{
    const TypeSet<int>& selected = visBase::DM().selMan().selected();

    for ( int sceneidx=0; sceneidx<scenes_.size(); sceneidx++ )
    {
	visSurvey::Scene* scene = scenes_[sceneidx];

	for ( int objidx=0; objidx<scene->size(); objidx++ )
	{
	    visBase::DataObject* obj = scene->getObject( objidx );
	    bool isdraggeron = selected.indexOf(obj->id())!=-1 && 
		(workmode_ == uiVisPartServer::Interactive); 

	    mDynamicCastGet(visSurvey::SurveyObject*,so,obj)
	    if ( so ) so->showManipulator(isdraggeron && !so->isLocked() );
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
					 TaskRunner* tr )
{
    visSurvey::Scene* scene = getScene( sceneid );
    if ( zat && scene )
	scene->setZAxisTransform( zat, tr );
}


ZAxisTransform* uiVisPartServer::getZAxisTransform( int sceneid )
{
    visSurvey::Scene* scene = getScene( sceneid );
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


// Headon light-related
float uiVisPartServer::sendGetHeadOnIntensityEvent( int sceneid )
{
    eventmutex_.lock();
    eventobjid_ = sceneid;
    return sendEvent( evGetHeadOnIntensity() );
}


void uiVisPartServer::sendSetHeadOnIntensityEvent( int sceneid, float val )
{
    eventmutex_.lock();
    eventobjid_ = sceneid;
    sendEvent( evSetHeadOnIntensity() );
}


float uiVisPartServer::getHeadOnIntensity() const
{ return dirlightdlg_ ? dirlightdlg_->getHeadOnIntensity() : 0; }

void uiVisPartServer::setHeadOnIntensity( float val )
{ if ( dirlightdlg_ ) dirlightdlg_->setHeadOnIntensity( val ); }


void uiVisPartServer::vwAll( CallBacker* )
{ eventmutex_.lock(); sendEvent( evViewAll() ); }

void uiVisPartServer::toHome( CallBacker* )
{ eventmutex_.lock(); sendEvent( evToHomePos() ); }


class uiWorkAreaDlg : public uiDialog
{
public:
uiWorkAreaDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Set work volume","","0.3.4"))
{
    selfld_ = new uiSelSubvol( this, false );
    fullbut_ = new uiToolButton( this, "exttofullsurv",
	    			"Set ranges to full survey",
				 mCB(this,uiWorkAreaDlg,fullPush) );
    fullbut_->attach( rightOf, selfld_ );
}

void fullPush( CallBacker* )
{
    selfld_->setSampling( SI().sampling(false) );
}

bool acceptOK( CallBacker* )
{
    CubeSampling cs = selfld_->getSampling();
    const_cast<SurveyInfo&>(SI()).setWorkRange( cs );
    return true;
}

    uiSelSubvol*	selfld_;
    uiToolButton*	fullbut_;
};


bool uiVisPartServer::setWorkingArea()
{
    uiWorkAreaDlg dlg( appserv().parent() );
    if ( !dlg.go() ) return false;

    TypeSet<int> sceneids;
    getChildIds( -1, sceneids );

    for ( int ids=0; ids<sceneids.size(); ids++ )
    {
	int sceneid = sceneids[ids];
	visBase::DataObject* obj = visBase::DM().getObject( sceneid );
	mDynamicCastGet(visSurvey::Scene*,scene,obj)
	if ( scene && scene->zDomainInfo().def_==ZDomain::SI() )
	    scene->setCubeSampling( SI().sampling(true) );
    }

    return true;
}


bool uiVisPartServer::usePar( const IOPar& par )
{
    BufferString res;
    if ( par.get( sKeyWorkArea(), res ) )
    {
	FileMultiString fms(res);
	CubeSampling cs;
	HorSampling& hs = cs.hrg; StepInterval<float>& zrg = cs.zrg;
	hs.start.inl = toInt(fms[0]); hs.stop.inl = toInt(fms[1]);
	hs.start.crl = toInt(fms[2]); hs.stop.crl = toInt(fms[3]);
	zrg.start = toFloat(fms[4]); zrg.stop = toFloat(fms[5]);
	const_cast<SurveyInfo&>(SI()).setRange( cs, true );
    }

    int sceneres = visBase::DM().usePar( par );
    if ( sceneres==-1 )
	return false;

    if ( sceneres==0 )
    {
	const char* errmsg = visBase::DM().errMsg();
	if ( errmsg )
	    uiMSG().errorWithDetails( errmsg );
    }

    TypeSet<int> sceneids;
    visBase::DM().getIds( typeid(visSurvey::Scene), sceneids );

    TypeSet<int> hasconnections;
    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	visSurvey::Scene* newscene =
		(visSurvey::Scene*) visBase::DM().getObject( sceneids[idx] );
	addScene( newscene );

	float appvel;
	if ( par.get(sKeyAppVel(),appvel) )
	    newscene->setZStretch( appvel/1000 );

	TypeSet<int> children;
	getChildIds( newscene->id(), children );

	for ( int idy=0; idy<children.size(); idy++ )
	{
	    int childid = children[idy];
	    if ( hasconnections.indexOf( childid ) >= 0 ) continue;

	    setUpConnections( childid );
	    hasconnections += childid;

	    turnOn( childid, isOn(childid) );
	}
    }

    mpetools_->initFromDisplay();

    return true;
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
	    visBase::DataObject* dobj = visBase::DM().getObject( childid );
	    mDynamicCastGet(visSurvey::SurveyObject*,so,dobj);
	    if ( !so ) continue;
	    for ( int attrib=so->nrAttribs()-1; attrib>=0; attrib-- )
		calculateAttrib( childid, attrib, false, true );
	}
    }
}


void uiVisPartServer::fillPar( IOPar& par ) const
{
    TypeSet<int> storids;

    for ( int idx=0; idx<scenes_.size(); idx++ )
	storids += scenes_[idx]->id();

    const CubeSampling& cs = SI().sampling( true );
    FileMultiString fms;
    fms += cs.hrg.start.inl; fms += cs.hrg.stop.inl; fms += cs.hrg.start.crl;
    fms += cs.hrg.stop.crl; fms += cs.zrg.start; fms += cs.zrg.stop;
    par.set( sKeyWorkArea(), fms );

    visBase::DM().fillPar( par, storids );
}


void uiVisPartServer::turnOn( int id, bool yn, bool doclean )
{
    if ( !vismgr_->allowTurnOn(id,doclean) )
	yn = false;

    visBase::DataObject* dobj = visBase::DM().getObject( id );
    if ( !dobj ) return;

    mDynamicCastGet(visBase::VisualObject*,vo,dobj)
    if ( !vo || vo->isOn()==yn )
	return;

    vo->turnOn( yn );

    TypeSet<int> sceneids;
    getChildIds( -1, sceneids );
    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	visSurvey::Scene* scene =
		(visSurvey::Scene*) visBase::DM().getObject( sceneids[idx] );
	if ( scene ) scene->objectMoved(dobj);
    }
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


bool uiVisPartServer::sendShowSetupDlgEvent()
{
    eventmutex_.lock();
    return sendEvent( evShowSetupDlg() );
}


bool uiVisPartServer::sendPickingStatusChangeEvent()
{
   eventmutex_.lock();
   return sendEvent( evPickingStatusChange() );
}


bool uiVisPartServer::sendDisableSelTrackerEvent()
{
   eventmutex_.lock();
   return sendEvent( evDisableSelTracker() );
}


void uiVisPartServer::trackInVolume()
{ mpetools_->trackInVolume(); }


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


void uiVisPartServer::turnQCPlaneOff()
{
    if ( isViewMode() )
	return;

    if ( !mpetools_ )
	return;

    mpetools_->turnQCPlaneOff();
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


void uiVisPartServer::loadPostponedData() const
{
    eventmutex_.lock();
    sendEvent( evLoadPostponedData() );
}


void uiVisPartServer::postponedLoadingData() const
{
    eventmutex_.lock();
    sendEvent( evPostponedLoadingData() );
}


void uiVisPartServer::toggleBlockDataLoad() const
{
    eventmutex_.lock();
    sendEvent( evToggleBlockDataLoad() );
}


#define mGetScene( prepostfix ) \
prepostfix visSurvey::Scene* \
uiVisPartServer::getScene( int sceneid ) prepostfix \
{ \
    for ( int idx=0; idx<scenes_.size(); idx++ ) \
    { \
	if ( scenes_[idx]->id()==sceneid ) \
	{ \
	    return scenes_[idx]; \
	} \
    } \
 \
    return 0; \
}

mGetScene();
mGetScene( const ); 


void uiVisPartServer::removeObject( int id, int sceneid )
{
    removeConnections( id );

    visSurvey::Scene* scene = getScene( sceneid );
    if ( !scene ) return;

    const int idx = scene->getFirstIdx( id );
    if ( idx!=-1 ) 
    {
	scene->removeObject( idx );
	objectaddedremoved.trigger();
    }
}


void uiVisPartServer::removeSelection()
{
    TypeSet<int> sceneids;
    getChildIds( -1, sceneids );
    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	const Selector<Coord3>* sel = getCoordSelector( sceneids[idx] );
	if ( sel && sel->isOK() )
	{
	    int selobjectid = getSelObjectId();
	    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(selobjectid));
	    if ( !so ) continue;
	    if ( so->canRemoveSelection() )
	    {
		BufferString msg = "Are you sure you want to \n"
		    "remove selected part of ";
		msg += getObjectName( selobjectid );
		msg += "?";
		
		if ( uiMSG().askContinue(msg.buf()) )
		{
		    uiTaskRunner taskrunner( appserv().parent() );
		    so->removeSelection( *sel, &taskrunner );
		}
	    }
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
    return sendEvent( evGetNewData() );
}


bool uiVisPartServer::hasMaterial( int id ) const
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    return so && so->allowMaterialEdit();
}


bool uiVisPartServer::setMaterial( int id )
{
    mDynamicCastGet(visBase::VisualObject*,vo,getObject(id))
    if ( !hasMaterial(id) || !vo ) return false;

    uiPropertiesDlg* dlg = new uiPropertiesDlg( appserv().parent(),
	    dynamic_cast<visSurvey::SurveyObject*>(vo) );
    dlg->setDeleteOnClose( true );
    dlg->go();
    
    return true;
}


bool uiVisPartServer::writeSceneToFile( int id, const char* dlgtitle ) const
{
    const char* extension = visBase::DataObject::doOsg()
	? "*.osg"
	: "*.iv";
    uiFileDialog filedlg( appserv().parent(), false, GetPersonalDir(),
	    		extension, dlgtitle );
    if ( filedlg.go() )
    {
	visBase::DataObject* obj = visBase::DM().getObject( id );
	if ( !obj ) return false;
	return obj->serialize( filedlg.fileName() );
    }

    return false;
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


void uiVisPartServer::selectObjCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,sel,cb);
    visBase::DataObject* dobj = visBase::DM().getObject( sel );
    mDynamicCastGet(visSurvey::SurveyObject*,so,dobj);
    if ( ( workmode_ == uiVisPartServer::Interactive ) && so )
	so->showManipulator( !so->isLocked() );

    selattrib_ = -1;

    eventmutex_.lock();
    eventobjid_ = sel;
    sendEvent( evSelection() );

    eventmutex_.lock();
    sendEvent( evPickingStatusChange() );
}


void uiVisPartServer::deselectObjCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,oldsel,cb);
    visBase::DataObject* dobj = visBase::DM().getObject( oldsel );
    mDynamicCastGet(visSurvey::SurveyObject*,so,dobj)
    if ( so )
    {    
	if ( so->isManipulated() )
	{
	    if ( isLocked(oldsel) )
		resetManipulation( oldsel );
	    else
	    {
		so->acceptManipulation();
		for ( int attrib=so->nrAttribs()-1; attrib>=0; attrib-- )
		    calculateAttrib( oldsel, attrib, false );
	    }
	}

	if ( workmode_ == uiVisPartServer::Interactive )
	    so->showManipulator(false);
    }

    eventmutex_.lock();
    eventobjid_ = oldsel;
    selattrib_ = -1;
    sendEvent( evDeSelection() );

    eventmutex_.lock();
    sendEvent( evPickingStatusChange() );
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


void uiVisPartServer::setMarkerPos( const Coord3& worldpos, int dontsetscene )
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
	scenes_[idx]->setMarkerPos( worldpos, dontsetscene );
}


void uiVisPartServer::mouseMoveCB( CallBacker* cb )
{
    mDynamicCastGet(visSurvey::Scene*,scene,cb)
    if ( !scene ) return;

    Coord3 worldpos = xytmousepos_ = scene->getMousePos(true);
    if ( xytmousepos_.isDefined() && scene->getZAxisTransform() )
	worldpos.z = scene->getZAxisTransform()->transformBack( xytmousepos_ );

    setMarkerPos( worldpos, scene->id() );

    MouseCursorExchange::Info info( worldpos );
    mousecursorexchange_->notifier.trigger( info, this );

    eventmutex_.lock();
    inlcrlmousepos_ = scene->getMousePos(false);
    mouseposval_ = scene->getMousePosValue();
    mouseposstr_ = scene->getMousePosString();
    zfactor_ = scene->zDomainUserFactor();
    sendEvent( evMouseMove() );
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

    MenuItemHolder* baseitem = menu->findItem( "&Display" );
    if ( !baseitem ) baseitem = menu;
    mAddMenuItemCond( baseitem, &changematerialmnuitem_, true, false, 
		      so->allowMaterialEdit() );

    resmnuitem_.id = -1;
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


void uiVisPartServer::showMPEToolbar( bool yn )
{
    if ( yn )
    {
	updateMPEToolbar();
	if ( !getTrackTB()->isVisible() )
	    getTrackTB()->display( true );
    }
    else if ( getTrackTB()->isVisible() )
	getTrackTB()->display( false );
}


void uiVisPartServer::updateMPEToolbar()
{
    mpetools_->validateSeedConMode();
    mpetools_->updateButtonSensitivity();
}


void uiVisPartServer::updateSeedConnectMode()
{ mpetools_->updateSeedModeSel(); }


void uiVisPartServer::introduceMPEDisplay()
{ mpetools_->introduceMPEDisplay(); }


uiToolBar* uiVisPartServer::getTrackTB() const
{ return mpetools_->getToolBar(); }


void uiVisPartServer::initMPEStuff()
{
    TypeSet<int> emobjids;
    findObject( typeid(visSurvey::EMObjectDisplay), emobjids );
    for ( int idx=0; idx<emobjids.size(); idx++ )
    {
	mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,
			getObject(emobjids[idx]))
	emod->updateFromMPE();
    }
    
    mpetools_->initFromDisplay();
}


void uiVisPartServer::fireLoadAttribDataInMPEServ()
{ 
    eventmutex_.lock();
    sendEvent( evLoadAttribDataInMPEServ() ); 
}


void uiVisPartServer::fireFromMPEManStoreEMObject()
{ 
    eventmutex_.lock();
    sendEvent( evFromMPEManStoreEMObject() ); 
}


void uiVisPartServer::updateOldActiVolInuiMPEMan()
{ mpetools_->updateOldActiveVol(); }


void uiVisPartServer::restoreActiveVolInuiMPEMan()
{ mpetools_->restoreActiveVolume(); }


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


void uiVisPartServer::displayMapperRangeEditForAttrbs( int displayid )
{
    MouseCursorChanger cursorlock( MouseCursor::Wait );
    mapperrgeditordisplayid_ = displayid;
    if ( multirgeditwin_ )
    {
	multirgeditwin_->close();
	delete multirgeditwin_;
	multirgeditwin_ = 0;
    }

    const DataPackMgr::ID dpmid = getDataPackMgrID( displayid );
    if ( dpmid < 1 )
    {
	uiMSG().error( "Cannot display histograms for this type of data" );
	return;
    }

    multirgeditwin_ = new uiMultiMapperRangeEditWin( 0, getNrAttribs(displayid),
						     dpmid );
    multirgeditwin_->setDeleteOnClose( false );
    multirgeditwin_->rangeChange.notify( 
	    mCB(this,uiVisPartServer,mapperRangeEditChanged) );

    for ( int idx=0; idx<getNrAttribs(displayid); idx++ )
    {
	const DataPack::ID dpid = getDataPackID( displayid, idx );
	if ( dpid < 1 )
	    continue;

	multirgeditwin_->setDataPackID( idx, dpid );
	const ColTab::MapperSetup* ms = getColTabMapperSetup( displayid, idx );
	if ( ms ) multirgeditwin_->setColTabMapperSetup( idx, *ms );

	const ColTab::Sequence* ctseq = getColTabSequence( displayid, idx );
	if ( ctseq ) multirgeditwin_->setColTabSeq( idx, *ctseq );
    }

    multirgeditwin_->go();
}


void uiVisPartServer::lock( int id, bool yn )
{
    mDynamicCastGet(visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return;

    const TypeSet<int>& selected = visBase::DM().selMan().selected();
    so->lock( yn );

    const bool isdraggeron = selected.indexOf(id)!=-1 && 
	( workmode_ == uiVisPartServer::Interactive );
    so->showManipulator( isdraggeron && !so->isLocked() );
}


bool uiVisPartServer::isLocked( int id ) const
{
    mDynamicCastGet(const visSurvey::SurveyObject*,so,getObject(id));
    if ( !so ) return false;

    return so->isLocked();
}


const Color& uiVisPartServer::getSceneAnnotCol( int sceneidx )
{
    return scenes_[ sceneidx ]->getAnnotColor();
}


void uiVisPartServer::displaySceneColorbar( bool yn )
{
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	scenes_[idx]->getSceneColTab()->setLegendColor(
		scenes_[idx]->getAnnotColor() );
	scenes_[idx]->getSceneColTab()->turnOn( yn );
    }
}


bool uiVisPartServer::sceneColorbarDisplayed() 
{
    return scenes_.size()>0 && scenes_[0]
	? scenes_[0]->getSceneColTab()->isOn() : false;
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

