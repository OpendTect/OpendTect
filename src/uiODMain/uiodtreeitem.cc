/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		Jul 2003
 RCS:		$Id: uiodtreeitem.cc,v 1.26 2004-05-27 15:38:34 kristofer Exp $
___________________________________________________________________

-*/

#include "uiodtreeitemimpl.h"
#include "errh.h"
#include "ptrman.h"
#include "ioobj.h"
#include "ioman.h"
#include "uimenu.h"
#include "pickset.h"
#include "survinfo.h"
#include "uilistview.h"
#include "uibinidtable.h"
#include "uivismenu.h"
#include "uisoviewer.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uimsg.h"
#include "uigeninputdlg.h"
#include "uiempartserv.h"
#include "uiwellpropdlg.h"
#include "uivispartserv.h"
#include "uiwellpartserv.h"
#include "uipickpartserv.h"
#include "uiwellattribpartserv.h"
#include "uiattribpartserv.h"
#include "uitrackingpartserv.h"
#include "uislicesel.h"
#include "uipickszdlg.h"

#include "visrandomtrackdisplay.h"
#include "vissurvwell.h"
#include "vissurvpickset.h"
#include "vissurvscene.h"
#include "settings.h"
#include "visplanedatadisplay.h"
#include "uivissurface.h"
#include "uiexecutor.h"


const char* uiODTreeTop::sceneidkey = "Sceneid";
const char* uiODTreeTop::viewerptr = "Viewer";
const char* uiODTreeTop::applmgrstr = "Applmgr";
const char* uiODTreeTop::scenestr = "Scene";

uiODTreeTop::uiODTreeTop( uiSoViewer* sovwr, uiListView* lv, uiODApplMgr* am,
			    uiTreeFactorySet* tfs_ )
    : uiTreeTopItem(lv)
    , tfs(tfs_)
{
    setProperty<int>( sceneidkey, sovwr->sceneId() );
    setPropertyPtr<uiSoViewer*>( viewerptr, sovwr );
    setPropertyPtr<uiODApplMgr*>( applmgrstr, am );
    // setPropertyPtr<uiODSceneMgr::Scene*>( scenestr, sc );

    tfs->addnotifier.notify( mCB(this,uiODTreeTop,addFactoryCB) );
    tfs->removenotifier.notify( mCB(this,uiODTreeTop,removeFactoryCB) );
}


uiODTreeTop::~uiODTreeTop()
{
    tfs->addnotifier.remove( mCB(this,uiODTreeTop,addFactoryCB) );
    tfs->removenotifier.remove( mCB(this,uiODTreeTop,removeFactoryCB) );
}


int uiODTreeTop::sceneID() const
{
    int sceneid=-1;
    getProperty<int>( sceneidkey, sceneid );
    return sceneid;
}


bool uiODTreeTop::select(int selkey)
{
    applMgr()->visServer()->setSelObjectId(selkey);
    return true;
}


uiODApplMgr* uiODTreeTop::applMgr()
{
    uiODApplMgr* res = 0;
    getPropertyPtr<uiODApplMgr*>( applmgrstr, res );
    return res;
}


uiODTreeItem::uiODTreeItem( const char* name__ )
    : uiTreeItem( name__ )
{}


uiODApplMgr* uiODTreeItem::applMgr()
{
    uiODApplMgr* res = 0;
    getPropertyPtr<uiODApplMgr*>( uiODTreeTop::applmgrstr, res );
    return res;
}


uiSoViewer* uiODTreeItem::viewer()
{
    uiSoViewer* res = 0;
    getPropertyPtr<uiSoViewer*>( uiODTreeTop::viewerptr, res );
    return res;
}


int uiODTreeItem::sceneID() const
{
    int sceneid=-1;
    getProperty<int>( uiODTreeTop::sceneidkey, sceneid );
    return sceneid;
}


void uiODTreeTop::addFactoryCB(CallBacker* cb)
{
    mCBCapsuleUnpack(int,idx,cb);
    addChild( tfs->getFactory(idx)->create() );
}


void uiODTreeTop::removeFactoryCB(CallBacker* cb)
{
    mCBCapsuleUnpack(int,idx,cb);
    PtrMan<uiTreeItem> dummy = tfs->getFactory(idx)->create();
    const uiTreeItem* child = findChild( dummy->name() );
    if ( children.indexOf(child)==-1 )
	return;

    removeChild( const_cast<uiTreeItem*>(child) );
}


#define mDisplayInit( inherited, creationfunc, checkfunc ) \
\
    if ( displayid==-1 ) \
    {	\
	displayid = applMgr()->visServer()->creationfunc; \
	if ( displayid==-1 ) \
	{\
	    return false;\
	}\
    } \
    else if ( !applMgr()->visServer()->checkfunc ) \
	return false;  \
\
    if ( !inherited::init() ) \
	return false; \



#define mParentShowSubMenu( creation ) \
    uiPopupMenu mnu( getUiParent(), "Action" ); \
    mnu.insertItem( new uiMenuItem("Add"), 0 ); \
    const int mnuid = mnu.exec(); \
    if ( mnuid==0 ) \
	creation; \
 \
    return true; \


bool uiODDisplayTreeItem::factory( uiTreeItem* treeitem, uiODApplMgr* appl,
				int displayid )
{
    const uiTreeFactorySet* tfs = ODMainWin()->sceneMgr().treeItemFactorySet();
    if ( !tfs )
	return false;

    for ( int idx=0; idx<tfs->nrFactories(); idx++ )
    {
	mDynamicCastGet( const uiODTreeItemFactory*, treefact,
			 tfs->getFactory(idx));
	if ( !treefact ) continue;

	uiTreeItem* res = treefact->create(displayid);
	if ( res )
	{
	    treeitem->addChild( res );
	    return true;
	}
    }

    return false;
}

	

uiODDisplayTreeItem::uiODDisplayTreeItem( )
    : uiODTreeItem( 0 )
    , displayid( -1 )
{
}


uiODDisplayTreeItem::~uiODDisplayTreeItem( )
{
    uiVisPartServer* visserv = applMgr()->visServer();
    uiVisMenu* menu = visserv->getMenu( displayid, false );
    if ( menu )
    {
	menu->createnotifier.remove(mCB(this,uiODDisplayTreeItem,createMenuCB));
	menu->handlenotifier.remove(mCB(this,uiODDisplayTreeItem,handleMenuCB));
    }
}


int uiODDisplayTreeItem::selectionKey() const { return displayid; }


bool uiODDisplayTreeItem::init()
{
    if ( !uiTreeItem::init() ) return false;

    uiVisPartServer* visserv = applMgr()->visServer();

    visserv->setSelObjectId( displayid );
    uilistviewitem->setChecked( visserv->isOn(displayid) );
    uilistviewitem->stateChanged.notify( mCB(this,uiODDisplayTreeItem,checkCB));

    name_ = createDisplayName();

    uiVisMenu* menu = visserv->getMenu( displayid, true );
    menu->createnotifier.notify( mCB(this,uiODDisplayTreeItem,createMenuCB));
    menu->handlenotifier.notify( mCB(this,uiODDisplayTreeItem,handleMenuCB));

    return true;
}


void uiODDisplayTreeItem::updateColumnText( int col )
{
    if ( !col )
    {
	name_ = createDisplayName();
	uiTreeItem::updateColumnText(col);
    }
}


bool uiODDisplayTreeItem::showSubMenu()
{
    return applMgr()->visServer()->showMenu(displayid);
}


void uiODDisplayTreeItem::checkCB(CallBacker*)
{
    applMgr()->visServer()->turnOn( displayid, uilistviewitem->isChecked() );
}


int uiODDisplayTreeItem::uiListViewItemType() const
{
    return uiListViewItem::CheckBox;
}


BufferString uiODDisplayTreeItem::createDisplayName() const
{
    const uiVisPartServer* visserv = const_cast<uiODDisplayTreeItem*>(this)->
							applMgr()->visServer();
    const AttribSelSpec* as =  visserv->getSelSpec( displayid );
    BufferString dispname( as ? as->userRef() : 0 );
    if ( as && as->isNLA() )
    {
	dispname = as->objectRef();
	const char* nodenm = as->userRef();
	if ( IOObj::isKey(as->userRef()) )
	    nodenm = IOM().nameOf( as->userRef(), false );
	dispname += " ("; dispname += nodenm; dispname += ")";
    }

    if ( as && !dispname[0] )
	dispname = "<right-click>";
    else if ( !as )
	dispname = visserv->getObjectName(displayid);

    return dispname;
}



const char* uiODDisplayTreeItem::attrselmnutxt = "Select Attribute ...";


void uiODDisplayTreeItem::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet( uiVisMenu*, menu, cb );
    uiVisPartServer* visserv = applMgr()->visServer();
    if ( visserv->hasAttrib(displayid) )
    {
	uiPopupMenu* selattrmnu = new uiPopupMenu( menu->getParent(),
						   attrselmnutxt);
	firstsetattrmnuid = menu->getFreeID();
	lastsetattrmnuid = firstsetattrmnuid;
	applMgr()->attrServer()->createAttribSubMenu( *selattrmnu,
					  lastsetattrmnuid,
					  *visserv->getSelSpec(displayid));
	for ( int idx=firstsetattrmnuid; idx<=lastsetattrmnuid; idx++ )
	    menu->getFreeID();

	menu->addSubMenu(selattrmnu,9999);
    }
    else 
	firstsetattrmnuid = lastsetattrmnuid = -1;

    duplicatemnuid = visserv->canDuplicate(displayid)
		    ? menu->addItem( new uiMenuItem("Duplicate") ) 
		    : -1;

    TypeSet<int> sceneids;
    visserv->getChildIds(-1,sceneids);
    if ( sceneids.size()>1 )
    {
	sharefirstmnuid = menu->getCurrentID();
	uiPopupMenu* sharemnu = new uiPopupMenu( menu->getParent(),
						 "Share with...");
	for ( int idx=0; idx<sceneids.size(); idx++ )
	{
	    if ( sceneids[idx]!=sceneID() )
	    {
		uiMenuItem* itm =
		    	new uiMenuItem(visserv->getObjectName(sceneids[idx]));
		sharemnu->insertItem( itm, menu->getFreeID() );
	    }
	}

	menu->addSubMenu( sharemnu );

	sharelastmnuid = menu->getCurrentID()-1;
    }
    else
    {
	sharefirstmnuid = -1;
	sharelastmnuid = -1;
    }

    removemnuid = menu->addItem( new uiMenuItem("Remove"), -1000 );
}


void uiODDisplayTreeItem::handleMenuCB(CallBacker* cb)
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( uiVisMenu*, menu, caller );
    uiVisPartServer* visserv = applMgr()->visServer();
    if ( mnuid==-1 || menu->isHandled() ) return;
    if ( mnuid==duplicatemnuid )
    {
	menu->setIsHandled(true);
	int newid =visserv->duplicateObject(displayid,sceneID());
	if ( newid!=-1 )
	    uiODDisplayTreeItem::factory( this, applMgr(), newid );
    }
    else if ( mnuid==removemnuid )
    {
	menu->setIsHandled(true);
	visserv->removeObject( displayid, sceneID() );
	parent->removeChild( this );
    }
    else if ( firstsetattrmnuid!=-1 && mnuid>=firstsetattrmnuid &&
	    mnuid<=lastsetattrmnuid )
    {
	menu->setIsHandled(true);
	const AttribSelSpec* as = visserv->getSelSpec( displayid );
	AttribSelSpec myas( *as );
	if ( applMgr()->attrServer()->handleAttribSubMenu(
		    mnuid-firstsetattrmnuid, myas ))
	{
	    visserv->setSelSpec( displayid, myas );
	    visserv->resetColorDataType( displayid );
	    visserv->calculateAttrib( displayid, false );
	    updateColumnText(0);
	}
    }
    else if ( firstsetattrmnuid!=-1 && mnuid>=sharefirstmnuid &&
	    mnuid<=sharelastmnuid )
    {
	menu->setIsHandled(true);
	TypeSet<int> sceneids;
	visserv->getChildIds(-1,sceneids);
	int idy=0;
	for ( int idx=0; idx<mnuid-sharefirstmnuid; idx++ )
	{
	    if ( sceneids[idx]!=sceneID() )
	    {
		idy++;
		if ( idy==mnuid-sharefirstmnuid )
		{
		    visserv->shareObject(sceneids[idx], displayid);
		    break;
		}
	    }
	}
    }
}


uiODEarthModelSurfaceTreeItem::uiODEarthModelSurfaceTreeItem(
						const MultiID& mid_)
    : mid( mid_ )
    , uivissurf( 0 )
{}


uiODEarthModelSurfaceTreeItem::~uiODEarthModelSurfaceTreeItem()
{ delete uivissurf; }


bool uiODEarthModelSurfaceTreeItem::init()
{
    if ( uivissurf ) delete uivissurf;
    if ( displayid!=-1 )
    {
	uivissurf = new uiVisSurface( getUiParent(), displayid,
				      applMgr()->visServer() );
	if ( !uivissurf->isOK() )
	{
	    delete uivissurf;
	    uivissurf = 0;
	    return false;
	}

	mid = *applMgr()->visServer()->getMultiID(displayid);
    }
    else
    {
	uivissurf = new uiVisSurface( getUiParent(), mid, sceneID(),
				      applMgr()->visServer() );
	displayid = uivissurf->id();
    }

    if ( !uiODDisplayTreeItem::init() )
	return false; 

    return true;
}


void uiODEarthModelSurfaceTreeItem::checkCB(CallBacker* cb)
{
    uiODDisplayTreeItem::checkCB(cb);

    if ( applMgr()->trackServer()->getTrackerID(mid)==-1 )
    {
	prevtrackstatus = false;
        return;
    }

    if ( uilistviewitem->isChecked() )
    {
	applMgr()->trackServer()->enableTracking(mid, prevtrackstatus);
    }
    else
    {
	prevtrackstatus = applMgr()->trackServer()->isTrackingEnabled(mid);
	applMgr()->trackServer()->enableTracking(mid,false);
    }
}


BufferString uiODEarthModelSurfaceTreeItem::createDisplayName() const
{
    const uiVisPartServer* visserv =
       const_cast<uiODEarthModelSurfaceTreeItem*>(this)->applMgr()->visServer();
    const AttribSelSpec* as =  visserv->getSelSpec( displayid );
    bool hasattr = as && as->id() > -2;
    BufferString dispname;
    if ( hasattr )
    {
	dispname = uiODDisplayTreeItem::createDisplayName();
	dispname += " (";
    }
    dispname += visserv->getObjectName( displayid );
    if ( hasattr ) dispname += ")";
    return dispname;
}


void uiODEarthModelSurfaceTreeItem::createMenuCB(CallBacker* cb)
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet( uiVisMenu*, menu, cb );

    uiVisPartServer* visserv = applMgr()->visServer();

    bool dotrack = false;
    mGetTrackingBoolean(dotrack);    
    if ( dotrack && uilistviewitem->isChecked() )
    {
	if ( applMgr()->EMServer()->isFullResolution(mid) && 
	     applMgr()->trackServer()->getTrackerID(mid)==-1 )
	{
	    trackmnuid = menu->addItem( new uiMenuItem("Start tracking ...") );
	    tracksetupmnuid = -1;
	    toogletrackingmnuid = -1;
	}
	else
	{
	    trackmnuid = -1;
	    tracksetupmnuid = menu->addItem(new uiMenuItem("Change setup ..."));
	    uiMenuItem* tracktooglemnuitem = new uiMenuItem("Enable tracking");
	    toogletrackingmnuid = menu->addItem(tracktooglemnuitem);
	    tracktooglemnuitem->setChecked(
		    applMgr()->trackServer()->isTrackingEnabled(mid));
	}

	uiMenuItem* storemenuitem =  new uiMenuItem("Store");
	storemnuid = menu->addItem( storemenuitem );
	storemenuitem->setEnabled(applMgr()->EMServer()->isChanged(mid) );

	storemenuitem = new uiMenuItem("Store as ...");
	storeasmnuid = menu->addItem( storemenuitem );
	storemenuitem->setEnabled(applMgr()->EMServer()->isChanged(mid) );
    }
    else
    {
	storemnuid = trackmnuid = tracksetupmnuid = -1;
    }

    uiPopupMenu* attrmnu = menu->getMenu( attrselmnutxt );
    if ( attrmnu )
    {
	attribstartmnuid = menu->getCurrentID();
	const AttribSelSpec* as = visserv->getSelSpec(displayid);
	const bool hasauxdata = as && as->id() == -1;
	int nraddeditems = applMgr()->EMServer()->createAuxDataSubMenu(
				*attrmnu, attribstartmnuid, mid, hasauxdata );

	for ( int idx=0; idx<nraddeditems; idx++ )
	    menu->getFreeID();

	attribstopmnuid = menu->getCurrentID()-1;
    }
    else
    {
	attribstartmnuid = -1;
	attribstopmnuid = -1;
    }

    EM::PatchID section = -1;
    if ( uivissurf->nrSections()==1 )
	section = uivissurf->getSection(0);
    else if ( menu->getPath() )
	section = uivissurf->getSection(menu->getPath());

    if ( section!=-1 )
    {
	cutstartmnuid = menu->getCurrentID();
	cutstopmnuid = cutstartmnuid;
	uiPopupMenu* cutmenu = 
	    applMgr()->trackServer()->createTrackerMenu( cutstopmnuid, mid,
		    					 section, false );
	if ( cutmenu && cutmenu->nrItems() )
	{
	    menu->addSubMenu( cutmenu );
	    for ( int idx=0; idx<cutstopmnuid-cutstartmnuid+1; idx++ )
		menu->getFreeID();
	}

	terminatestartmnuid = menu->getCurrentID();
	terminatestopmnuid = terminatestartmnuid;
	uiPopupMenu* terminatemenu = 
	    applMgr()->trackServer()->createTrackerMenu( terminatestopmnuid,mid,
		    					 section, true );
	if ( terminatemenu && terminatemenu->nrItems() )
	{
	    menu->addSubMenu( terminatemenu );
	    for ( int idx=0; idx<terminatestopmnuid-terminatestartmnuid+1; idx++ )
		menu->getFreeID();
	}
    }
    else
    {
	cutstartmnuid = -1;
	cutstopmnuid = -1;
	terminatestartmnuid = -1;
	terminatestopmnuid = -1;
    }

#ifdef __debug__
    reloadmnuid = menu->addItem( new uiMenuItem("Reload") );
#else
    reloadmnuid = -1;
#endif
}


void uiODEarthModelSurfaceTreeItem::handleMenuCB(CallBacker* cb)
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( uiVisMenu*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
	return;

    uiVisPartServer* visserv = applMgr()->visServer();
	
    if ( mnuid==storeasmnuid )
    {
	menu->setIsHandled(true);
	applMgr()->storeSurface(displayid);
    }
    else if ( mnuid==storemnuid )
    {
	menu->setIsHandled(true);
	pErrMsg("Not Implemented yet");
    }
    else if ( mnuid==trackmnuid )
    {
	applMgr()->trackServer()->setSceneID( sceneID() );
	if ( uivissurf->isHorizon(displayid) )
	{
	    applMgr()->trackServer()->setDisplayID(displayid);
	    applMgr()->trackServer()->addHorizonTracker( mid, false );
	}
	else if ( uivissurf->isFault(displayid) )
	{
	    applMgr()->trackServer()->setDisplayID(displayid);
	    applMgr()->trackServer()->addFaultTracker( mid, false );
	}
    }
    else if ( mnuid==tracksetupmnuid )
    {
	applMgr()->trackServer()->showSetup( mid );
    }
    else if ( mnuid==reloadmnuid )
    {
	menu->setIsHandled(true);
	uiTreeItem* parent__ = parent;

	applMgr()->visServer()->removeObject(displayid, sceneID());
	delete uivissurf; uivissurf = 0;

	if ( !applMgr()->EMServer()->loadSurface( mid ) )
	    return;

	uivissurf = new uiVisSurface( getUiParent(), mid, sceneID(),
				      applMgr()->visServer() );
	displayid = uivissurf->id();
    }
    else if ( mnuid>=attribstartmnuid && mnuid<=attribstopmnuid )
    {
	menu->setIsHandled(true);
	if ( applMgr()->EMServer()->loadAuxData(mid, mnuid-attribstartmnuid) )
	    applMgr()->handleStoredSurfaceData( displayid );
    }
    else if ( mnuid>=cutstartmnuid && mnuid<=cutstopmnuid )
    {	
	menu->setIsHandled(true);
	EM::PatchID section = -1;
	if ( uivissurf->nrSections()==1 )
	    section = uivissurf->getSection(0);
	else if ( menu->getPath() )
	    section = uivissurf->getSection(menu->getPath());

	if ( section==-1 )
	    return;

	applMgr()->trackServer()->handleTrackerMenu(mnuid,cutstartmnuid,mid,
				  		    section,false);
    }
    else if ( mnuid>=terminatestartmnuid && mnuid<=terminatestopmnuid )
    {	
	menu->setIsHandled(true);
	EM::PatchID section = -1;
	if ( uivissurf->nrSections()==1 )
	    section = uivissurf->getSection(0);
	else if ( menu->getPath() )
	    section = uivissurf->getSection(menu->getPath());

	if ( section==-1 )
	    return;

	applMgr()->trackServer()->handleTrackerMenu(mnuid,terminatestartmnuid,
		mid, section,true);
    }
    else if ( mnuid==toogletrackingmnuid )
    {
	applMgr()->trackServer()->enableTracking(mid,
		!applMgr()->trackServer()->isTrackingEnabled(mid));
    }
}



/*
uiODFaultStickParentTreeItem::uiODFaultStickParentTreeItem()
    : uiODTreeItem("FaultSticks" )
{}


bool uiODFaultStickParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("New"), 0 );
    mnu.insertItem( new uiMenuItem("Load ..."), 1);

    const int mnuid = mnu.exec();

    MultiID mid;
    bool success = false;
    
    if ( mnuid==0 )
    {
	success = applMgr()->EMServer()->createStickSet(mid);
    }
    else if ( mnuid==1 )
    {
	success = applMgr()->EMServer()->selectStickSet(mid);
    }

    if ( !success )
	return false;

    addChild( new uiODFaultStickTreeItem(mid) );

    return true;
}


mMultiIDDisplayConstructor( FaultStick )

bool uiODFaultStickTreeItem::init()
{
    mMultiIDInit( addStickSet, isStickSet );
    return true;
}


bool uiODFaultStickTreeItem::showSubMenu()
{
    return uiODDisplayTreeItem::showSubMenu();
}

*/


uiTreeItem* uiODRandomLineFactory::create(int visid) const
{
    mDynamicCastGet( visSurvey::RandomTrackDisplay*, rtd, 
	    	     ODMainWin()->applMgr().visServer()->getObject(visid));
    return rtd ? new uiODRandomLineTreeItem(visid) : 0;
}



uiODRandomLineParentTreeItem::uiODRandomLineParentTreeItem()
    : uiODTreeItem( "Random line" )
{}


bool uiODRandomLineParentTreeItem::showSubMenu()
{
    mParentShowSubMenu( addChild(new uiODRandomLineTreeItem(-1)); );
}


uiODRandomLineTreeItem::uiODRandomLineTreeItem( int id )
{ displayid = id; } 


bool uiODRandomLineTreeItem::init()
{
    uiVisPartServer* visserv = applMgr()->visServer();
    if ( displayid==-1 )
    {
	visSurvey::RandomTrackDisplay* rtd =
				    visSurvey::RandomTrackDisplay::create();
	displayid = rtd->id();
	visserv->addObject( rtd, sceneID(), true );
    }
    else
    {
	mDynamicCastGet( visSurvey::RandomTrackDisplay*, rtd,
			  visserv->getObject(displayid));
	if ( !rtd ) return false;
    }

    return uiODDisplayTreeItem::init();
}


void uiODRandomLineTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet( uiVisMenu*, menu, cb );
    uiVisPartServer* visserv = applMgr()->visServer();
    editnodesmnuid = menu->addItem( new uiMenuItem("Edit nodes ...") );

    uiPopupMenu* insertnodemnu = new uiPopupMenu( menu->getParent(),
						  "Insert node ...");
    mDynamicCastGet( visSurvey::RandomTrackDisplay*,rtd,
	    	     visserv->getObject(displayid));

    for ( int idx=0; idx<=rtd->nrKnots(); idx++ )
    {
	BufferString nodename;
	if ( idx==rtd->nrKnots() )
	{
	    nodename = "after node ";
	    nodename += idx-1;
	}
	else
	{
	    nodename = "before node ";
	    nodename += idx;
	}

	const int mnuid = menu->getFreeID();
	uiMenuItem* itm = new uiMenuItem(nodename);
	insertnodemnu->insertItem( itm, mnuid );
	itm->setEnabled(rtd->canAddKnot(idx));
	if ( !idx )
	    insertnodemnuid = mnuid;
    }

    menu->addSubMenu(insertnodemnu);
}


void uiODRandomLineTreeItem::handleMenuCB(CallBacker* cb)
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( uiVisMenu*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
	return;
	
    uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet( visSurvey::RandomTrackDisplay*,rtd,
	    	     visserv->getObject(displayid));

    if ( mnuid==editnodesmnuid )
    {
	editNodes();
	menu->setIsHandled(true);
    }
    else if ( insertnodemnuid!=-1 && mnuid>=insertnodemnuid &&
	      mnuid<=insertnodemnuid+rtd->nrKnots() )
    {
	menu->setIsHandled(true);
	rtd->addKnot(mnuid-insertnodemnuid);
    }
}


void uiODRandomLineTreeItem::editNodes()
{
    uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet( visSurvey::RandomTrackDisplay*,rtd,
	    	     visserv->getObject(displayid));

    TypeSet<BinID> bidset;
    rtd->getAllKnotPos( bidset );
    uiBinIDTableDlg dlg( getUiParent(), "Specify nodes", bidset );
    if ( dlg.go() )
    {
	bool viewmodeswap = false;
	if ( visserv->isViewMode() )
	{
	    visserv->setViewMode( false );
	    viewmodeswap = true;
	}

	TypeSet<BinID> newbids;
	dlg.getBinIDs( newbids );
	if ( newbids.size() < 2 ) return;
	while ( rtd->nrKnots()>newbids.size() )
	    rtd->removeKnot( rtd->nrKnots()-1 );

	for ( int idx=0; idx<newbids.size(); idx++ )
	{
	    const BinID bid = newbids[idx];
	    if ( idx<rtd->nrKnots() )
		rtd->setManipKnotPos( idx, bid );
	    else
		rtd->addKnot( bid );
	}

	visserv->setSelObjectId( rtd->id() );
	visserv->calculateAttrib( rtd->id(), false );
	visserv->calculateColorAttrib( rtd->id(), false );
	if ( viewmodeswap ) visserv->setViewMode( true );
    }
}

uiODFaultParentTreeItem::uiODFaultParentTreeItem()
   : uiODTreeItem( "Fault" )
{}


bool uiODFaultParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("Load ..."), 0 );

    bool dotrack = false;
    mGetTrackingBoolean(dotrack);
    if ( dotrack )
	mnu.insertItem( new uiMenuItem("New ..."), 1 );

    const int mnuid = mnu.exec();

    MultiID mid;
    bool success = false;
    
    if ( mnuid == 0 )
	success = applMgr()->EMServer()->selectFault(mid);
    else if ( mnuid == 1 )
    {
	applMgr()->trackServer()->setSceneID( sceneID() );
	applMgr()->trackServer()->addFaultTracker( "", true );
	return true;
    }

    if ( !success )
	return false;

    addChild( new uiODFaultTreeItem(mid) );

    return true;
}


uiTreeItem* uiODFaultFactory::create(int visid) const
{
    return uiVisSurface::isFault(visid) ? new uiODFaultTreeItem(visid) : 0;
}


uiODFaultTreeItem::uiODFaultTreeItem( const MultiID& mid_ )
    : uiODEarthModelSurfaceTreeItem( mid_ )
{}


uiODFaultTreeItem::uiODFaultTreeItem( int id )
    : uiODEarthModelSurfaceTreeItem( 0 )
{ displayid=id; }


uiODHorizonParentTreeItem::uiODHorizonParentTreeItem()
    : uiODTreeItem( "Horizon" )
{}


bool uiODHorizonParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("Load ..."), 0 );

    bool dotrack = false;
    Settings::common().getYN(IOPar::compKey("dTect","Enable Tracking"),dotrack);
    if ( dotrack )
	mnu.insertItem( new uiMenuItem("New ..."), 1 );

    const int mnuid = mnu.exec();

    MultiID mid;
    bool success = false;
    
    if ( !mnuid )
	success = applMgr()->EMServer()->selectHorizon(mid);
    else if ( mnuid == 1 )
    {
	applMgr()->trackServer()->setSceneID( sceneID() );
	applMgr()->trackServer()->addHorizonTracker( "", true );
	return true;
    }

    if ( !success )
	return false;

    addChild( new uiODHorizonTreeItem(mid) );

    return true;
}


uiTreeItem* uiODHorizonFactory::create(int visid) const
{
    return uiVisSurface::isHorizon(visid) ? new uiODHorizonTreeItem(visid) : 0;
}


uiODHorizonTreeItem::uiODHorizonTreeItem( const MultiID& mid_ )
    : uiODEarthModelSurfaceTreeItem( mid_ )
{}


uiODHorizonTreeItem::uiODHorizonTreeItem( int id )
    : uiODEarthModelSurfaceTreeItem( 0 )
{ displayid=id; }


void uiODHorizonTreeItem::updateColumnText(int col)
{
    if ( col==1 )
    {
	BufferString shift = uivissurf->getShift();
	uilistviewitem->setText( shift, col );
	return;
    }

    return uiODDisplayTreeItem::updateColumnText(col);
}


uiODWellParentTreeItem::uiODWellParentTreeItem()
    : uiODTreeItem( "Well" )
{}


bool uiODWellParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("Add"), 0 );

    const int mnuid = mnu.exec();

    if ( mnuid<0 ) return false;

    ObjectSet<MultiID> emwellids;
    applMgr()->selectWells( emwellids );
    if ( !emwellids.size() )
	return false;

    for ( int idx=0; idx<emwellids.size(); idx++ )
	addChild( new uiODWellTreeItem(*emwellids[idx]) );

    deepErase( emwellids );
    return true;
}


uiTreeItem* uiODWellFactory::create( int visid ) const
{
    mDynamicCastGet( visSurvey::WellDisplay*, wd, 
	    	     ODMainWin()->applMgr().visServer()->getObject(visid));
    return wd ? new uiODWellTreeItem(visid) : 0;
}


uiODWellTreeItem::uiODWellTreeItem( int did )
{ displayid=did; }


uiODWellTreeItem::uiODWellTreeItem( const MultiID& mid_ )
{ mid = mid_; }


bool uiODWellTreeItem::init()
{
    uiVisPartServer* visserv = applMgr()->visServer();
    if ( displayid==-1 )
    {
	visSurvey::WellDisplay* wd = visSurvey::WellDisplay::create();
	displayid = wd->id();
	visserv->addObject(wd,sceneID(),true);
	if ( !wd->setWellId(mid) )
	{
	    visserv->removeObject(wd,sceneID());
	    return false;
	}
    }
    else
    {
	mDynamicCastGet( visSurvey::WellDisplay*, wd,
			 visserv->getObject(displayid));
	if ( !wd )
	    return false;
    }

    return uiODDisplayTreeItem::init();
}
	    
	
void uiODWellTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet( uiVisMenu*, menu, cb );

    uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet( visSurvey::WellDisplay*, wd,
	    	     visserv->getObject(displayid));

    propertiesmnuid = menu->addItem( new uiMenuItem("Properties ...") );

    uiPopupMenu* showmnu = new uiPopupMenu( menu->getParent(), "Show" );

    uiMenuItem* wellnameitem = new uiMenuItem("Well name");
    namemnuid = menu->getFreeID();
    showmnu->insertItem( wellnameitem, namemnuid );
    wellnameitem->setChecked( wd->wellNameShown() );

    uiMenuItem* markeritem = new uiMenuItem("Markers");
    markermnuid = menu->getFreeID();
    showmnu->insertItem( markeritem, markermnuid );
    markeritem->setChecked( wd->markersShown() );
    markeritem->setEnabled( wd->canShowMarkers() );

    uiMenuItem* markernameitem = new uiMenuItem("Marker names");
    markernamemnuid = menu->getFreeID();
    showmnu->insertItem( markernameitem, markernamemnuid );
    markernameitem->setChecked( wd->canShowMarkers() && wd->markerNameShown() );
    markernameitem->setEnabled( wd->canShowMarkers() );

    uiMenuItem* logsmnuitem = new uiMenuItem("Logs");
    showlogmnuid = menu->getFreeID();
    showmnu->insertItem( logsmnuitem, showlogmnuid );
    logsmnuitem->setChecked( wd->logsShown() );
    logsmnuitem->setEnabled( applMgr()->wellServer()->hasLogs(wd->wellId()) );

    menu->addSubMenu( showmnu );

    selattrmnuid = menu->addItem(  new uiMenuItem("Select Attribute ...") );
    uiMenuItem* sellogmnuitem = new uiMenuItem("Select logs ...");
    sellogmnuid = menu->addItem( sellogmnuitem );
    sellogmnuitem->setEnabled( applMgr()->wellServer()->hasLogs(wd->wellId()) );
}


void uiODWellTreeItem::handleMenuCB(CallBacker* cb)
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( uiVisMenu*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
	return;

    uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet( visSurvey::WellDisplay*, wd,
	    	     visserv->getObject(displayid));
    const MultiID& wellid = wd->wellId();
    if ( mnuid == selattrmnuid )
    {
	menu->setIsHandled(true);
	applMgr()->wellAttribServer()->setAttribSet( 
				*applMgr()->attrServer()->curDescSet() );
	applMgr()->wellAttribServer()->selectAttribute( wellid );
    }
    else if ( mnuid==sellogmnuid )
    {
	menu->setIsHandled(true);
	int selidx = -1;
	int lognr = 1;
	applMgr()->wellServer()->selectLogs( wellid, selidx, lognr );
	if ( selidx > -1 )
	    wd->displayLog(selidx,lognr,false);
    }
    else if ( mnuid == propertiesmnuid )
    {
	menu->setIsHandled(true);
	uiWellPropDlg dlg( getUiParent(), wd );
	dlg.go();
    }
    else if ( mnuid == namemnuid )
    {
	menu->setIsHandled(true);
	wd->showWellName( !wd->wellNameShown() );
    }
    else if ( mnuid == markermnuid )
    {
	menu->setIsHandled(true);
	wd->showMarkers( !wd->markersShown() );

    }
    else if ( mnuid == markernamemnuid )
    {
	menu->setIsHandled(true);
	wd->showMarkerName( !wd->markerNameShown() );
    }
    else if ( mnuid == showlogmnuid )
    {
	wd->showLogs( !wd->logsShown() );
    }
}


uiODPickSetParentTreeItem::uiODPickSetParentTreeItem()
    : uiODTreeItem( "PickSet" )
{}


bool uiODPickSetParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("New/Load ..."), 0 );
    if ( children.size() )
	mnu.insertItem( new uiMenuItem("Store ..."), 1);

    const int mnuid = mnu.exec();

    if ( mnuid<0 ) return false;

    if ( mnuid==0 )
    {
	if ( !applMgr()->pickServer()->fetchPickSets() ) return -1;
	PickSetGroup* psg = new PickSetGroup;
	applMgr()->getPickSetGroup( *psg );
	if ( psg->nrSets() )
	{
	    for ( int idx=0; idx<psg->nrSets(); idx++ )
	    {
		//TODO make sure it's not in list already
		addChild( new uiODPickSetTreeItem(psg->get(idx)) );
	    }
	}
	else
	{
	    PickSet pset( psg->name() );
	    pset.color = applMgr()->getPickColor();
	    addChild( new uiODPickSetTreeItem(&pset) );
	    //TODO create pickset
	}
    }
    else if ( mnuid==1 )
    {
	applMgr()->storePickSets();
    }

    return true;
}


uiTreeItem* uiODPickSetFactory::create( int visid ) const
{
    mDynamicCastGet( visSurvey::PickSetDisplay*, psd, 
	    	     ODMainWin()->applMgr().visServer()->getObject(visid));
    return psd ? new uiODPickSetTreeItem(visid) : 0;
}


uiODPickSetTreeItem::uiODPickSetTreeItem( const PickSet* ps_ )
    : ps( ps_ )
{}


uiODPickSetTreeItem::uiODPickSetTreeItem( int id )
{ displayid = id; }


bool uiODPickSetTreeItem::init()
{
    uiVisPartServer* visserv = applMgr()->visServer();
    if ( displayid==-1 )
    {
	visSurvey::PickSetDisplay* psd = visSurvey::PickSetDisplay::create();
	displayid = psd->id();
	psd->copyFromPickSet(*ps);
	visserv->addObject(psd,sceneID(),true);
    }
    else
    {
	mDynamicCastGet( visSurvey::PickSetDisplay*, psd,
			 visserv->getObject(displayid) );
	if ( !psd )
	    return false;
    }

    return uiODDisplayTreeItem::init();
}


void uiODPickSetTreeItem::updateColumnText( int col )
{
    if ( col==1 )
    {
	uiVisPartServer* visserv = applMgr()->visServer();
	mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
			visserv->getObject(displayid));
	BufferString text = psd->nrPicks();
	uilistviewitem->setText( text, col );
	return;
    }

    return uiODDisplayTreeItem::updateColumnText(col);
}




void uiODPickSetTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet( uiVisMenu*, menu, cb );

    renamemnuid = menu->addItem( new uiMenuItem("Rename ...") );
    storemnuid = menu->addItem( new uiMenuItem("Store ...") );
    dirmnuid = menu->addItem( new uiMenuItem("Set directions ...") );

    uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet( visSurvey::PickSetDisplay*, psd,
	    	     visserv->getObject(displayid));
    uiMenuItem* showallitem = new uiMenuItem("Show all");
    showallmnuid = menu->addItem( showallitem );
    showallitem->setChecked( psd->allShown() );

    propertymnuid = menu->addItem( new uiMenuItem("Properties ...") );
}


void uiODPickSetTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( uiVisMenu*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( mnuid==renamemnuid )
    {
	menu->setIsHandled(true);
	applMgr()->renamePickset( displayid );
    }
    else if ( mnuid==storemnuid )
    {
	menu->setIsHandled(true);
	applMgr()->storeSinglePickSet( displayid );
    }
    else if ( mnuid==dirmnuid )
    {
	menu->setIsHandled(true);
	applMgr()->setPickSetDirs( displayid );
    }
    else if ( mnuid==showallmnuid )
    {
	uiVisPartServer* visserv = applMgr()->visServer();
	mDynamicCastGet( visSurvey::PickSetDisplay*, psd,
			 visserv->getObject(displayid));
	const bool showall = !psd->allShown();
	psd->showAll( showall );
    }
    else if ( mnuid==propertymnuid )
    {
	mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
			applMgr()->visServer()->getObject(displayid))
	uiPickSizeDlg dlg( getUiParent(), psd );
	dlg.go();
    }

    updateColumnText(0);
    updateColumnText(1);
}


/*
uiODVolumeParentTreeItem::uiODVolumeParentTreeItem()
    : uiODTreeItem( "Volume" )
{}


bool uiODVolumeParentTreeItem::showSubMenu()
{
    mParentShowSubMenu( addChild(new uiODVolumeTreeItem); );
}


uiODVolumeTreeItem::uiODVolumeTreeItem( int id )
{ displayid = id; }


bool uiODVolumeTreeItem::init()
{
    mDisplayInit(uiODDisplayTreeItem,addVolView(sceneID()),
	    	 isVolView(displayid));

    TypeSet<int> subids;
    applMgr()->visServer()->getChildIds( displayid, subids );
    for ( int idx=subids.size()-1; idx>=0; idx-- )
	addChild( new uiODVolumePartTreeItem(subids[idx]) );

    uilistviewitem->setOpen(false);
    return true;
}


uiODVolumePartTreeItem::uiODVolumePartTreeItem( int id )
{ displayid = id; }


bool uiODVolumePartTreeItem::init()
{
    return uiODDisplayTreeItem::init();
}

*/


uiODPlaneDataTreeItem::uiODPlaneDataTreeItem( int did, int dim_ )
    : dim( dim_ )
{ displayid = did; }


bool uiODPlaneDataTreeItem::init()
{
    uiVisPartServer* visserv = applMgr()->visServer();
    if ( displayid==-1 )
    {
	visSurvey::PlaneDataDisplay* pdd=visSurvey::PlaneDataDisplay::create();
	displayid = pdd->id();
	pdd->setType( (visSurvey::PlaneDataDisplay::Type) dim );
	visserv->addObject( pdd, sceneID(), true );
    }
    else
    {
	mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd,
			  visserv->getObject(displayid));
	if ( !pdd ) return false;
    }

    return uiODDisplayTreeItem::init();
}


void uiODPlaneDataTreeItem::updateColumnText(int col)
{
    if ( col==1 )
    {
	uiVisPartServer* visserv = applMgr()->visServer();
	mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
			visserv->getObject(displayid));
	BufferString text = pdd->getManipulationPos();
	uilistviewitem->setText( text, col );
	return;
    }

    return uiODDisplayTreeItem::updateColumnText(col);
}


void uiODPlaneDataTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet( uiVisMenu*, menu, cb );

    positionmnuid = menu->addItem( new uiMenuItem("Position ...") );
}


void uiODPlaneDataTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( uiVisMenu*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( mnuid == positionmnuid )
    {
	menu->setIsHandled(true);
	uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
	mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
			visserv->getObject(displayid))
	uiSliceSel dlg( getUiParent(), pdd->getCubeSampling(), 
			mCB(this,uiODPlaneDataTreeItem,updatePlanePos), dim );
	if ( !dlg.go() ) return;
	CubeSampling cs = dlg.getCubeSampling();
	pdd->setCubeSampling( cs );
	visserv->calculateAttrib( displayid, false );
	visserv->calculateColorAttrib( displayid, false );
    }
}


void uiODPlaneDataTreeItem::updatePlanePos( CallBacker* cb )
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
	    	    visserv->getObject(displayid))
    mDynamicCastGet(uiSliceSel*,dlg,cb)
    if ( !dlg ) return;

    CubeSampling cs = dlg->getCubeSampling();
    pdd->setCubeSampling( cs );
    visserv->calculateAttrib( displayid, false );
    visserv->calculateColorAttrib( displayid, false );
}


uiTreeItem* uiODInlineFactory::create( int visid ) const
{
    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd, 
	    	     ODMainWin()->applMgr().visServer()->getObject(visid));
    return pdd && pdd->getType()==0 ? new uiODInlineTreeItem(visid) : 0;
}


uiODInlineParentTreeItem::uiODInlineParentTreeItem()
    : uiODTreeItem( "Inline" )
{ }


bool uiODInlineParentTreeItem::showSubMenu()
{
    mParentShowSubMenu( addChild( new uiODInlineTreeItem(-1)); );
}


uiODInlineTreeItem::uiODInlineTreeItem( int id )
    : uiODPlaneDataTreeItem( id, 0 )
{}


uiTreeItem* uiODCrosslineFactory::create(int visid) const
{
    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd, 
	    	     ODMainWin()->applMgr().visServer()->getObject(visid));
    return pdd && pdd->getType()==1 ? new uiODCrosslineTreeItem(visid) : 0;
}


uiODCrosslineParentTreeItem::uiODCrosslineParentTreeItem()
    : uiODTreeItem( "Crossline" )
{ }


bool uiODCrosslineParentTreeItem::showSubMenu()
{
    mParentShowSubMenu( addChild( new uiODCrosslineTreeItem(-1)); );
}


uiODCrosslineTreeItem::uiODCrosslineTreeItem( int id )
    : uiODPlaneDataTreeItem( id, 1 )
{}


uiTreeItem* uiODTimesliceFactory::create(int visid) const
{
    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd, 
	    	     ODMainWin()->applMgr().visServer()->getObject(visid));
    return pdd && pdd->getType()==2 ? new uiODTimesliceTreeItem(visid) : 0;
}


uiODTimesliceParentTreeItem::uiODTimesliceParentTreeItem()
    : uiODTreeItem( "Timeslice" )
{ }


bool uiODTimesliceParentTreeItem::showSubMenu()
{
    mParentShowSubMenu( addChild( new uiODTimesliceTreeItem(-1)); );
}


uiODTimesliceTreeItem::uiODTimesliceTreeItem( int id )
    : uiODPlaneDataTreeItem( id, 2 )
{}


uiODSceneTreeItem::uiODSceneTreeItem( const char* name__, int displayid_)
    : uiODTreeItem( name__ )
    , displayid( displayid_ )
{}


#define mAnnotText	0
#define mAnnotScale	1
#define mSurveyBox	2
#define mDumpIV		3
#define mSubMnuScenePr	4


bool uiODSceneTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet( visSurvey::Scene*, scene, visserv->getObject(displayid));

    const bool showcube = scene->isAnnotShown();
    uiMenuItem* anntxt = new uiMenuItem("Annotation text");
    mnu.insertItem( anntxt, mAnnotText );
    anntxt->setChecked( showcube && scene->isAnnotTextShown() );
    anntxt->setEnabled( showcube );

    uiMenuItem* annscale = new uiMenuItem("Annotation scale");
    mnu.insertItem( annscale, mAnnotScale );
    annscale->setChecked( showcube && scene->isAnnotScaleShown() );
    annscale->setEnabled( showcube );

    uiMenuItem* annsurv = new uiMenuItem("Survey box");
    mnu.insertItem( annsurv, mSurveyBox );
    annsurv->setChecked( showcube );


    bool doi = false;
    Settings::common().getYN( IOPar::compKey("dTect","Dump OI Menu"), doi );
    if ( doi )
	mnu.insertItem( new uiMenuItem("Dump OI ..."), mDumpIV );

    if ( getenv("DTECT_ENABLE_SCENE_PRINT") )
	mnu.insertItem( new uiMenuItem("'Print' to file ..."), mSubMnuScenePr );

    const int mnuid=mnu.exec();
    if ( mnuid==mAnnotText )
	scene->showAnnotText( !scene->isAnnotTextShown() );
    else if ( mnuid==mAnnotScale )
	scene->showAnnotScale( !scene->isAnnotScaleShown() );
    else if ( mnuid==mSurveyBox )
	scene->showAnnot( !scene->isAnnotShown() );
    else if ( mnuid==mDumpIV )
	visserv->dumpOI( displayid );
    else if ( mnuid==mSubMnuScenePr )
    {
	viewer()->renderToFile();
    }

    return true;
}
