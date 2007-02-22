/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Jan 2005
 RCS:           $Id: uivisemobj.cc,v 1.53 2007-02-22 12:49:56 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uivisemobj.h"

#include "attribsel.h"
#include "emhistory.h"
#include "emhorizon.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emobject.h"
#include "emsurfaceiodata.h"
#include "survinfo.h"
#include "uiexecutor.h"
#include "uicursor.h"
#include "uigeninputdlg.h"
#include "uimenu.h"
#include "uimpe.h"
#include "uimsg.h"
#include "uimenuhandler.h"
#include "uiseedpropdlg.h"
#include "uivispartserv.h"
#include "visdataman.h"
#include "vishorizondisplay.h"
#include "vishorizon2ddisplay.h"
#include "vismpeeditor.h"
#include "vissurvobj.h"

const char* uiVisEMObject::trackingmenutxt = "Tracking";


uiVisEMObject::uiVisEMObject( uiParent* uip, int newid, uiVisPartServer* vps )
    : displayid(newid)
    , visserv(vps)
    , uiparent(uip)
    , nodemenu( *new uiMenuHandler(uip,-1) )
    , interactionlinemenu( *new uiMenuHandler(uip,-1) )
    , edgelinemenu( *new uiMenuHandler(uip,-1) )
    , showedtexture(true)
{
    nodemenu.ref();
    interactionlinemenu.ref();
    edgelinemenu.ref();

    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod ) return;

    mDynamicCastGet(const visSurvey::HorizonDisplay*,hordisp,emod);

    uiCursorChanger cursorchanger( uiCursor::Wait );

    const MultiID mid = emod->getMultiID();
    EM::ObjectID emid = EM::EMM().getObjectID( mid );
    if ( !EM::EMM().getObject(emid) )
    {
	Executor* exec = 0;
	EM::SurfaceIOData sd;
	if ( !EM::EMM().getSurfaceData(mid,sd) )
	{
	    EM::SurfaceIODataSelection sel( sd );
	    sel.setDefault();

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
		const StepInterval<int> rowrg = hordisp->displayedRowRange();
		const StepInterval<int> colrg = hordisp->displayedColRange();
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
	else
	    exec = EM::EMM().objectLoader( mid );

	if ( exec )
	{
	    emid = EM::EMM().getObjectID( mid );
	    EM::EMObject* emobject = EM::EMM().getObject( emid );
	    emobject->ref();
	    uiExecutor dlg( uiparent, *exec );
	    if ( !dlg.go() )
	    {
		emid = -1;
		emobject->unRef();
		emod->unRef();
		return;
	    }

	    delete exec;
	    emobject->unRefNoDelete();
	}
    }

    if ( !emod->setEMObject(emid) ) { emod->unRef(); return; }

    if ( hordisp && hordisp->usesTexture() )
    {
	for ( int idx=0; idx<emod->nrAttribs(); idx++ )
	{
	    if ( hordisp->getSelSpec(idx)->id()==Attrib::SelSpec::cNoAttrib() )
		setDepthAsAttrib( idx );
	}
    }

    setUpConnections();
}


#define mRefUnrefRet { emod->ref(); emod->unRef(); return; }

uiVisEMObject::uiVisEMObject( uiParent* uip, const EM::ObjectID& emid,
			      int sceneid, uiVisPartServer* vps )
    : displayid(-1)
    , visserv( vps )
    , uiparent( uip )
    , nodemenu( *new uiMenuHandler(uip,-1) )
    , interactionlinemenu( *new uiMenuHandler(uip,-1) )
    , edgelinemenu( *new uiMenuHandler(uip,-1) )
    , showedtexture(true)
{
    nodemenu.ref();
    interactionlinemenu.ref();
    edgelinemenu.ref();

    const EM::EMObject* emobj = EM::EMM().getObject(emid);
    visSurvey::EMObjectDisplay* emod = 0;
    mDynamicCastGet( const EM::Horizon*, hor, emobj );
    if ( hor )
	emod = visSurvey::HorizonDisplay::create();
    mDynamicCastGet( const EM::Horizon2D*, hor2d, emobj );
    if ( hor2d )
	emod = visSurvey::Horizon2DDisplay::create();

    mDynamicCastGet(visSurvey::Scene*,scene,visBase::DM().getObject(sceneid))
    emod->setDisplayTransformation( scene->getUTM2DisplayTransform() );

    uiCursorChanger cursorchanger(uiCursor::Wait);
    if ( !emod->setEMObject(emid) ) mRefUnrefRet

    visserv->addObject( emod, sceneid, true );
    displayid = emod->id();
    setDepthAsAttrib( 0 );

    setUpConnections();
}


uiVisEMObject::~uiVisEMObject()
{
    MenuHandler* menu = visserv->getMenuHandler();
    if ( menu )
    {
	menu->createnotifier.remove( mCB(this,uiVisEMObject,createMenuCB) );
	menu->handlenotifier.remove( mCB(this,uiVisEMObject,handleMenuCB) );
    }

    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( emod && emod->getEditor() )
    {
	emod->getEditor()->noderightclick.remove(
		mCB(this,uiVisEMObject,nodeRightClick) );
	emod->getEditor()->interactionlinerightclick.remove(
		mCB(this,uiVisEMObject,interactionLineRightClick) );
    }

    nodemenu.createnotifier.remove( mCB(this,uiVisEMObject,createNodeMenuCB) );
    nodemenu.handlenotifier.remove( mCB(this,uiVisEMObject,handleNodeMenuCB) );
    nodemenu.unRef();

    interactionlinemenu.createnotifier.remove(
	    mCB(this,uiVisEMObject,createInteractionLineMenuCB) );
    interactionlinemenu.handlenotifier.remove(
	    mCB(this,uiVisEMObject,handleInteractionLineMenuCB) );
    interactionlinemenu.unRef();

    edgelinemenu.createnotifier.remove(
	    mCB(this,uiVisEMObject,createEdgeLineMenuCB) );
    edgelinemenu.handlenotifier.remove(
	    mCB(this,uiVisEMObject,handleEdgeLineMenuCB) );
    edgelinemenu.unRef();
}


bool uiVisEMObject::isOK() const
{
    return getDisplay();
}


void uiVisEMObject::setUpConnections()
{
    singlecolmnuitem.text = "Use single color";
    trackmenuitem.text = uiVisEMObject::trackingmenutxt;
    seedsmenuitem.text = "Seeds";
    showseedsmnuitem.text = "Show";
    seedpropmnuitem.text = "Properties ...";
    lockseedsmnuitem.text = "Lock";
    wireframemnuitem.text = "Wireframe";
    editmnuitem.text = "Edit";
    shiftmnuitem.text = "Shift ...";
    removesectionmnuitem.text ="Remove section";
    makepermnodemnuitem.text = "Make control permanent";
    removecontrolnodemnuitem.text = "Remove control";
    showonlyatsectionsmnuitem.text = "Display only at sections";
    changesectionnamemnuitem.text = "Change section's name";

    MenuHandler* menu = visserv->getMenuHandler();
    menu->createnotifier.notify( mCB(this,uiVisEMObject,createMenuCB) );
    menu->handlenotifier.notify( mCB(this,uiVisEMObject,handleMenuCB) );
    nodemenu.createnotifier.notify( mCB(this,uiVisEMObject,createNodeMenuCB) );
    nodemenu.handlenotifier.notify( mCB(this,uiVisEMObject,handleNodeMenuCB) );
    interactionlinemenu.createnotifier.notify(
	    mCB(this,uiVisEMObject,createInteractionLineMenuCB) );
    interactionlinemenu.handlenotifier.notify(
	    mCB(this,uiVisEMObject,handleInteractionLineMenuCB) );
    edgelinemenu.createnotifier.notify(
	    mCB(this,uiVisEMObject,createEdgeLineMenuCB) );
    edgelinemenu.handlenotifier.notify(
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

	//interactionlinemenu.setID( emod->getEditor()->lineID() );
	//edgelinemenu.setID( emod->getEditor()->lineID() );
    }
}

const char* uiVisEMObject::getObjectType( int id )
{
    mDynamicCastGet(visSurvey::EMObjectDisplay*,obj,visBase::DM().getObject(id))
    return obj ? EM::EMM().objectType( obj->getMultiID() ) : 0;
}


void uiVisEMObject::setDepthAsAttrib( int attrib )
{
    uiCursorChanger cursorchanger( uiCursor::Wait );
    mDynamicCastGet( visSurvey::HorizonDisplay*, hordisp, getDisplay() );
    if ( hordisp ) hordisp->setDepthAsAttrib( attrib );
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
    if ( menu->menuID()!=displayid )
	return;

    visSurvey::EMObjectDisplay* emod = getDisplay();

    const EM::ObjectID emid = emod->getObjectID();
    const EM::EMObject* emobj = EM::EMM().getObject(emid);
    const EM::SectionID sid = emod->getSectionID(menu->getPath());

    mDynamicCastGet( const visSurvey::HorizonDisplay*, hordisp, getDisplay() );

    mAddMenuItem( menu, &singlecolmnuitem, !emod->getOnlyAtSectionsDisplay(),
	  	  !hordisp || (hordisp&&!hordisp->usesTexture()) );
    mAddMenuItem( menu, &showonlyatsectionsmnuitem, true,
	          emod->getOnlyAtSectionsDisplay() );
    mAddMenuItem( menu, &changesectionnamemnuitem, 
	          emobj->canSetSectionName() && sid!=-1, false );
    const bool enabmenu =
	!strcmp(getObjectType(displayid),EM::Horizon::typeStr())
	&& !visserv->isLocked(displayid);
    mAddMenuItem( menu, &shiftmnuitem, enabmenu, false );

    mAddMenuItem( &trackmenuitem, &editmnuitem, enabmenu,
	    	  emod->isEditingEnabled() );
    if ( hordisp )
	mAddMenuItem( &trackmenuitem, &wireframemnuitem, true,
		      hordisp->usesWireframe() );
   
    const TypeSet<EM::PosID>* seeds =
			      emobj->getPosAttribList(EM::EMObject::sSeedNode);
    showseedsmnuitem.text = emod->showsPosAttrib(EM::EMObject::sSeedNode) ?
			    "Hide" : "Show" ;	
    mAddMenuItem( &seedsmenuitem, &showseedsmnuitem, seeds && seeds->size(),
	    	  false );
    mAddMenuItem( &seedsmenuitem, &seedpropmnuitem, true, false );
    lockseedsmnuitem.text = emobj->isPosAttribLocked(EM::EMObject::sSeedNode) ?
			    "Unlock" : "Lock" ;	
    mAddMenuItem( &seedsmenuitem, &lockseedsmnuitem, true, false );
    mAddMenuItem( &trackmenuitem,&seedsmenuitem,seedsmenuitem.nrItems(),false );

    mAddMenuItem( menu, &trackmenuitem, trackmenuitem.nrItems(), false );

    mAddMenuItem( menu, &removesectionmnuitem, false, false );
    if ( emobj->nrSections()>1 && sid!=-1 )
	removesectionmnuitem.enabled = true;
}


void uiVisEMObject::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod || mnuid==-1 || menu->isHandled() || menu->menuID()!=displayid )
	return;

    mDynamicCastGet( visSurvey::HorizonDisplay*, hordisp, getDisplay() );
    const EM::ObjectID emid = emod->getObjectID();
    EM::EMObject* emobj = EM::EMM().getObject(emid);
    const EM::SectionID sid = emod->getSectionID(menu->getPath());

    if ( mnuid==singlecolmnuitem.id )
    {
	if ( hordisp ) hordisp->useTexture( !hordisp->usesTexture(), true );
	visserv->triggerTreeUpdate();
	menu->setIsHandled(true);
    }
    else if ( mnuid==showonlyatsectionsmnuitem.id )
    {
	const bool turnon = !emod->getOnlyAtSectionsDisplay();
	setOnlyAtSectionsDisplay( turnon );
	menu->setIsHandled(true);
    }
    else if ( mnuid==changesectionnamemnuitem.id )
    {
	StringInpSpec* spec = new StringInpSpec( emobj->sectionName(sid) );
	uiGenInputDlg dlg(uiparent,"Change section-name", "Name", spec);
	while ( dlg.go() )
	{
	    if ( emobj->setSectionName(sid,dlg.text(), true ) )
	    {
		EM::History& history = EM::EMM().history();
		const int currentevent = history.currentEventNr();
		history.setLevel(currentevent,mEMHistoryUserInteractionLevel);
		break;
	    }
	}

	menu->setIsHandled(true);
    }
    else if ( mnuid==wireframemnuitem.id )
    {
	menu->setIsHandled(true);
	if ( hordisp ) hordisp->useWireframe( !hordisp->usesWireframe() );
    }
    else if ( mnuid==showseedsmnuitem.id )
    {
	menu->setIsHandled(true);
	emod->showPosAttrib( EM::EMObject::sSeedNode,
			     !emod->showsPosAttrib(EM::EMObject::sSeedNode) );
    }
    else if ( mnuid==seedpropmnuitem.id )
    {
	uiSeedPropDlg dlg( uiparent, emobj );
	dlg.go();
	menu->setIsHandled(true);
    }
    else if ( mnuid==lockseedsmnuitem.id )
    {
	emobj->lockPosAttrib( EM::EMObject::sSeedNode, 
			!emobj->isPosAttribLocked(EM::EMObject::sSeedNode) );
	menu->setIsHandled(true);
    }
    else if ( mnuid==editmnuitem.id )
    {
	bool turnon = !emod->isEditingEnabled();
	emod->enableEditing(turnon);
	if ( turnon ) connectEditor();
	menu->setIsHandled(true);
    }
    else if ( mnuid==shiftmnuitem.id )
    {
	menu->setIsHandled(true);
	if ( hordisp )
	{
	    Coord3 shift = hordisp->getTranslation();
	    BufferString lbl( "Shift " ); lbl += SI().getZUnit();
	    DataInpSpec* inpspec = new FloatInpSpec( shift.z );
	    uiGenInputDlg dlg( uiparent,"Specify horizon shift", lbl, inpspec );
	    if ( !dlg.go() ) return;

	    double newshift = dlg.getfValue();
	    if ( shift.z == newshift ) return;

	    shift.z = newshift;
	    hordisp->setTranslation( shift );
	    for ( int attrib=0; attrib<hordisp->nrAttribs(); attrib++ )
	    {
		if ( !hordisp->hasStoredAttrib( attrib ) )
		    visserv->calculateAttrib( displayid, attrib, false );
		else
		{
		    uiMSG().error( "Cannot calculate this attribute on "
			    	   "new location"
				   "\nDepth will be displayed instead" );
		    hordisp->setDepthAsAttrib( attrib );
		}
	    }

	    visserv->triggerTreeUpdate();
	}
    }
    else if ( mnuid==removesectionmnuitem.id )
    {
	if ( !emobj )
	    return;

	emobj->removeSection(sid, true );

	EM::History& history = EM::EMM().history();
	const int currentevent = history.currentEventNr();
	history.setLevel(currentevent,mEMHistoryUserInteractionLevel);
    }
}


void uiVisEMObject::setOnlyAtSectionsDisplay( bool yn )
{
    visSurvey::EMObjectDisplay* emod = getDisplay();
    mDynamicCastGet( visSurvey::HorizonDisplay*, hordisp, emod );
    if ( hordisp )
    {
	bool usetexture = false;
	if ( yn )
	    showedtexture = hordisp->usesTexture();
	else 
	    usetexture = showedtexture;

	hordisp->useTexture( usetexture );
    }

    emod->setOnlyAtSectionsDisplay( yn );
}


int uiVisEMObject::nrSurfaceData() const
{
    const visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod ) return 0;

    EM::SurfaceIOData sd;
    const char* err = EM::EMM().getSurfaceData( emod->getMultiID(), sd );
    return err && *err ? 0 : sd.valnames.size();
}



#define mMakePerm	0
#define mRemoveCtrl	1
#define mRemoveNode	2


void uiVisEMObject::interactionLineRightClick( CallBacker* )
{
    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod ) return;

    PtrMan<MPE::uiEMEditor> uimpeeditor =
	MPE::uiMPE().editorfact.create(uiparent,
				       emod->getEditor()->getMPEEditor());
    if ( !uimpeeditor ) return;

    interactionlinemenu.createnotifier.notify(
	    mCB(uimpeeditor.ptr(),MPE::uiEMEditor,createInteractionLineMenus));
    interactionlinemenu.handlenotifier.notify(
	    mCB(uimpeeditor.ptr(),MPE::uiEMEditor,handleInteractionLineMenus));

    interactionlinemenu.executeMenu(uiMenuHandler::fromScene);

    interactionlinemenu.createnotifier.remove(
	    mCB(uimpeeditor.ptr(),MPE::uiEMEditor,createInteractionLineMenus));
    interactionlinemenu.handlenotifier.remove(
	    mCB(uimpeeditor.ptr(),MPE::uiEMEditor,handleInteractionLineMenus));
}


void uiVisEMObject::nodeRightClick( CallBacker* )
{
    visSurvey::EMObjectDisplay* emod = getDisplay();
    if ( !emod ) return;

    PtrMan<MPE::uiEMEditor> uimpeeditor =
	MPE::uiMPE().editorfact.create(uiparent,
				       emod->getEditor()->getMPEEditor());
    if ( !uimpeeditor ) return;
    const EM::PosID empid = emod->getEditor()->getNodePosID(
				emod->getEditor()->getRightClickNode());
    if ( empid.objectID()==-1 )
	return;

    uimpeeditor->setActiveNode( empid );

    nodemenu.createnotifier.notify(
	    mCB(uimpeeditor.ptr(),MPE::uiEMEditor,createNodeMenus));
    nodemenu.handlenotifier.notify(
	    mCB(uimpeeditor.ptr(),MPE::uiEMEditor,handleNodeMenus));

    nodemenu.executeMenu(uiMenuHandler::fromScene);

    nodemenu.createnotifier.remove(
	    mCB(uimpeeditor.ptr(),MPE::uiEMEditor,createNodeMenus));
    nodemenu.handlenotifier.remove(
	    mCB(uimpeeditor.ptr(),MPE::uiEMEditor,handleNodeMenus));
}


void uiVisEMObject::edgeLineRightClick( CallBacker* cb )
{
    /*
    mCBCapsuleUnpack(const visSurvey::EdgeLineSetDisplay*,edgelinedisplay,cb);
    if ( !edgelinedisplay ) return;

    edgelinemenu.setID(edgelinedisplay->id());
    edgelinemenu.executeMenu(uiMenuHandler::fromScene);
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

    mAddMenuItem( menu, &makepermnodemnuitem,
	          emobj->isPosAttrib(empid,EM::EMObject::sTemporaryControlNode),
		  false );

    mAddMenuItem( menu, &removecontrolnodemnuitem,
	emobj->isPosAttrib(empid,EM::EMObject::sPermanentControlNode),
	true);
/*
    removenodenodemnuitem = emobj->isDefined(*empid)
        ? menu->addItem( new uiMenuItem("Remove node") )
	: -1;

    uiMenuItem* snapitem = new uiMenuItem("Snap after edit");
    tooglesnappingnodemnuitem = menu->addItem(snapitem);
    snapitem->setChecked(emod->getEditor()->snapAfterEdit());
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

    if ( mnuid==makepermnodemnuitem.id )
    {
	menu->setIsHandled(true);
        emobj->setPosAttrib(empid,EM::EMObject::sPermanentControlNode,true);
	emobj->setPosAttrib(empid,EM::EMObject::sTemporaryControlNode,false);
	emobj->setPosAttrib(empid,EM::EMObject::sEdgeControlNode,false);
    }
    else if ( mnuid==removecontrolnodemnuitem.id )
    {
	menu->setIsHandled(true);
        emobj->setPosAttrib(empid,EM::EMObject::sPermanentControlNode,false);
	emobj->setPosAttrib(empid,EM::EMObject::sTemporaryControlNode,false);
	emobj->setPosAttrib(empid,EM::EMObject::sEdgeControlNode,false);
    }
}


visSurvey::EMObjectDisplay* uiVisEMObject::getDisplay()
{
    mDynamicCastGet( visSurvey::EMObjectDisplay*, emod,
		     visserv->getObject(displayid));
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

