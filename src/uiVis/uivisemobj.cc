/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Jan 2005
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uivisemobj.cc,v 1.101 2012-07-30 20:57:41 cvskris Exp $";

#include "uivisemobj.h"

#include "attribsel.h"
#include "undo.h"
#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emobject.h"
#include "emioobjinfo.h"
#include "emsurfaceiodata.h"
#include "executor.h"
#include "mousecursor.h"
#include "survinfo.h"

#include "uigeninputdlg.h"
#include "uimenu.h"
#include "uimpe.h"
#include "uimsg.h"
#include "uimenuhandler.h"
#include "uiseedpropdlg.h"
#include "uitaskrunner.h"
#include "uivispartserv.h"

#include "visdataman.h"
#include "vishorizondisplay.h"
#include "vishorizon2ddisplay.h"
#include "vismarchingcubessurfacedisplay.h"
#include "vismpeeditor.h"
#include "vissurvobj.h"

const char* uiVisEMObject::trackingmenutxt()	    { return "Tracking"; }


uiVisEMObject::uiVisEMObject( uiParent* uip, int newid, uiVisPartServer* vps )
    : displayid_(newid)
    , visserv_(vps)
    , uiparent_(uip)
    , nodemenu_( *new uiMenuHandler(uip,-1) )
    , interactionlinemenu_( *new uiMenuHandler(uip,-1) )
    , edgelinemenu_( *new uiMenuHandler(uip,-1) )
    , showedtexture_(true)
{
    MouseCursorChanger cursorchanger( MouseCursor::Wait );

    nodemenu_.ref();
    interactionlinemenu_.ref();
    edgelinemenu_.ref();

    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod ) return;

    visSurvey::Scene* scene = emod->getScene();
    mDynamicCastGet(const visSurvey::HorizonDisplay*,hordisp,emod);
    const MultiID mid = emod->getMultiID();
    EM::ObjectID emid = EM::EMM().getObjectID( mid );
    
    uiTaskRunner dlg( uiparent_ );
    if ( !EM::EMM().getObject(emid) )
    {
	Executor* exec = 0;
	EM::IOObjInfo oi( mid ); EM::SurfaceIOData sd;
	const char* rdres = oi.getSurfaceData( sd );
	if ( rdres )
	    exec = EM::EMM().objectLoader( mid );
	else
	{
	    EM::SurfaceIODataSelection sel( sd );
	    sel.setDefault();
	    sel.selvalues.erase();

	    const BufferStringSet sections = emod->displayedSections();

	    TypeSet<int> sectionidx;
	    for ( int idx=sections.size()-1; idx>=0; idx-- )
	    {
		const int idy = sel.sd.sections.indexOf( *sections[idx] );
		if ( idy!=-1 )
		    sectionidx += idy;
	    }

	    if ( sectionidx.size() )
		sel.selsections = sectionidx;

	    if ( hordisp )
	    {
		const StepInterval<int> rowrg = hordisp->geometryRowRange();
		const StepInterval<int> colrg = hordisp->geometryColRange();
		if ( rowrg.step!=-1 && colrg.step!=-1 )
		{
		    sel.rg.start.inl = rowrg.start;
		    sel.rg.start.crl = colrg.start;
		    sel.rg.stop.inl = rowrg.stop;
		    sel.rg.step.crl = colrg.step;
		    sel.rg.step.inl = rowrg.step;
		    sel.rg.stop.crl = colrg.stop;
		}
	    }

	    exec = EM::EMM().objectLoader( mid, &sel );
	}

	if ( exec )
	{
	    emid = EM::EMM().getObjectID( mid );
	    EM::EMObject* emobject = EM::EMM().getObject( emid );
	    emobject->ref();
	    if ( !dlg.execute(*exec) )
	    {
		emid = -1;
		emobject->unRef();
		if ( scene ) visserv_->removeObject( emod, scene->id() );
		delete exec;
		return;
	    }

	    delete exec;
	    emobject->unRefNoDelete();
	}
    }

    if ( !emod->setEMObject(emid,&dlg) )
    {
	if ( scene ) visserv_->removeObject( emod, scene->id() );
	return;
    }

    if ( hordisp && hordisp->usesTexture() )
    {
	for ( int idx=0; idx<emod->nrAttribs(); idx++ )
	{
	    if ( hordisp->getSelSpec(idx)->id().asInt()
		 ==Attrib::SelSpec::cNoAttrib().asInt() )
		setDepthAsAttrib( idx );
	}
    }

    setUpConnections();
}


#define mRefUnrefRet { emod->ref(); emod->unRef(); return; }

uiVisEMObject::uiVisEMObject( uiParent* uip, const EM::ObjectID& emid,
			      int sceneid, uiVisPartServer* vps )
    : displayid_(-1)
    , visserv_( vps )
    , uiparent_( uip )
    , nodemenu_( *new uiMenuHandler(uip,-1) )
    , interactionlinemenu_( *new uiMenuHandler(uip,-1) )
    , edgelinemenu_( *new uiMenuHandler(uip,-1) )
    , showedtexture_(true)
{
    nodemenu_.ref();
    interactionlinemenu_.ref();
    edgelinemenu_.ref();

    const EM::EMObject* emobj = EM::EMM().getObject(emid);
    visSurvey::EMObjectDisplay* emod = 0;
    mDynamicCastGet(const EM::Horizon3D*,hor3d,emobj);
    if ( hor3d )
	emod = visSurvey::HorizonDisplay::create();
    mDynamicCastGet(const EM::Horizon2D*,hor2d,emobj);
    if ( hor2d )
	emod = visSurvey::Horizon2DDisplay::create();

    mDynamicCastGet(visSurvey::Scene*,scene,visBase::DM().getObject(sceneid))
    if ( emod )
    {
     	emod->setDisplayTransformation( scene->getUTM2DisplayTransform() );
    	emod->setZAxisTransform( scene->getZAxisTransform(),0 );
	
    	uiTaskRunner dlg( uiparent_ );
    	if ( !emod->setEMObject(emid, &dlg ) ) mRefUnrefRet
	    
	    visserv_->addObject( emod, sceneid, true );
    	displayid_ = emod->id();
    	setDepthAsAttrib( 0 );
    }

    setUpConnections();
}


uiVisEMObject::~uiVisEMObject()
{
    MenuHandler* menu = visserv_->getMenuHandler();
    if ( menu )
    {
	menu->createnotifier.remove( mCB(this,uiVisEMObject,createMenuCB) );
	menu->handlenotifier.remove( mCB(this,uiVisEMObject,handleMenuCB) );
    }

    MenuHandler* tb = visserv_->getToolBarHandler();
    if ( tb )
    {
	tb->createnotifier.remove( mCB(this,uiVisEMObject,addToToolBarCB) );
	tb->handlenotifier.remove( mCB(this,uiVisEMObject,handleMenuCB) );
    }

    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( emod && emod->getEditor() )
    {
	emod->getEditor()->noderightclick.remove(
		mCB(this,uiVisEMObject,nodeRightClick) );
	emod->getEditor()->interactionlinerightclick.remove(
		mCB(this,uiVisEMObject,interactionLineRightClick) );
    }

    nodemenu_.createnotifier.remove( mCB(this,uiVisEMObject,createNodeMenuCB) );
    nodemenu_.handlenotifier.remove( mCB(this,uiVisEMObject,handleNodeMenuCB) );
    nodemenu_.unRef();

    interactionlinemenu_.createnotifier.remove(
	    mCB(this,uiVisEMObject,createInteractionLineMenuCB) );
    interactionlinemenu_.handlenotifier.remove(
	    mCB(this,uiVisEMObject,handleInteractionLineMenuCB) );
    interactionlinemenu_.unRef();

    edgelinemenu_.createnotifier.remove(
	    mCB(this,uiVisEMObject,createEdgeLineMenuCB) );
    edgelinemenu_.handlenotifier.remove(
	    mCB(this,uiVisEMObject,handleEdgeLineMenuCB) );
    edgelinemenu_.unRef();
}


bool uiVisEMObject::isOK() const
{
    return getDisplay();
}


void uiVisEMObject::setUpConnections()
{
    singlecolmnuitem_.text = "Use single &color";
    singlecolmnuitem_.checkable = true;
    seedsmenuitem_.text = "S&eeds";
    seedsmenuitem_.checkable = false;
    showseedsmnuitem_.text = "&Show";
    seedpropmnuitem_.text = "&Properties ...";
    lockseedsmnuitem_.text = "&Lock";
    wireframemnuitem_.text = "&Wireframe";
    wireframemnuitem_.checkable = true;
    editmnuitem_.text = "&Edit";
    editmnuitem_.checkable = true;
    removesectionmnuitem_.text ="Remove &section";
    makepermnodemnuitem_.text = "Make control &permanent";
    removecontrolnodemnuitem_.text = "Remove &control";
    changesectionnamemnuitem_.text = "Change section's &name";
    showonlyatsectionsmnuitem_.text = "&Only at sections";
    showfullmnuitem_.text = "&In full";
    showbothmnuitem_.text = "&At sections and in full";
    showonlyatsectionsmnuitem_.checkable = true;
    showfullmnuitem_.checkable = true;
    showbothmnuitem_.checkable = true;

    MenuHandler* menu = visserv_->getMenuHandler();
    menu->createnotifier.notify( mCB(this,uiVisEMObject,createMenuCB) );
    menu->handlenotifier.notify( mCB(this,uiVisEMObject,handleMenuCB) );

    MenuHandler* tbmenu = visserv_->getToolBarHandler();
    tbmenu->createnotifier.notify( mCB(this,uiVisEMObject,addToToolBarCB) );
    tbmenu->handlenotifier.notify( mCB(this,uiVisEMObject,handleMenuCB) );

    nodemenu_.createnotifier.notify( mCB(this,uiVisEMObject,createNodeMenuCB) );
    nodemenu_.handlenotifier.notify( mCB(this,uiVisEMObject,handleNodeMenuCB) );
    interactionlinemenu_.createnotifier.notify(
	    mCB(this,uiVisEMObject,createInteractionLineMenuCB) );
    interactionlinemenu_.handlenotifier.notify(
	    mCB(this,uiVisEMObject,handleInteractionLineMenuCB) );
    edgelinemenu_.createnotifier.notify(
	    mCB(this,uiVisEMObject,createEdgeLineMenuCB) );
    edgelinemenu_.handlenotifier.notify(
	    mCB(this,uiVisEMObject,handleEdgeLineMenuCB) );

    connectEditor();
}


void uiVisEMObject::connectEditor()
{
    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( emod && emod->getEditor() )
    {
	emod->getEditor()->noderightclick.notifyIfNotNotified(
		mCB(this,uiVisEMObject,nodeRightClick) );

	emod->getEditor()->interactionlinerightclick.notifyIfNotNotified(
		mCB(this,uiVisEMObject,interactionLineRightClick) );

	//interactionlinemenu_.setID( emod->getEditor()->lineID() );
	//edgelinemenu_.setID( emod->getEditor()->lineID() );
    }
}


const char* uiVisEMObject::getObjectType( int id )
{
    mDynamicCastGet(visSurvey::EMObjectDisplay*,obj,visBase::DM().getObject(id))
    if ( !obj ) return 0;

    RefMan<EM::EMObject> emobj = EM::EMM().getObject( obj->getObjectID() );
    return emobj
	? emobj->getTypeStr()
	: EM::EMM().objectType( obj->getMultiID() );
}


void uiVisEMObject::setDepthAsAttrib( int attrib )
{
    MouseCursorChanger cursorchanger( MouseCursor::Wait );
    mDynamicCastGet( visSurvey::HorizonDisplay*, hordisp, getDisplay() );
    if ( hordisp ) hordisp->setDepthAsAttrib( attrib );
    mDynamicCastGet( visSurvey::MarchingCubesDisplay*, mcdisp, getDisplay() );
    if ( mcdisp ) mcdisp->setDepthAsAttrib( attrib );
}


int uiVisEMObject::nrSections() const
{
    const visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod ) return 0;

    EM::ObjectID emid = emod->getObjectID();
    const EM::EMObject* emobj = EM::EMM().getObject(emid);
    return emobj ? emobj->nrSections() : 0;
}


EM::SectionID uiVisEMObject::getSectionID( int idx ) const
{
    const visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod ) return -1;

    EM::ObjectID emid = emod->getObjectID();
    const EM::EMObject* emobj = EM::EMM().getObject(emid);
    return emobj ? emobj->sectionID( idx ) : -1;
}


EM::SectionID uiVisEMObject::getSectionID( const TypeSet<int>* path ) const
{
    const visSurvey::EMObjectDisplay* emod = getDisplay();
    return path && emod ? emod->getSectionID( path ) : -1;
}


void uiVisEMObject::checkTrackingStatus()
{
    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod ) return;
    emod->updateFromMPE();
    connectEditor();
}


float uiVisEMObject::getShift() const
{
    mDynamicCastGet( const visSurvey::HorizonDisplay*, hordisp, getDisplay() );
    return hordisp ? hordisp->getTranslation().z : 0;
}


void uiVisEMObject::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( menu->menuID()!=displayid_ )
	return;

    visSurvey::EMObjectDisplay* emod = getDisplay();
    const EM::ObjectID emid = emod->getObjectID();
    const EM::EMObject* emobj = EM::EMM().getObject(emid);
    const EM::SectionID sid = emod->getSectionID(menu->getPath());

    mDynamicCastGet( visSurvey::HorizonDisplay*, hordisp, getDisplay() );
    mDynamicCastGet( visSurvey::Horizon2DDisplay*, hor2ddisp, getDisplay() );

    MenuItemHolder* displaymnuitem = menu->findItem( "&Display" );
    if ( !displaymnuitem ) displaymnuitem = menu;

    if ( hor2ddisp )
	mResetMenuItem( &singlecolmnuitem_ )
    else if ( displaymnuitem )
	mAddMenuItem( displaymnuitem, &singlecolmnuitem_,
		      !emod->getOnlyAtSectionsDisplay(),
		      !hordisp || (hordisp&&!hordisp->shouldUseTexture()) );

    const bool atsect = emod->getOnlyAtSectionsDisplay();
    bool infull, both = false;
    if ( hor2ddisp )
	infull = !emod->getOnlyAtSectionsDisplay();
    else
    {
	infull = !emod->getOnlyAtSectionsDisplay() &&
			!hordisp->displaysIntersectionLines();
	both = !emod->getOnlyAtSectionsDisplay() &&
			hordisp->displaysIntersectionLines();
    }

    mAddMenuItem( displaymnuitem, &showonlyatsectionsmnuitem_, true,
	    	  atsect );
    mAddMenuItem( displaymnuitem, &showfullmnuitem_, true, infull );
    if ( hordisp )
    { mAddMenuItem( displaymnuitem, &showbothmnuitem_, true, both ); }
    else
    { mResetMenuItem( &showbothmnuitem_ ); }

    //Commented out as mAddMenu is commented out below
    //visSurvey::Scene* scene = hordisp ? hordisp->getScene() : 0;
    //const bool hastransform = scene && scene->getZAxisTransform();
    //const bool enabmenu =
	//!strcmp(getObjectType(displayid_),EM::Horizon3D::typeStr())
	//&& !visserv_->isLocked(displayid_) && !hastransform;

    seedsmenuitem_.removeItems();

    MenuItem* trackmnu = menu->findItem( uiVisEMObject::trackingmenutxt() );
    if ( trackmnu )
    {
//	mAddMenuItem( trackmnu, &editmnuitem_, enabmenu,
//		      emod->isEditingEnabled() );

	const bool canhavewireframe =  hordisp && hordisp->getResolution()>0;
	const bool haswireframe = canhavewireframe && hordisp->usesWireframe();
	if ( hordisp )
	    mAddMenuItem( trackmnu, &wireframemnuitem_,
			  true, haswireframe );
	wireframemnuitem_.enabled = canhavewireframe;
	if ( !wireframemnuitem_.enabled ) wireframemnuitem_.checked = false;
       
	const TypeSet<EM::PosID>* seeds =
	    emobj->getPosAttribList(EM::EMObject::sSeedNode());
	showseedsmnuitem_.text =
	    emod->showsPosAttrib(EM::EMObject::sSeedNode()) ? "&Hide" : "S&how";
	mAddMenuItem( &seedsmenuitem_, &showseedsmnuitem_,
		      seeds && seeds->size(), false );
	mAddMenuItem( &seedsmenuitem_, &seedpropmnuitem_,
		      !visserv_->isTrackingSetupActive(), false );
	lockseedsmnuitem_.text = 
	    emobj->isPosAttribLocked(EM::EMObject::sSeedNode()) ? 
	    "Un&lock" : "&Lock" ;	
	mAddMenuItem( &seedsmenuitem_, &lockseedsmnuitem_, true, false );
	mAddMenuItem( trackmnu, &seedsmenuitem_,
		      seedsmenuitem_.nrItems(), false );
    }

#ifdef __debug__
    MenuItemHolder* toolsmnuitem = menu->findItem( "Tools" );
    if ( !toolsmnuitem ) toolsmnuitem = menu;
    mAddMenuItem( toolsmnuitem, &changesectionnamemnuitem_, 
	          emobj->canSetSectionName() && sid!=-1, false );
    mAddMenuItem( toolsmnuitem, &removesectionmnuitem_, false, false );
    if ( emobj->nrSections()>1 && sid!=-1 )
	removesectionmnuitem_.enabled = true;
#else
    mResetMenuItem( &changesectionnamemnuitem_ );
    mResetMenuItem( &removesectionmnuitem_ );
#endif
}


void uiVisEMObject::addToToolBarCB( CallBacker* cb )
{
}


void uiVisEMObject::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(MenuHandler*,menu,caller);
    mDynamicCastGet(uiMenuHandler*,uimenu,caller);
    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod || mnuid==-1 || !menu || 
	 menu->isHandled() || menu->menuID()!=displayid_ )
	return;

    mDynamicCastGet( visSurvey::HorizonDisplay*, hordisp, getDisplay() );
    const EM::ObjectID emid = emod->getObjectID();
    EM::EMObject* emobj = EM::EMM().getObject(emid);
    EM::SectionID sid = -1;
    if ( uimenu && uimenu->getPath() )
	sid = emod->getSectionID( uimenu->getPath() );

    if ( mnuid==singlecolmnuitem_.id )
    {
	if ( hordisp ) hordisp->useTexture( !hordisp->shouldUseTexture(),true );
	visserv_->triggerTreeUpdate();
	menu->setIsHandled( true );
    }
    else if ( mnuid==showonlyatsectionsmnuitem_.id )
    {
	setOnlyAtSectionsDisplay( true );
	menu->setIsHandled( true );
    }
    else if ( mnuid==showfullmnuitem_.id )
    {
	setOnlyAtSectionsDisplay( false );
	if ( hordisp ) hordisp->displayIntersectionLines( false );
	menu->setIsHandled( true );
    }
    else if ( mnuid==showbothmnuitem_.id )
    {
	setOnlyAtSectionsDisplay( false );
	if ( hordisp ) hordisp->displayIntersectionLines( true );
	menu->setIsHandled( true );
    }
    else if ( mnuid==changesectionnamemnuitem_.id )
    {
	StringInpSpec* spec = new StringInpSpec( emobj->sectionName(sid) );
	uiGenInputDlg dlg(uiparent_,"Change section-name", "Name", spec);
	while ( dlg.go() )
	{
	    if ( emobj->setSectionName(sid,dlg.text(), true ) )
	    {
		Undo& undo = EM::EMM().undo();
		const int currentevent = undo.currentEventID();
		undo.setUserInteractionEnd(currentevent);
		break;
	    }
	}

	menu->setIsHandled( true );
    }
    else if ( mnuid==wireframemnuitem_.id )
    {
	menu->setIsHandled(true);
	if ( hordisp ) hordisp->useWireframe( !hordisp->usesWireframe() );
    }
    else if ( mnuid==showseedsmnuitem_.id )
    {
	menu->setIsHandled( true );
	emod->showPosAttrib( EM::EMObject::sSeedNode(),
			     !emod->showsPosAttrib(EM::EMObject::sSeedNode()) );
    }
    else if ( mnuid==seedpropmnuitem_.id )
    {
	uiSeedPropDlg dlg( uiparent_, emobj );
	dlg.go();
	menu->setIsHandled( true );
    }
    else if ( mnuid==lockseedsmnuitem_.id )
    {
	emobj->lockPosAttrib( EM::EMObject::sSeedNode(), 
			!emobj->isPosAttribLocked(EM::EMObject::sSeedNode()) );
	menu->setIsHandled( true );
    }
    else if ( mnuid==editmnuitem_.id )
    {
	bool turnon = !emod->isEditingEnabled();
	emod->enableEditing(turnon);
	if ( turnon ) connectEditor();
	menu->setIsHandled( true );
    }
    else if ( mnuid==removesectionmnuitem_.id )
    {
	if ( !emobj )
	    return;

	emobj->removeSection(sid, true );

	Undo& undo = EM::EMM().undo();
	const int currentevent = undo.currentEventID();
	undo.setUserInteractionEnd(currentevent);
    }
}


void uiVisEMObject::setOnlyAtSectionsDisplay( bool yn )
{
    visSurvey::EMObjectDisplay* emod = getDisplay();
    mDynamicCastGet( visSurvey::HorizonDisplay*, hordisp, emod );
    if ( hordisp && hordisp->getOnlyAtSectionsDisplay()!=yn )
    {
	bool usetexture = false;
	if ( yn )
	    showedtexture_ = hordisp->shouldUseTexture();
	else 
	    usetexture = showedtexture_;

	hordisp->useTexture( usetexture );
    }

    emod->setOnlyAtSectionsDisplay( yn );
}


#define mMakePerm	0
#define mRemoveCtrl	1
#define mRemoveNode	2

void uiVisEMObject::interactionLineRightClick( CallBacker* )
{
    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod ) return;

    PtrMan<MPE::uiEMEditor> uimpeeditor =
	MPE::uiMPE().editorfact.create(uiparent_,
				       emod->getEditor()->getMPEEditor());
    if ( !uimpeeditor ) return;

    interactionlinemenu_.createnotifier.notify(
	    mCB(uimpeeditor.ptr(),MPE::uiEMEditor,createInteractionLineMenus));
    interactionlinemenu_.handlenotifier.notify(
	    mCB(uimpeeditor.ptr(),MPE::uiEMEditor,handleInteractionLineMenus));

    interactionlinemenu_.executeMenu(uiMenuHandler::fromScene());

    interactionlinemenu_.createnotifier.remove(
	    mCB(uimpeeditor.ptr(),MPE::uiEMEditor,createInteractionLineMenus));
    interactionlinemenu_.handlenotifier.remove(
	    mCB(uimpeeditor.ptr(),MPE::uiEMEditor,handleInteractionLineMenus));
}


void uiVisEMObject::nodeRightClick( CallBacker* )
{
    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod ) return;

    PtrMan<MPE::uiEMEditor> uimpeeditor =
	MPE::uiMPE().editorfact.create(uiparent_,
				       emod->getEditor()->getMPEEditor());
    if ( !uimpeeditor ) return;
    const EM::PosID empid = emod->getEditor()->getNodePosID(
				emod->getEditor()->getRightClickNode());
    if ( empid.objectID()==-1 )
	return;

    uimpeeditor->setActiveNode( empid );

    nodemenu_.createnotifier.notify(
	    mCB(uimpeeditor.ptr(),MPE::uiEMEditor,createNodeMenus));
    nodemenu_.handlenotifier.notify(
	    mCB(uimpeeditor.ptr(),MPE::uiEMEditor,handleNodeMenus));

    nodemenu_.executeMenu(uiMenuHandler::fromScene());

    nodemenu_.createnotifier.remove(
	    mCB(uimpeeditor.ptr(),MPE::uiEMEditor,createNodeMenus));
    nodemenu_.handlenotifier.remove(
	    mCB(uimpeeditor.ptr(),MPE::uiEMEditor,handleNodeMenus));
}


void uiVisEMObject::edgeLineRightClick( CallBacker* cb )
{
    /*
    mCBCapsuleUnpack(const visSurvey::EdgeLineSetDisplay*,edgelinedisplay,cb);
    if ( !edgelinedisplay ) return;

    edgelinemenu_.setID(edgelinedisplay->id());
    edgelinemenu_.executeMenu(uiMenuHandler::fromScene());
    */
}


void uiVisEMObject::createNodeMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);

    visSurvey::EMObjectDisplay* emod = getDisplay();
    const EM::PosID empid = emod->getEditor()->getNodePosID(
				emod->getEditor()->getRightClickNode());
    if ( empid.objectID()==-1 )
	return;

    EM::EMManager& em = EM::EMM();
    EM::EMObject* emobj = em.getObject(empid.objectID());

    mAddMenuItem( menu, &makepermnodemnuitem_,
	          emobj->isPosAttrib(empid,EM::EMObject::sTemporaryControlNode()),
		  false );

    mAddMenuItem( menu, &removecontrolnodemnuitem_,
	emobj->isPosAttrib(empid,EM::EMObject::sPermanentControlNode()),
	true);
/*
    removenodenodemnuitem_ = emobj->isDefined(*empid)
        ? menu->addItem( new uiMenuItem("Remove &node") )
	: -1;

    uiMenuItem* snapitem_ = new uiMenuItem("S&nap after edit");
    tooglesnappingnodemnuitem_ = menu->addItem(snapitem_);
    snapitem_->setChecked(emod->getEditor()->snapAfterEdit());
*/
}


EM::ObjectID uiVisEMObject::getObjectID() const
{
    const visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod ) return -1;

    return emod->getObjectID();
}



void uiVisEMObject::handleNodeMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(uiMenuHandler*,menu,caller)
    if ( mnuid==-1 || menu->isHandled() )
	return;

    visSurvey::EMObjectDisplay* emod = getDisplay();
    const EM::PosID empid = emod->getEditor()->getNodePosID(
				emod->getEditor()->getRightClickNode());

    EM::EMManager& em = EM::EMM();
    EM::EMObject* emobj = em.getObject(empid.objectID());

    if ( mnuid==makepermnodemnuitem_.id )
    {
	menu->setIsHandled(true);
        emobj->setPosAttrib(empid,EM::EMObject::sPermanentControlNode(),true);
	emobj->setPosAttrib(empid,EM::EMObject::sTemporaryControlNode(),false);
	emobj->setPosAttrib(empid,EM::EMObject::sEdgeControlNode(),false);
    }
    else if ( mnuid==removecontrolnodemnuitem_.id )
    {
	menu->setIsHandled(true);
        emobj->setPosAttrib(empid,EM::EMObject::sPermanentControlNode(),false);
	emobj->setPosAttrib(empid,EM::EMObject::sTemporaryControlNode(),false);
	emobj->setPosAttrib(empid,EM::EMObject::sEdgeControlNode(),false);
    }
}


visSurvey::EMObjectDisplay* uiVisEMObject::getDisplay()
{
    mDynamicCastGet( visSurvey::EMObjectDisplay*, emod,
		     visserv_->getObject(displayid_));
    return emod;
}


const visSurvey::EMObjectDisplay* uiVisEMObject::getDisplay() const
{
    return const_cast<uiVisEMObject*>(this)->getDisplay();
}


void uiVisEMObject::createEdgeLineMenuCB( CallBacker* cb )
{
}


void uiVisEMObject::handleEdgeLineMenuCB( CallBacker* cb )
{
}

