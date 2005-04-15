/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Jan 2005
 RCS:           $Id: uivisemobj.cc,v 1.9 2005-04-15 12:26:49 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uivisemobj.h"

#include "attribsel.h"
//#include "callback.h"
#include "emhistory.h"
#include "emhorizon.h"
#include "emmanager.h"
#include "emobject.h"
//#include "emsurfaceedgeline.h"
//#include "emsurfacegeometry.h"
//#include "errh.h"
//#include "ptrman.h"
#include "survinfo.h"
//#include "settings.h"
//#include "emsurfaceedgelineimpl.h"
//#include "attribsel.h"
#include "uiexecutor.h"
#include "uicursor.h"
#include "uigeninputdlg.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uivismenu.h"
#include "uivispartserv.h"
#include "visdataman.h"
#include "vissurvemobj.h"
#include "vismpeeditor.h"
//#include "vishingeline.h"

const char* uiVisEMObject::trackingmenutxt = "Tracking";


uiVisEMObject::uiVisEMObject( uiParent* uip, int id, uiVisPartServer* vps )
    : displayid(id)
    , visserv(vps)
    , uiparent(uip)
    , nodemenu( *new uiVisMenu(uip,-1) )
    , interactionlinemenu( *new uiVisMenu(uip,-1) )
    , edgelinemenu( *new uiVisMenu(uip,-1) )
{
    nodemenu.ref();
    interactionlinemenu.ref();
    edgelinemenu.ref();

    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,
	    	    visserv->getObject(displayid))
    const MultiID* mid = visserv->getMultiID( displayid );
    if ( !mid ) return;
    EM::ObjectID emid = EM::EMM().multiID2ObjectID( *mid );
    if ( !EM::EMM().getObject(emid) )
    {
	PtrMan<Executor> exec = EM::EMM().objectLoader( *mid );
	if ( exec )
	{
	    uiExecutor dlg( uiparent, *exec );
	    dlg.go();
	}
    }

    uiCursorChanger cursorchanger(uiCursor::Wait);
    if ( !emod->updateFromEM() ) { emod->unRef(); return; }

    if ( emod->getSelSpec()->id() == AttribSelSpec::noAttrib )
	setDepthAsAttrib();

    setUpConnections();
}



#define mRefUnrefRet { emod->ref(); emod->unRef(); return; }

uiVisEMObject::uiVisEMObject( uiParent* uip, const MultiID& mid, int scene,
			    uiVisPartServer* vps )
    : displayid(-1)
    , visserv( vps )
    , uiparent( uip )
    , nodemenu( *new uiVisMenu(uip,-1) )
    , interactionlinemenu( *new uiVisMenu(uip,-1) )
    , edgelinemenu( *new uiVisMenu(uip,-1) )
{
    nodemenu.ref();
    interactionlinemenu.ref();
    edgelinemenu.ref();

    visSurvey::EMObjectDisplay* emod = visSurvey::EMObjectDisplay::create();
    emod->setDisplayTransformation(visSurvey::SPM().getUTM2DisplayTransform());

    uiCursorChanger cursorchanger(uiCursor::Wait);
    if ( !emod->setEMObject(mid) ) mRefUnrefRet

    EM::EMManager& em = EM::EMM();
    mDynamicCastGet(EM::EMObject*,emobj,em.getObject(em.multiID2ObjectID(mid)));

    visserv->addObject( emod, scene, true );
    displayid = emod->id();
    emod->setDepthAsAttrib();

    setUpConnections();
}


uiVisEMObject::~uiVisEMObject()
{
    uiVisMenu* menu = visserv->getMenu(displayid,false);
    if ( menu )
    {
	menu->createnotifier.remove( mCB(this,uiVisEMObject,createMenuCB) );
	menu->handlenotifier.remove( mCB(this,uiVisEMObject,handleMenuCB) );
    }

    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,
	    	    visserv->getObject(displayid))
    if ( emod && emod->getEditor() )
    {
	emod->getEditor()->noderightclick.remove(
		mCB(this,uiVisEMObject,nodeRightClick) );
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
    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,
	    	    visBase::DM().getObject(displayid));
    return emod;
}


void uiVisEMObject::prepareForShutdown()
{
    const MultiID* mid = visserv->getMultiID( displayid );
    if ( !mid ) return;
    EM::ObjectID emid = EM::EMM().multiID2ObjectID( *mid );
    mDynamicCastGet(EM::EMObject*,emobj,EM::EMM().getObject(emid))
    if ( !emobj || !emobj->isChanged(-1) )
	return;

    BufferString msg( emobj->getTypeStr() );
    msg += " '";
    msg += emobj->name(); msg += "' has changed.\nDo you want to save it?";
    if ( uiMSG().askGoOn( msg, true, "Object Saver" ) )
    {
	PtrMan<Executor> saver = emobj->saver();
	uiCursorChanger uicursor( uiCursor::Wait );
	if ( saver )
	    saver->execute();
    }
}


void uiVisEMObject::setUpConnections()
{
    uiVisMenu* menu = visserv->getMenu(displayid,true);
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

    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,
	    	    visserv->getObject(displayid));
    if ( emod && emod->getEditor() )
    {
	emod->getEditor()->noderightclick.notify(
		mCB(this,uiVisEMObject,nodeRightClick) );
	//interactionlinemenu.setID( emod->getEditor()->lineID() );
	//edgelinemenu.setID( emod->getEditor()->lineID() );
    }
}


const char* uiVisEMObject::getObjectType( int id )
{
    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,visBase::DM().getObject(id));
    if ( !emod ) return 0;

    const MultiID* mid = emod->getMultiID();
    if ( !mid ) return 0;

    return EM::EMM().objectType( EM::EMM().multiID2ObjectID(*mid) );
}


bool uiVisEMObject::canHandle( int id )
{
    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,
	    	    visBase::DM().getObject(id));
    return emod;
}


void uiVisEMObject::setDepthAsAttrib()
{
    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,
	    	    visserv->getObject(displayid))
    emod->setDepthAsAttrib();
}


void uiVisEMObject::readAuxData()
{
    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,
	    	    visserv->getObject(displayid))
    emod->readAuxData();
}


int uiVisEMObject::nrSections() const
{
    const MultiID* mid = visserv->getMultiID( displayid );
    EM::ObjectID emid = EM::EMM().multiID2ObjectID( *mid );
    mDynamicCastGet(const EM::EMObject*,emobj,EM::EMM().getObject(emid))
    if ( !emobj ) return 0;

    return emobj->nrSections();
}


EM::SectionID uiVisEMObject::getSectionID( int idx ) const
{
    const MultiID* mid = visserv->getMultiID( displayid );
    EM::ObjectID emid = EM::EMM().multiID2ObjectID( *mid );
    mDynamicCastGet(const EM::EMObject*,emobj,EM::EMM().getObject(emid))
    if ( !emobj ) return -1;

    return emobj->sectionID( idx );
}


EM::SectionID uiVisEMObject::getSectionID( const TypeSet<int>* path ) const
{
    if ( !path ) return -1;
    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,
	    	    visserv->getObject(displayid))

    return emod->getSectionID( path );
}


float uiVisEMObject::getShift() const
{
    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,
		    visserv->getObject(displayid))
    Coord3 shift = emod->getTranslation();
    return shift.z;
}


/*
NotifierAccess* uiVisEMObject::finishEditingNotifier()
{
    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,visserv->getObject(displayid))
    return emod && emod->getEditor() ? &emod->getEditor()->finishedEditing : 0;
}


void uiVisEMObject::getMovedNodes(TypeSet<EM::PosID>& res) const
{
    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,visserv->getObject(displayid))
    if ( emod && emod->getEditor() ) emod->getEditor()->getMovedNodes(res);
}


bool uiVisEMObject::snapAfterEdit() const
{
    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,visserv->getObject(displayid))
    return emod && emod->getEditor() ? emod->getEditor()->snapAfterEdit() : false;
}


*/

void uiVisEMObject::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiVisMenu*,menu,cb)
    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,
	    	    visserv->getObject(displayid))

    uiMenuItem* colitm = new uiMenuItem( "Use single color" );
    singlecolmnuid = menu->addItem( colitm );
    colitm->setChecked( !emod->usesTexture() );

    shiftmnuid = !strcmp(getObjectType(displayid),EM::Horizon::typeStr())
	? menu->addItem( new uiMenuItem("Shift ...") )
	: -1;

    uiPopupMenu* trackmnu = new uiPopupMenu( uiparent, 
	    				     uiVisEMObject::trackingmenutxt );
    menu->addSubMenu( trackmnu );
    
    uiMenuItem* edititem = new uiMenuItem( "Edit" );
    editmnuid = menu->getFreeID();
    trackmnu->insertItem( edititem, editmnuid );
    edititem->setChecked( emod->isEditingEnabled() );

    uiMenuItem* wireframeitem = new uiMenuItem("Wireframe");
    wireframemnuid = menu->getFreeID();
    trackmnu->insertItem( wireframeitem, wireframemnuid );
    wireframeitem->setChecked( emod->usesWireframe() );

    const MultiID* mid = visserv->getMultiID( displayid );
    if ( !mid ) return;
    EM::ObjectID emid = EM::EMM().multiID2ObjectID( *mid );
    mDynamicCastGet(EM::EMObject*,emobj,EM::EMM().getObject(emid));
    removesectionmnuid =
	emobj->nrSections()>1&&emod->getSectionID(menu->getPath())!=-1
	? menu->addItem( new uiMenuItem("Remove section") ) : -1;
}


void uiVisEMObject::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(uiVisMenu*,menu,caller)
    if ( mnuid==-1 || menu->isHandled() )
	return;

    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,
	    	    visserv->getObject(displayid))

    if ( mnuid==singlecolmnuid )
    {
	emod->useTexture( !emod->usesTexture() );
	menu->setIsHandled(true);
    }
    else if ( mnuid==wireframemnuid )
    {
	menu->setIsHandled(true);
	emod->useWireframe( !emod->usesWireframe() );
    }
    else if ( mnuid==editmnuid )
    {
	emod->enableEditing(!emod->isEditingEnabled());
	menu->setIsHandled(true);
    }
    else if ( mnuid==shiftmnuid )
    {
	menu->setIsHandled(true);
	Coord3 shift = emod->getTranslation();
	BufferString lbl( "Shift " ); lbl += SI().getZUnit();
	DataInpSpec* inpspec = new FloatInpSpec( shift.z );
	uiGenInputDlg dlg( uiparent,"Specify horizon shift", lbl, inpspec );
	if ( !dlg.go() ) return;

	double newshift = dlg.getfValue();
	if ( shift.z == newshift ) return;

	shift.z = newshift;
	emod->setTranslation( shift );
	if ( !emod->hasStoredAttrib() )
	    visserv->calculateAttrib( displayid, false );
	else
	{
	    uiMSG().error( "Cannot calculate this attribute on new location"
		           "\nDepth will be displayed instead" );
	    emod->setDepthAsAttrib();
	}
	visserv->triggerTreeUpdate();
    }
    else if ( mnuid==removesectionmnuid )
    {
	const MultiID* mid = visserv->getMultiID( displayid );
	if ( !mid ) return;
	EM::ObjectID emid = EM::EMM().multiID2ObjectID( *mid );
	mDynamicCastGet(EM::EMObject*,emobj,EM::EMM().getObject(emid))
	emobj->removeSection(emod->getSectionID(menu->getPath()), true );

	EM::History& history = EM::EMM().history();
	const int currentevent = history.currentEventNr();
	history.setLevel(currentevent,mEMHistoryUserInteractionLevel);
    }
}

#define mMakePerm	0
#define mRemoveCtrl	1
#define mRemoveNode	2


void uiVisEMObject::interactionLineRightClick( CallBacker* cb )
{
    mCBCapsuleUnpack( int, nodedisplayid, cb );
    interactionlinemenu.setID(nodedisplayid);
    interactionlinemenu.executeMenu(uiVisMenu::fromScene);
}


void uiVisEMObject::nodeRightClick( CallBacker* cb )
{
    mCBCapsuleUnpack( int, nodedisplayid, cb );
    nodemenu.setID(nodedisplayid);
    nodemenu.executeMenu(uiVisMenu::fromScene);
}


void uiVisEMObject::edgeLineRightClick( CallBacker* cb )
{
    /*
    mCBCapsuleUnpack(const visSurvey::EdgeLineSetDisplay*,edgelinedisplay,cb);
    if ( !edgelinedisplay ) return;

    edgelinemenu.setID(edgelinedisplay->id());
    edgelinemenu.executeMenu(uiVisMenu::fromScene);
    */
}


void uiVisEMObject::createNodeMenuCB( CallBacker* cb )
{
    /*
    mDynamicCastGet(uiVisMenu*,menu,cb)
    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,visserv->getObject(displayid))

    const EM::PosID empid = emod->getEditor()->getNodePosID(menu->id());

    EM::EMManager& em = EM::EMM();
    EM::EMObject* emobj = em.getObject(empid.objectID());

    makepermnodemnusel =
	emobj->isPosAttrib(*empid,EM::EMObject::sTemporaryControlNode)
	    ? menu->addItem( new uiMenuItem("Make control permanent") ) 
	    : -1;

    removecontrolnodemnusel = 
	emobj->isPosAttrib(*empid,EM::EMObject::sPermanentControlNode)
	    ? menu->addItem( new uiMenuItem("Remove control") )
	    : -1;

    removenodenodemnusel = emobj->isDefined(*empid)
        ? menu->addItem( new uiMenuItem("Remove node") )
	: -1;

    uiMenuItem* snapitem = new uiMenuItem("Snap after edit");
    tooglesnappingnodemnusel = menu->addItem(snapitem);
    snapitem->setChecked(emod->getEditor()->snapAfterEdit());
    */
}


void uiVisEMObject::handleNodeMenuCB( CallBacker* cb )
{
    /*
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(uiVisMenu*,menu,caller)
    if ( mnuid==-1 || menu->isHandled() )
	return;

    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,visserv->getObject(displayid))

    const EM::PosID* empid = emod->getEditor()->getEMPosFromDisplayID(menu->id());

    EM::EMManager& em = EM::EMM();
    EM::EMObject* emobj = em.getObject(empid->objectID());

    if ( mnuid==makepermnodemnusel )
    {
	menu->setIsHandled(true);
        emobj->setPosAttrib(*empid,EM::EMObject::sPermanentControlNode,true);
	emobj->setPosAttrib(*empid,EM::EMObject::sTemporaryControlNode,false);
	emobj->setPosAttrib(*empid,EM::EMObject::sEdgeControlNode,false);
    }
    else if ( mnuid==removecontrolnodemnusel )
    {
	menu->setIsHandled(true);
        emobj->setPosAttrib(*empid,EM::EMObject::sPermanentControlNode,false);
	emobj->setPosAttrib(*empid,EM::EMObject::sTemporaryControlNode,false);
	emobj->setPosAttrib(*empid,EM::EMObject::sEdgeControlNode,false);
    }
    else if ( mnuid==removenodenodemnusel )
    {
	menu->setIsHandled(true);
	emobj->setPos(*empid,Coord3(mUndefValue,mUndefValue,mUndefValue),true);
    }
    else if ( mnuid==tooglesnappingnodemnusel )
    {
	menu->setIsHandled(true);
        emod->getEditor()->setSnapAfterEdit(!emod->getEditor()->snapAfterEdit());
    }
    */
}


void uiVisEMObject::createInteractionLineMenuCB( CallBacker* cb )
{
    /*
    mDynamicCastGet(uiVisMenu*,menu,cb)
    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,visserv->getObject(displayid))
    mDynamicCastGet( const visSurvey::EdgeLineSetDisplay*, linedisplay,
	    	     visserv->getObject(emod->getEditor()->lineID()) );

    const EM::EdgeLineSegment& interactionline =
	*linedisplay->getEdgeLineSet()->getLine(0)->getSegment(0);

    EM::EMManager& em = EM::EMM();
    mDynamicCastGet( EM::EMObject*, emobj,
	    em.getObject(interactionline.getSurface().id()));
    EM::EdgeLineSet* lineset =
	emobj->edgelinesets.getEdgeLineSet(interactionline.getSection(),true);
    if ( !lineset )
	return;

    const int mainlineidx = lineset->getMainLine();
    EM::EdgeLine* line = lineset->getLine(mainlineidx);
    if ( !line )
	return;

    bool noneonedge = false;
    bool canstop = false;
    if ( line->getSegment( interactionline.first() )!=-1 &&
	 line->getSegment( interactionline.last() )!=-1 )
    {
	noneonedge = true;

	for ( int idx=1; idx<interactionline.size()-1; idx++ )
	{
	    const EM::PosID posid( interactionline.getSurface().id(),
		       interactionline.getSection(),
		       emobj->geometry.rowCol2SubID(interactionline[idx]));
	    if ( emobj->geometry.isAtEdge(posid) )
		noneonedge = false;
	}

	int dummy;
	bool dummybool;
	canstop = canMakeStopLine( *lineset, interactionline, dummy, dummybool);
    }

    uiMenuItem* smallitem = new uiMenuItem("Cut away small part");
    smallitem->setEnabled(noneonedge);
    cutsmalllinemnusel = menu->addItem( smallitem );

    uiMenuItem* largeitem = new uiMenuItem("Cut away large part");
    largeitem->setEnabled(noneonedge);
    cutlargelinemnusel = menu->addItem( largeitem );

    uiMenuItem* splititem = new uiMenuItem("Split");
    splititem->setEnabled(noneonedge);
    splitlinemnusel = menu->addItem( splititem );

    uiMenuItem* stopitem = new uiMenuItem("Disable tracking");
    stopitem->setEnabled(canstop);
    mkstoplinemnusel = menu->addItem( stopitem );
    */
}


void uiVisEMObject::handleInteractionLineMenuCB( CallBacker* cb )
{
    /*
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(uiVisMenu*,menu,caller)
    if ( mnuid==-1 || menu->isHandled() )
	return;

    mDynamicCastGet(visSurvey::EMObjectDisplay*,emod,visserv->getObject(displayid))
    mDynamicCastGet( const visSurvey::EdgeLineSetDisplay*, linedisplay,
	    	     visserv->getObject(emod->getEditor()->lineID()) );

    const EM::EdgeLineSegment& interactionline =
	*linedisplay->getEdgeLineSet()->getLine(0)->getSegment(0);

    EM::EMManager& em = EM::EMM();
    mDynamicCastGet( EM::EMObject*, emobj,
	    em.getObject(interactionline.getSurface().id()));
    EM::EdgeLineSet* lineset =
	emobj->edgelinesets.getEdgeLineSet(interactionline.getSection(),true);
    if ( !lineset )
	return;

    const int mainlineidx = lineset->getMainLine();
    EM::EdgeLine* line = lineset->getLine(mainlineidx);
    if ( !line )
	return;

    if ( line->getSegment( interactionline.first() )==-1 ||
	 line->getSegment( interactionline.last() )==-1 )
	return;

    if ( mnuid==cutsmalllinemnusel || mnuid==cutlargelinemnusel )
    {
	PtrMan<EM::EdgeLineSet> part1lineset = lineset->clone();
	PtrMan<EM::EdgeLineSet> part2lineset = lineset->clone();
	EM::EdgeLine* part1line = part1lineset->getLine(mainlineidx);
	EM::EdgeLine* part2line = part2lineset->getLine(mainlineidx);

	EM::EdgeLineSegment* part1cut = interactionline.clone();
	EM::EdgeLineSegment* part2cut = new EM::EdgeLineSegment(
		    part1cut->getSurface(), interactionline.getSection() );

	for ( int idx=interactionline.size()-1; idx>=0; idx-- )
	    (*part2cut) += interactionline[idx];

	part1line->insertSegment( part1cut, -1, true );
	part2line->insertSegment( part2cut, -1, true );

	const int area1 = part1line->computeArea();
	const int area2 = part2line->computeArea();
	const bool keeppart1 = area1>area2==(mnuid==cutsmalllinemnusel);

	lineset->getLine(mainlineidx)->insertSegment(
	    keeppart1 ? interactionline.clone() : part2cut->clone(), -1, true );

	lineset->removeAllNodesOutsideLines();
	menu->setIsHandled(true);
	emod->getEditor()->clearInteractionLine();
    }
    else if ( mnuid==splitlinemnusel )
    {
	const EM::SectionID newsection =
	    emobj->geometry.cloneSection(interactionline.getSection());

	EM::SurfaceConnectLine* part1cut =
	    EM::SurfaceConnectLine::create( *emobj, 
					    interactionline.getSection() );
	part1cut->setConnectingSection( newsection );
	for ( int idx=0; idx<interactionline.size(); idx++ )
	    (*part1cut) += interactionline[idx];

	EM::SurfaceConnectLine* part2cut =
	    EM::SurfaceConnectLine::create( *emobj, newsection );
	part2cut->setConnectingSection( interactionline.getSection() );
	for ( int idx=interactionline.size()-1; idx>=0; idx-- )
	    (*part2cut) += interactionline[idx];

	EM::EdgeLineSet* lineset2 =
	    emobj->edgelinesets.getEdgeLineSet(newsection,false);

	const int mainlineidx = lineset2->getMainLine();
	EM::EdgeLine* line2 = lineset2->getLine(mainlineidx);
	if ( !line2 )
	    return;

	line->insertSegment(part1cut,-1,true);
	lineset->removeAllNodesOutsideLines();
	line2->insertSegment(part2cut,-1,true);
	lineset2->removeAllNodesOutsideLines();
	menu->setIsHandled(true);
	emod->getEditor()->clearInteractionLine();
    }
    else if ( mnuid==mkstoplinemnusel )
    {
	int linenr;
	bool forward;
	if ( !canMakeStopLine( *lineset, interactionline, linenr, forward) )
	    return;

	EM::EdgeLineSegment* terminationsegment =
	    EM::TerminationEdgeLineSegment::create( *emobj, 
		    interactionline.getSection() );
	terminationsegment->copyNodesFrom(&interactionline, !forward );

	lineset->getLine(linenr)->insertSegment( terminationsegment, -1, true );
	menu->setIsHandled(true);
	emod->getEditor()->clearInteractionLine();
    }
    */
}


void uiVisEMObject::createEdgeLineMenuCB( CallBacker* cb )
{
    /*
    mDynamicCastGet(uiVisMenu*,menu,cb);
    mDynamicCastGet(visSurvey::EdgeLineSetDisplay*,edgelinedisplay,
	            visserv->getObject(menu->id()));

    const EM::EdgeLineSet* edgelineset = edgelinedisplay->getEdgeLineSet();
    const EM::EdgeLine* edgeline =
	    edgelineset->getLine(edgelinedisplay->getRightClickedLine());
    const EM::EdgeLineSegment* segment =
	edgeline->getSegment(edgelinedisplay->getRightClickedSegment());

    mDynamicCastGet( const EM::SurfaceConnectLine*, connectline,segment);
    mDynamicCastGet( const EM::TerminationEdgeLineSegment*,
		     terminationline, segment );

    removetermedgelinemnusel = -1;
    removeconnedgelinemnusel = -1;
    joinedgelinemnusel = -1;
    if ( terminationline )
    {
	removetermedgelinemnusel =
	    menu->addItem( new uiMenuItem("Remove termination") );
    }
    else if ( connectline )
    {
	removeconnedgelinemnusel =
	    menu->addItem( new uiMenuItem("Remove connection") );
	joinedgelinemnusel =
	    menu->addItem( new uiMenuItem("Join sections") );
    }
    */
}


void uiVisEMObject::handleEdgeLineMenuCB( CallBacker* cb )
{
    /*
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(uiVisMenu*,menu,caller)
    if ( mnuid==-1 || menu->isHandled() )
	return;

    mDynamicCastGet(visSurvey::EdgeLineSetDisplay*,edgelinedisplay,
	            visserv->getObject(menu->id()));

    EM::EdgeLineSet* edgelineset =
	const_cast<EM::EdgeLineSet*>(edgelinedisplay->getEdgeLineSet());
    EM::EdgeLine* edgeline =
	    edgelineset->getLine(edgelinedisplay->getRightClickedLine());
    EM::EdgeLineSegment* segment =
	edgeline->getSegment(edgelinedisplay->getRightClickedSegment());

    if ( mnuid==removetermedgelinemnusel )
    {
	EM::EdgeLineSegment* replacement =
	   new EM::EdgeLineSegment(segment->getSurface(),segment->getSection());
	replacement->copyNodesFrom(segment,false);
	edgeline->insertSegment( replacement, -1, true );
	menu->setIsHandled(true);
    }
    else
    {
	pErrMsg("Not implemented");
	menu->setIsHandled(true);
    }
    */
}

/*
bool uiVisEMObject::canMakeStopLine( const EM::EdgeLineSet& lineset,
			const EM::EdgeLineSegment& interactionline, int& linenr,
			bool& forward )
{
    bool canstop = false;
    for ( int idx=0; !canstop && idx<lineset.nrLines(); idx++ )
    {
	const EM::EdgeLine* curline = lineset.getLine(idx);

	int firstsegpos;
	const int firstsegment =
	    curline->getSegment(interactionline[0],&firstsegpos);


	if ( firstsegment==-1 )
	    continue;

	EM::EdgeLineIterator
	    fwditer(*curline,true,firstsegment,firstsegpos);
	if ( !fwditer.isOK() ) continue;
	EM::EdgeLineIterator
	    backiter(*curline,false,firstsegment,firstsegpos );
	if ( !backiter.isOK() ) continue;

	canstop = true;
	for ( int idy=1; canstop && idy<interactionline.size(); idy++ )
	{
	    canstop = false;

	    if ( idy==1 || forward )
	    {
		fwditer.next();
		if ( fwditer.currentRowCol()==interactionline[idy] )
		{
		    if ( idy==1 ) forward = true;
		    canstop = true;
		    continue;
		}
	    }

	    if ( idy==1 || !forward )
	    {
		backiter.next();
		if ( backiter.currentRowCol()==interactionline[idy] )
		{
		    if ( idy==1 ) forward=false;
		    canstop = true;
		}
	    }
	}

	if ( canstop )
	{
	    linenr = idx;
	    return true;
	}
    }

    return false;
}

*/
