/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		Jul 2003
 RCS:		$Id: uiodtreeitem.cc,v 1.89 2005-07-27 11:19:51 cvsnanne Exp $
___________________________________________________________________

-*/

#include "uiodtreeitemimpl.h"

#include "attribsel.h"
#include "errh.h"
#include "ptrman.h"
#include "ioobj.h"
#include "ioman.h"
#include "uimenu.h"
#include "pickset.h"
#include "survinfo.h"
#include "uilistview.h"
#include "uibinidtable.h"
#include "uimenuhandler.h"
#include "uisoviewer.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uimsg.h"
#include "uigeninputdlg.h"
#include "uivisemobj.h"
#include "uiempartserv.h"
#include "uiwellpropdlg.h"
#include "uivispartserv.h"
#include "uiwellpartserv.h"
#include "uipickpartserv.h"
#include "uiwellattribpartserv.h"
#include "uiattribpartserv.h"
#include "uimpepartserv.h"
#include "uiseispartserv.h"
#include "uislicesel.h"
#include "uipickszdlg.h"
#include "uicolor.h"

#include "visrandomtrackdisplay.h"
#include "vissurvwell.h"
#include "vissurvpickset.h"
#include "vissurvscene.h"
#include "visplanedatadisplay.h"
#include "uiexecutor.h"
#include "settings.h"
#include "emhorizon.h"
#include "emfault.h"


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
    setPropertyPtr( viewerptr, sovwr );
    setPropertyPtr( applmgrstr, am );

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
    void* res = 0;
    getPropertyPtr( applmgrstr, res );
    return reinterpret_cast<uiODApplMgr*>( res );
}


// ***** uiODTreeItem

uiODTreeItem::uiODTreeItem( const char* name__ )
    : uiTreeItem( name__ )
{}


uiODApplMgr* uiODTreeItem::applMgr()
{
    void* res = 0;
    getPropertyPtr( uiODTreeTop::applmgrstr, res );
    return reinterpret_cast<uiODApplMgr*>( res );
}


uiSoViewer* uiODTreeItem::viewer()
{
    void* res = 0;
    getPropertyPtr( uiODTreeTop::viewerptr, res );
    return reinterpret_cast<uiSoViewer*>( res );
}


int uiODTreeItem::sceneID() const
{
    int sceneid=-1;
    getProperty<int>( uiODTreeTop::sceneidkey, sceneid );
    return sceneid;
}


void uiODTreeItem::addStandardItems( uiPopupMenu& mnu )
{
#ifdef __debug__
    if ( !children.size() ) return;

    mnu.insertSeparator( 100 );
    mnu.insertItem( new uiMenuItem("Show all"), 101 );
    mnu.insertItem( new uiMenuItem("Hide all"), 102 );
    mnu.insertItem( new uiMenuItem("Remove all"), 103 );
#endif
}


void uiODTreeItem::handleStandardItems( int mnuid )
{
#ifdef __debug__
    for ( int idx=0; idx<children.size(); idx++ )
    {
	if ( mnuid == 101 )
	    children[idx]->setChecked( true );
	else if ( mnuid == 102 )
	    children[idx]->setChecked( false );
    }

    if ( mnuid==103 )
    {
	BufferString msg( "All " ); msg += name(); 
	msg += " items will be removed.\nDo you want to continue?";
	if ( !uiMSG().askGoOn(msg) ) return;

	while ( children.size() )
	{
	    mDynamicCastGet(uiODDisplayTreeItem*,itm,children[0])
	    if ( !itm ) continue;
	    applMgr()->visServer()->removeObject( itm->displayID(), sceneID() );
	    removeChild( itm );
	}
    }
#endif
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
    addStandardItems( mnu ); \
    const int mnuid = mnu.exec(); \
    if ( mnuid==0 ) \
	creation; \
    handleStandardItems( mnuid ); \
 \
    return true; \


bool uiODDisplayTreeItem::create( uiTreeItem* treeitem, uiODApplMgr* appl,
				  int displayid )
{
    const uiTreeFactorySet* tfs = ODMainWin()->sceneMgr().treeItemFactorySet();
    if ( !tfs )
	return false;

    for ( int idx=0; idx<tfs->nrFactories(); idx++ )
    {
	mDynamicCastGet(const uiODTreeItemFactory*,itmcreater,
			tfs->getFactory(idx))
	if ( !itmcreater ) continue;

	uiTreeItem* res = itmcreater->create( displayid, treeitem );
	if ( res )
	{
	    treeitem->addChild( res );
	    return true;
	}
    }

    return false;
}

	

uiODDisplayTreeItem::uiODDisplayTreeItem()
    : uiODTreeItem(0)
    , displayid(-1)
    , visserv(ODMainWin()->applMgr().visServer())
    , selattrmnuitem(attrselmnutxt,9999)
    , duplicatemnuitem("Duplicate")
    , removemnuitem("Remove",-1000)
{
}


uiODDisplayTreeItem::~uiODDisplayTreeItem()
{
    uiMenuHandler* menu = visserv->getMenu( displayid, false );
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

    visserv->setSelObjectId( displayid );
    uilistviewitem->setChecked( visserv->isOn(displayid) );
    uilistviewitem->stateChanged.notify( mCB(this,uiODDisplayTreeItem,checkCB));

    name_ = createDisplayName();

    uiMenuHandler* menu = visserv->getMenu( displayid, true );
    menu->createnotifier.notify( mCB(this,uiODDisplayTreeItem,createMenuCB) );
    menu->handlenotifier.notify( mCB(this,uiODDisplayTreeItem,handleMenuCB) );

    return true;
}


void uiODDisplayTreeItem::updateColumnText( int col )
{
    if ( !col )
	name_ = createDisplayName();

    uiTreeItem::updateColumnText(col);
}


bool uiODDisplayTreeItem::showSubMenu()
{
    return applMgr()->visServer()->showMenu( displayid, 
	    				     uiMenuHandler::fromTree );
}


void uiODDisplayTreeItem::checkCB( CallBacker* )
{
    applMgr()->visServer()->turnOn( displayid, uilistviewitem->isChecked() );
}


int uiODDisplayTreeItem::uiListViewItemType() const
{
    return uiListViewItem::CheckBox;
}


BufferString uiODDisplayTreeItem::createDisplayName() const
{
    const uiVisPartServer* cvisserv = const_cast<uiODDisplayTreeItem*>(this)->
							applMgr()->visServer();
    const AttribSelSpec* as = cvisserv->getSelSpec( displayid );
    BufferString dispname( as ? as->userRef() : 0 );
    if ( as && as->isNLA() )
    {
	dispname = as->objectRef();
	const char* nodenm = as->userRef();
	if ( IOObj::isKey(as->userRef()) )
	    nodenm = IOM().nameOf( as->userRef(), false );
	dispname += " ("; dispname += nodenm; dispname += ")";
    }

    if ( as && as->id()==AttribSelSpec::attribNotSel )
	dispname = "<right-click>";
    else if ( !as )
	dispname = cvisserv->getObjectName(displayid);
    else if ( as->id() == AttribSelSpec::noAttrib )
	dispname = "";

    return dispname;
}


const char* uiODDisplayTreeItem::attrselmnutxt = "Select Attribute";


void uiODDisplayTreeItem::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);

    selattrmnuitem.removeItems();
    if ( visserv->hasAttrib(displayid) )
    {
	const AttribSelSpec* as = visserv->getSelSpec(displayid);
	uiAttribPartServer* attrserv = applMgr()->attrServer();
	MenuItem* subitem = attrserv->storedAttribMenuItem( *as );
	mAddMenuItem( &selattrmnuitem, subitem, subitem->nrItems(),
		       subitem->checked );

	subitem = attrserv->calcAttribMenuItem( *as );
	mAddMenuItem( &selattrmnuitem, subitem, subitem->nrItems(),
			 subitem->checked );

	subitem = attrserv->nlaAttribMenuItem( *as );
	if ( subitem && subitem->nrItems() )
	    mAddMenuItem( &selattrmnuitem, subitem, true, subitem->checked );

	mAddMenuItem( menu, &selattrmnuitem, true, false );
    }

    mAddMenuItemCond( menu, &duplicatemnuitem, true, false,
	    	      visserv->canDuplicate(displayid) );

    mAddMenuItem( menu, &removemnuitem, true, false );
}


void uiODDisplayTreeItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() ) return;
    if ( mnuid==duplicatemnuitem.id )
    {
	menu->setIsHandled(true);
	int newid =visserv->duplicateObject(displayid,sceneID());
	if ( newid!=-1 )
	    uiODDisplayTreeItem::create( this, applMgr(), newid );
    }
    else if ( mnuid==removemnuitem.id )
    {
	menu->setIsHandled(true);
	prepareForShutdown();
	visserv->removeObject( displayid, sceneID() );
	parent->removeChild( this );
    }
    else 
    {
	const AttribSelSpec* as = visserv->getSelSpec( displayid );
	if ( !as ) return;
	AttribSelSpec myas( *as );
	if ( applMgr()->attrServer()->handleAttribSubMenu(mnuid,myas) )
	{
	    menu->setIsHandled(true);
	    visserv->setSelSpec( displayid, myas );
	    visserv->resetColorDataType( displayid );
	    visserv->calculateAttrib( displayid, false );
	    updateColumnText(0);
	}
    }
}


uiODEarthModelSurfaceTreeItem::uiODEarthModelSurfaceTreeItem(
						const MultiID& mid_ )
    : mid(mid_)
    , uivisemobj(0)
    , depthattribmnuitem("Depth")
    , savemenuitem("Save")
    , enabletrackingmnuitem("Enable tracking")
    , changesetupmnuitem("Change setup ...")
    , addsectionmnuitem("Add section ...")
    , extendsectionmnuitem("Extend section ...")
    , reloadmenuitem("Reload")
    , relationsmnuitem("Relations ...")
    , savesurfacedatamnuitem("Save attribute ...")
    , loadsurfacedatamnuitem("Surface data ...")
    , starttrackmnuitem("Start tracking ...")

{}


uiODEarthModelSurfaceTreeItem::~uiODEarthModelSurfaceTreeItem()
{ 
    delete uivisemobj;
}


#define mDelRet { delete uivisemobj; uivisemobj = 0; return false; }


bool uiODEarthModelSurfaceTreeItem::init()
{
    delete uivisemobj;
    if ( displayid!=-1 )
    {
	uivisemobj = new uiVisEMObject( getUiParent(), displayid, visserv );
	if ( !uivisemobj->isOK() )
	    mDelRet;

	mid = *visserv->getMultiID(displayid);
    }
    else
    {
	uivisemobj = new uiVisEMObject( getUiParent(), mid, sceneID(), visserv);
	displayid = uivisemobj->id();
	if ( !uivisemobj->isOK() )
	    mDelRet;
    }

    if ( !uiODDisplayTreeItem::init() )
	return false; 

    return true;
}


void uiODEarthModelSurfaceTreeItem::checkCB( CallBacker* cb )
{
    uiODDisplayTreeItem::checkCB(cb);

    const int trackerid = applMgr()->mpeServer()->getTrackerID(mid);
    if ( trackerid==-1 )
    {
	prevtrackstatus = false;
        return;
    }

    if ( uilistviewitem->isChecked() )
	applMgr()->mpeServer()->enableTracking(trackerid, prevtrackstatus);
    else
    {
	prevtrackstatus = applMgr()->mpeServer()->isTrackingEnabled(trackerid);
	applMgr()->mpeServer()->enableTracking(trackerid,false);
    }
}


void uiODEarthModelSurfaceTreeItem::prepareForShutdown()
{
    uivisemobj->prepareForShutdown();
}


BufferString uiODEarthModelSurfaceTreeItem::createDisplayName() const
{
    const uiVisPartServer* cvisserv =
       const_cast<uiODEarthModelSurfaceTreeItem*>(this)->applMgr()->visServer();
    const AttribSelSpec* as = cvisserv->getSelSpec( displayid );
    bool hasattr = as && as->id() > -2;
    BufferString dispname;
    if ( hasattr )
    {
	dispname = uiODDisplayTreeItem::createDisplayName();
	dispname += " (";
    }
    dispname += cvisserv->getObjectName( displayid );
    if ( hasattr ) dispname += ")";
    return dispname;
}


#define mIsObject(typestr) \
    !strcmp(uivisemobj->getObjectType(displayid),typestr)

void uiODEarthModelSurfaceTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb);

    const AttribSelSpec* as = visserv->getSelSpec(displayid);

    mAddMenuItem( &selattrmnuitem, &loadsurfacedatamnuitem, true, false );
    mAddMenuItem( &selattrmnuitem, &depthattribmnuitem, true,
	    	  as->id()==AttribSelSpec::noAttrib );

    MenuItem* trackmnu = menu->findItem(uiVisEMObject::trackingmenutxt);
    if ( uilistviewitem->isChecked() && trackmnu )
    {
	uiMPEPartServer* mps = applMgr()->mpeServer();
	mps->setCurrentAttribDescSet( applMgr()->attrServer()->curDescSet() );

	EM::SectionID section = -1;
	if ( uivisemobj->nrSections()==1 )
	    section = uivisemobj->getSectionID(0);
	else if ( menu->getPath() )
	    section = uivisemobj->getSectionID( menu->getPath() );

	const Coord3& pickedpos = menu->getPickedPos();
	const bool hastracker = applMgr()->mpeServer()->getTrackerID(mid)>=0;

	if (  pickedpos.isDefined() && !hastracker &&
		applMgr()->EMServer()->isFullResolution(mid) )
	{
	    mAddMenuItem( trackmnu, &starttrackmnuitem, true, false );

	    mResetMenuItem( &addsectionmnuitem );
	    mResetMenuItem( &extendsectionmnuitem );
	    mResetMenuItem( &addsectionmnuitem );
	    mResetMenuItem( &extendsectionmnuitem );
	    mResetMenuItem( &changesetupmnuitem );
	    mResetMenuItem( &enabletrackingmnuitem );
	    mResetMenuItem( &relationsmnuitem );
	}
	else if ( hastracker && section != -1 )
	{
	    mResetMenuItem( &starttrackmnuitem );

#ifdef __debug__
	    mAddMenuItemCond( trackmnu, &addsectionmnuitem, true, false,
		    		mIsObject(EM::Horizon::typeStr()) );
	    mAddMenuItem( trackmnu, &extendsectionmnuitem, true, false );
#else
	    mResetMenuItem( &addsectionmnuitem );
	    mResetMenuItem( &extendsectionmnuitem );
#endif

	    mAddMenuItem( trackmnu, &changesetupmnuitem, true, false );
	    mAddMenuItem( trackmnu, &enabletrackingmnuitem, true,
		   applMgr()->mpeServer()->isTrackingEnabled(
		      applMgr()->mpeServer()->getTrackerID(mid)) );
	    mAddMenuItem( trackmnu, &relationsmnuitem,
		    mIsObject(EM::Horizon::typeStr()), false );
	}

    }
    else
    {
	mResetMenuItem( &starttrackmnuitem );
	mResetMenuItem( &addsectionmnuitem );
	mResetMenuItem( &extendsectionmnuitem );
	mResetMenuItem( &addsectionmnuitem );
	mResetMenuItem( &extendsectionmnuitem );
	mResetMenuItem( &changesetupmnuitem );
	mResetMenuItem( &enabletrackingmnuitem );
	mResetMenuItem( &relationsmnuitem );
    }

    mAddMenuItem( menu, &savemenuitem,
		  applMgr()->EMServer()->isChanged(mid), false );

    mAddMenuItem( menu, &savesurfacedatamnuitem,
		  as && as->id() >= 0, false );

#ifdef __debug__
    mAddMenuItem( menu, &reloadmenuitem, true, false );
#else
    mResetMenuItem( &reloadmenuitem );
#endif
}


void uiODEarthModelSurfaceTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    EM::SectionID sectionid = -1;
    if ( uivisemobj->nrSections()==1 )
	sectionid = uivisemobj->getSectionID(0);
    else if ( menu->getPath() )
	sectionid = uivisemobj->getSectionID( menu->getPath() );

    if ( mnuid==savesurfacedatamnuitem.id )
    {
	menu->setIsHandled(true);
	applMgr()->EMServer()->storeAuxData( mid, true );
    }
    else if ( mnuid==savemenuitem.id )
    {
	menu->setIsHandled(true);
	applMgr()->EMServer()->storeObject( mid, false );
    }
    else if ( mnuid==starttrackmnuitem.id )
    {
	menu->setIsHandled(true);
	if ( sectionid < 0 ) return;

	applMgr()->mpeServer()->addTracker( mid, menu->getPickedPos() );
    }
    else if ( mnuid==changesetupmnuitem.id )
    {
	menu->setIsHandled(true);
	applMgr()->mpeServer()->showSetupDlg( mid, sectionid );
    }
    else if ( mnuid==reloadmenuitem.id )
    {
	menu->setIsHandled(true);
	uiTreeItem* parent__ = parent;

	applMgr()->visServer()->removeObject( displayid, sceneID() );
	delete uivisemobj; uivisemobj = 0;

	if ( !applMgr()->EMServer()->loadSurface(mid) )
	    return;

	uivisemobj = new uiVisEMObject( getUiParent(), mid, sceneID(), visserv);
	displayid = uivisemobj->id();
    }
    else if ( mnuid==depthattribmnuitem.id )
    {
	menu->setIsHandled(true);
	uivisemobj->setDepthAsAttrib();
	updateColumnText(0);
    }
    else if ( mnuid==relationsmnuitem.id )
    {	
	menu->setIsHandled(true);
	applMgr()->mpeServer()->showRelationsDlg( mid, sectionid );
    }
    else if ( mnuid==enabletrackingmnuitem.id )
    {
	menu->setIsHandled(true);
	const int trackerid = applMgr()->mpeServer()->getTrackerID(mid);
	applMgr()->mpeServer()->enableTracking(trackerid,
		!applMgr()->mpeServer()->isTrackingEnabled(trackerid));
    }
    else if ( mnuid==addsectionmnuitem.id )
    {
	menu->setIsHandled(true);
	//applMgr()->mpeServer()->addNewSection( mid );
    }
    else if ( mnuid==extendsectionmnuitem.id )
    {
	menu->setIsHandled(true);
	//applMgr()->mpeServer()->extendSection( mid, sectionid );
    }
    else if ( mnuid==loadsurfacedatamnuitem.id )
    {
	menu->setIsHandled(true);
	const bool res = applMgr()->EMServer()->loadAuxData(mid);
	if ( !res ) return;
	uivisemobj->readAuxData();
	visserv->selectTexture( displayid, 0 );
	ODMainWin()->sceneMgr().updateTrees();
    }
}


uiTreeItem* uiODBodyTreeItemFactory::create( int visid, uiTreeItem*) const
{
    const char* objtype = uiVisEMObject::getObjectType(visid);
    return objtype && !strcmp(objtype, "Horizontal Tube")
	? new uiODBodyTreeItem(visid) : 0;
}


uiODBodyParentTreeItem::uiODBodyParentTreeItem()
    : uiODTreeItem("Bodies" )
{}



bool uiODBodyParentTreeItem::showSubMenu()
{
    /*
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

    */
    return true;
}


uiODBodyTreeItem::uiODBodyTreeItem( int displayid_ )
    : uivisemobj(0)
    , prevtrackstatus( false )
{ displayid = displayid_; }


uiODBodyTreeItem::uiODBodyTreeItem( const MultiID& mid_ )
    : mid(mid_)
    , uivisemobj(0)
    , prevtrackstatus( false )
{}


uiODBodyTreeItem::~uiODBodyTreeItem()
{ 
    delete uivisemobj;
}


bool uiODBodyTreeItem::init()
{
    delete uivisemobj;
    if ( displayid!=-1 )
    {
	uivisemobj = new uiVisEMObject( getUiParent(), displayid,
					applMgr()->visServer() );
	if ( !uivisemobj->isOK() ) { delete uivisemobj; return false; }

	mid = *applMgr()->visServer()->getMultiID(displayid);
    }
    else
    {
	uivisemobj = new uiVisEMObject( getUiParent(), mid, sceneID(),
					applMgr()->visServer() );
	if ( !uivisemobj->isOK() ) { delete uivisemobj; return false; }
	displayid = uivisemobj->id();
    }

    if ( !uiODDisplayTreeItem::init() )
	return false; 

    return true;
}


void uiODBodyTreeItem::checkCB( CallBacker* cb )
{
    uiODDisplayTreeItem::checkCB(cb);

    const int trackerid = applMgr()->mpeServer()->getTrackerID(mid);
    if ( trackerid==-1 )
    {
	prevtrackstatus = false;
        return;
    }

    if ( uilistviewitem->isChecked() )
	applMgr()->mpeServer()->enableTracking(trackerid, prevtrackstatus);
    else
    {
	prevtrackstatus = applMgr()->mpeServer()->isTrackingEnabled(trackerid);
	applMgr()->mpeServer()->enableTracking(trackerid,false);
    }
}


void uiODBodyTreeItem::prepareForShutdown()
{
    uivisemobj->prepareForShutdown();
}


void uiODBodyTreeItem::createMenuCB(CallBacker*)
{
}


void uiODBodyTreeItem::handleMenuCB(CallBacker*)
{
}


BufferString uiODBodyTreeItem::createDisplayName() const
{
    const uiVisPartServer* visserv =
       const_cast<uiODBodyTreeItem*>(this)->applMgr()->visServer();
    const AttribSelSpec* as = visserv->getSelSpec( displayid );
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



uiTreeItem* uiODRandomLineTreeItemFactory::create( int visid, uiTreeItem*) const
{
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd, 
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
    : editnodesmnuitem("Edit nodes ...")
    , insertnodemnuitem("Insert node")
{ displayid = id; } 


bool uiODRandomLineTreeItem::init()
{
    if ( displayid==-1 )
    {
	visSurvey::RandomTrackDisplay* rtd =
				    visSurvey::RandomTrackDisplay::create();
	displayid = rtd->id();
	visserv->addObject( rtd, sceneID(), true );
    }
    else
    {
	mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
			visserv->getObject(displayid));
	if ( !rtd ) return false;
    }

    return uiODDisplayTreeItem::init();
}


void uiODRandomLineTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb);

    mAddMenuItem( menu, &editnodesmnuitem, true, false );
    mAddMenuItem( menu, &insertnodemnuitem, true, false );

    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
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

	mAddManagedMenuItem(&insertnodemnuitem,new MenuItem(nodename), 
			     rtd->canAddKnot(idx), false );
    }
}


void uiODRandomLineTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;
	
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
	    	    visserv->getObject(displayid));

    if ( mnuid==editnodesmnuitem.id )
    {
	editNodes();
	menu->setIsHandled(true);
    }
    else if ( insertnodemnuitem.itemIndex(mnuid)!=-1 )
    {
	menu->setIsHandled(true);
	rtd->addKnot(insertnodemnuitem.itemIndex(mnuid));
    }
}


void uiODRandomLineTreeItem::editNodes()
{
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
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
	    {
		rtd->setKnotPos( idx, bid );
		rtd->setManipKnotPos( idx, bid );
	    }
	    else
		rtd->addKnot( bid );
	}

	visserv->setSelObjectId( rtd->id() );
	visserv->calculateAttrib( rtd->id(), false );
	visserv->calculateColorAttrib( rtd->id(), false );
	ODMainWin()->sceneMgr().updateTrees();
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
    mnu.insertItem( new uiMenuItem("New ..."), 1 );
    addStandardItems( mnu );

    MultiID mid;
    bool addflt = false;

    const int mnuid = mnu.exec();
    if ( mnuid == 0 )
	addflt = applMgr()->EMServer()->selectFault(mid);
    else if ( mnuid == 1 )
    {
	uiMPEPartServer* mps = applMgr()->mpeServer();
	mps->setCurrentAttribDescSet( applMgr()->attrServer()->curDescSet() );
	mps->startWizard( EM::Fault::typeStr(), 0 );
	return true;
    }
    else
	handleStandardItems( mnuid );

    if ( addflt )
	addChild( new uiODFaultTreeItem(mid) );

    return true;
}


uiTreeItem* uiODFaultTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    const char* objtype = uiVisEMObject::getObjectType(visid);
    return objtype && !strcmp(objtype, "Fault")
	? new uiODFaultTreeItem(visid) : 0;
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
    mnu.insertItem( new uiMenuItem("New ..."), 1 );
    addStandardItems( mnu );

    MultiID mid;
    bool addhor = false;

    const int mnuid = mnu.exec();
    if ( mnuid == 0 )
	addhor = applMgr()->EMServer()->selectHorizon(mid);
    else if ( mnuid == 1 )
    {
	uiMPEPartServer* mps = applMgr()->mpeServer();
	mps->setCurrentAttribDescSet( applMgr()->attrServer()->curDescSet() );
	mps->startWizard( EM::Horizon::typeStr(), 0 );
	return true;
    }
    else
	handleStandardItems( mnuid );

    if ( addhor )
	addChild( new uiODHorizonTreeItem(mid) );

    return true;
}


uiTreeItem* uiODHorizonTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    const char* objtype = uiVisEMObject::getObjectType(visid);
    return objtype && !strcmp(objtype, "Horizon")
	? new uiODHorizonTreeItem(visid) : 0;
}


uiODHorizonTreeItem::uiODHorizonTreeItem( const MultiID& mid_ )
    : uiODEarthModelSurfaceTreeItem( mid_ )
{}


uiODHorizonTreeItem::uiODHorizonTreeItem( int id )
    : uiODEarthModelSurfaceTreeItem( 0 )
{ displayid=id; }


void uiODHorizonTreeItem::updateColumnText( int col )
{
    if ( col==1 )
    {
	BufferString shift = uivisemobj->getShift();
	uilistviewitem->setText( shift, col );
	return;
    }

    return uiODDisplayTreeItem::updateColumnText( col );
}


uiODWellParentTreeItem::uiODWellParentTreeItem()
    : uiODTreeItem( "Well" )
{}


bool uiODWellParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("Add"), 0 );

    if ( children.size() )
	mnu.insertItem( new uiMenuItem("Properties ..."), 1 );
    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid<0 ) return false;
    if ( mnuid == 0 )
    {
	ObjectSet<MultiID> emwellids;
	applMgr()->selectWells( emwellids );
	if ( !emwellids.size() )
	    return false;

	for ( int idx=0; idx<emwellids.size(); idx++ )
	    addChild( new uiODWellTreeItem(*emwellids[idx]) );

	deepErase( emwellids );
    }
    else if ( mnuid == 1 )
    {
	uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
	TypeSet<int> wdids;
	visserv->findObject( typeid(visSurvey::WellDisplay), wdids );
	ObjectSet<visSurvey::WellDisplay> wds;
	for ( int idx=0; idx<wdids.size(); idx++ )
	{
	    mDynamicCastGet(visSurvey::WellDisplay*,wd,
		    	    visserv->getObject(wdids[idx]))
	    wds += wd;
	}

	if ( !wds.size() ) return false;
	uiWellPropDlg dlg( getUiParent(), wds );
	dlg.go();
    }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem* uiODWellTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::WellDisplay*,wd, 
	    	    ODMainWin()->applMgr().visServer()->getObject(visid));
    return wd ? new uiODWellTreeItem(visid) : 0;
}


uiODWellTreeItem::uiODWellTreeItem( int did )
    : selattrmnuitem("Select Attribute ...")
    , sellogmnuitem("Select logs ...")
    , propertiesmnuitem("Properties ...")
    , namemnuitem("Well name")
    , markermnuitem("Markers")
    , markernamemnuitem("Marker names")
    , showlogmnuitem( "Logs" )
    , showmnuitem( "Show" )
{ displayid=did; }


uiODWellTreeItem::uiODWellTreeItem( const MultiID& mid_ )
    : selattrmnuitem("Select Attribute ...")
    , sellogmnuitem("Select logs ...")
    , propertiesmnuitem("Properties ...")
    , namemnuitem("Well name")
    , markermnuitem("Markers")
    , markernamemnuitem("Marker names")
    , showlogmnuitem( "Logs" )
    , showmnuitem( "Show" )
{
    mid = mid_;
}


bool uiODWellTreeItem::init()
{
    if ( displayid==-1 )
    {
	visSurvey::WellDisplay* wd = visSurvey::WellDisplay::create();
	displayid = wd->id();
	visserv->addObject( wd, sceneID(), true );
	if ( !wd->setWellId(mid) )
	{
	    visserv->removeObject( wd, sceneID() );
	    return false;
	}
    }
    else
    {
	mDynamicCastGet(visSurvey::WellDisplay*,wd,
			visserv->getObject(displayid));
	if ( !wd )
	    return false;
    }

    return uiODDisplayTreeItem::init();
}
	    
	
void uiODWellTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    mDynamicCastGet(visSurvey::WellDisplay*,wd,visserv->getObject(displayid));

    mAddMenuItem( menu, &sellogmnuitem,
	          applMgr()->wellServer()->hasLogs(wd->wellId()), false );
    mAddMenuItem( menu, &selattrmnuitem, true, false );
    mAddMenuItem( menu, &propertiesmnuitem, true, false );
    mAddMenuItem( menu, &showmnuitem, true, false );
    mAddMenuItem( &showmnuitem, &namemnuitem, true,  wd->wellNameShown() );
    mAddMenuItem( &showmnuitem, &markermnuitem, wd->canShowMarkers(),
	         wd->markersShown() );
    mAddMenuItem( &showmnuitem, &markernamemnuitem, wd->canShowMarkers(),
	          wd->canShowMarkers() && wd->markerNameShown() );
    mAddMenuItem( &showmnuitem, &showlogmnuitem,
	          applMgr()->wellServer()->hasLogs(wd->wellId()), 
		  wd->logsShown() );
}


void uiODWellTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    mDynamicCastGet(visSurvey::WellDisplay*,wd,visserv->getObject(displayid))
    const MultiID& wellid = wd->wellId();
    if ( mnuid == selattrmnuitem.id )
    {
	menu->setIsHandled(true);
	applMgr()->wellAttribServer()->setAttribSet( 
				*applMgr()->attrServer()->curDescSet() );
	applMgr()->wellAttribServer()->selectAttribute( wellid );
    }
    else if ( mnuid==sellogmnuitem.id )
    {
	menu->setIsHandled(true);
	int selidx = -1;
	int lognr = 1;
	applMgr()->wellServer()->selectLogs( wellid, selidx, lognr );
	if ( selidx > -1 )
	    wd->displayLog(selidx,lognr,false);
    }
    else if ( mnuid == propertiesmnuitem.id )
    {
	menu->setIsHandled(true);
	uiWellPropDlg dlg( getUiParent(), wd );
	dlg.go();
    }
    else if ( mnuid == namemnuitem.id )
    {
	menu->setIsHandled(true);
	wd->showWellName( !wd->wellNameShown() );
    }
    else if ( mnuid == markermnuitem.id )
    {
	menu->setIsHandled(true);
	wd->showMarkers( !wd->markersShown() );

    }
    else if ( mnuid == markernamemnuitem.id )
    {
	menu->setIsHandled(true);
	wd->showMarkerName( !wd->markerNameShown() );
    }
    else if ( mnuid == showlogmnuitem.id )
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
    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid<0 ) return false;
    if ( mnuid==0 )
    {
	if ( !applMgr()->pickServer()->fetchPickSets() ) return -1;
	PickSetGroup& psg = applMgr()->pickServer()->group();
	if ( psg.nrSets() )
	{
	    for ( int idx=0; idx<psg.nrSets(); idx++ )
	    {
		//TODO make sure it's not in list already
		const PickSet* ps = psg.get( idx );
		if ( ps )
		    addChild( new uiODPickSetTreeItem(*ps) );
	    }
	}
	else
	{
	    PickSet pset( psg.name() );
	    pset.color = applMgr()->getPickColor();
	    addChild( new uiODPickSetTreeItem(pset) );
	}
    }
    else if ( mnuid==1 )
    {
	applMgr()->storePickSets();
    }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem* uiODPickSetTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    mDynamicCastGet( visSurvey::PickSetDisplay*, psd, 
	    	     ODMainWin()->applMgr().visServer()->getObject(visid));
    return psd ? new uiODPickSetTreeItem(visid) : 0;
}


uiODPickSetTreeItem::uiODPickSetTreeItem( const PickSet& ps )
    : ps_(new PickSet(ps))
    , renamemnuitem("Rename ...")
    , storemnuitem("Store ...")
    , dirmnuitem("Set directions ...")
    , showallmnuitem("Show all")
    , propertymnuitem("Properties ...")
{}


uiODPickSetTreeItem::uiODPickSetTreeItem( int id )
    : ps_(0)
    , renamemnuitem("Rename ...")
    , storemnuitem("Store ...")
    , dirmnuitem("Set directions ...")
    , showallmnuitem("Show all")
    , propertymnuitem("Properties ...")
{ displayid = id; }


uiODPickSetTreeItem::~uiODPickSetTreeItem()
{ delete ps_; }


bool uiODPickSetTreeItem::init()
{
    if ( displayid==-1 )
    {
	visSurvey::PickSetDisplay* psd = visSurvey::PickSetDisplay::create();
	displayid = psd->id();
	psd->copyFromPickSet( *ps_ );
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
    mDynamicCastGet( uiMenuHandler*, menu, cb );

    mAddMenuItem( menu, &renamemnuitem, true, false );
    mAddMenuItem( menu, &storemnuitem, true, false );
    mAddMenuItem( menu, &dirmnuitem, true, false );

    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
	    	    visserv->getObject(displayid));

    mAddMenuItem( menu, &showallmnuitem, true, psd->allShown() );
    mAddMenuItem( menu, &propertymnuitem, true, false );
}


void uiODPickSetTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( uiMenuHandler*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( mnuid==renamemnuitem.id )
    {
	menu->setIsHandled(true);
	BufferString newname;
	const char* oldname = visserv->getObjectName( displayid );
	applMgr()->pickServer()->renamePickset( oldname, newname );
	visserv->setObjectName( displayid, newname );
    }
    else if ( mnuid==storemnuitem.id )
    {
	menu->setIsHandled(true);
	mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
			visserv->getObject(displayid));
	PickSet* ps = new PickSet( psd->name() );
	psd->copyToPickSet( *ps );
	applMgr()->pickServer()->storeSinglePickSet( ps );
    }
    else if ( mnuid==dirmnuitem.id )
    {
	menu->setIsHandled(true);
	applMgr()->setPickSetDirs( displayid );
    }
    else if ( mnuid==showallmnuitem.id )
    {
	mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
			visserv->getObject(displayid));
	const bool showall = !psd->allShown();
	psd->showAll( showall );
	mDynamicCastGet( visSurvey::Scene*,scene,visserv->getObject(sceneID()));
	scene->objectMoved(0);
    }
    else if ( mnuid==propertymnuitem.id )
    {
	mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
			applMgr()->visServer()->getObject(displayid))
	uiPickSizeDlg dlg( getUiParent(), psd );
	dlg.go();
    }

    updateColumnText(0);
    updateColumnText(1);
}



uiODPlaneDataTreeItem::uiODPlaneDataTreeItem( int did, int dim_ )
    : dim(dim_)
    , positiondlg(0)
    , positionmnuitem("Position ...")
{ displayid = did; }


uiODPlaneDataTreeItem::~uiODPlaneDataTreeItem()
{
    delete positiondlg;
}


bool uiODPlaneDataTreeItem::init()
{
    if ( displayid==-1 )
    {
	visSurvey::PlaneDataDisplay* pdd=visSurvey::PlaneDataDisplay::create();
	displayid = pdd->id();
	pdd->setType( (visSurvey::PlaneDataDisplay::Type) dim );
	visserv->addObject( pdd, sceneID(), true );
    }
    else
    {
	mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
			visserv->getObject(displayid));
	if ( !pdd ) return false;
    }

    return uiODDisplayTreeItem::init();
}


void uiODPlaneDataTreeItem::updateColumnText( int col )
{
    if ( col==1 )
    {
	mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
			visserv->getObject(displayid))
	BufferString text = pdd->getManipulationPos();
	uilistviewitem->setText( text, col );
	return;
    }

    return uiODDisplayTreeItem::updateColumnText(col);
}


void uiODPlaneDataTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb);

    mAddMenuItem( menu, &positionmnuitem, true, false );

    uiSeisPartServer* seisserv = applMgr()->seisServer();
    int type = menu->getMenuType();
    if ( type==uiMenuHandler::fromScene )
    {
	MenuItem* displaygathermnu = seisserv->storedGathersSubMenu();
	if ( displaygathermnu )
	{
	    mAddMenuItem( menu, displaygathermnu, displaygathermnu->nrItems(),
		         false );
	    displaygathermnu->placement = -500;
	}
    }
}


void uiODPlaneDataTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( mnuid == positionmnuitem.id )
    {
	menu->setIsHandled(true);
	mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
			visserv->getObject(displayid))
	delete positiondlg;
	const CubeSampling& sics = SI().sampling();
	positiondlg = new uiSliceSel( getUiParent(), pdd->getCubeSampling(),
				SI().sampling(),
				mCB(this,uiODPlaneDataTreeItem,updatePlanePos), 
				(uiSliceSel::Type)dim );
	positiondlg->windowClosed.notify( 
		mCB(this,uiODPlaneDataTreeItem,posDlgClosed) );
	positiondlg->go();
	applMgr()->enableSceneMenu( false );
    }
    else
    {
	uiSeisPartServer* seisserv = applMgr()->seisServer();
	MenuItem* gathermnuitem = seisserv->storedGathersSubMenu();
	if ( gathermnuitem && gathermnuitem->itemIndex(mnuid)!=-1 )
	{
	    menu->setIsHandled(true);
	    const Coord3 inlcrlpos = visserv->getMousePos(false);
	    const BinID bid( (int)inlcrlpos.x, (int)inlcrlpos.y );
	    seisserv->handleGatherSubMenu( mnuid,bid);
	}
    }
}


void uiODPlaneDataTreeItem::posDlgClosed( CallBacker* )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
	    	    visserv->getObject(displayid))
    CubeSampling newcs = positiondlg->getCubeSampling();
    bool samepos = newcs == pdd->getCubeSampling();
    if ( positiondlg->uiResult() && !samepos )
    {
	pdd->setCubeSampling( newcs );
	pdd->resetManipulation();
	visserv->calculateAttrib( displayid, false );
	visserv->calculateColorAttrib( displayid, false );
	updateColumnText(0);
    }

    applMgr()->enableSceneMenu( true );
}


void uiODPlaneDataTreeItem::updatePlanePos( CallBacker* cb )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
	    	    visserv->getObject(displayid))
    mDynamicCastGet(uiSliceSel*,dlg,cb)
    if ( !dlg ) return;

    CubeSampling cs = dlg->getCubeSampling();
    pdd->setCubeSampling( cs );
    pdd->resetManipulation();
    visserv->calculateAttrib( displayid, false );
    visserv->calculateColorAttrib( displayid, false );
}


uiTreeItem* uiODInlineTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd, 
	    	    ODMainWin()->applMgr().visServer()->getObject(visid))
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


uiTreeItem* uiODCrosslineTreeItemFactory::create( int visid, uiTreeItem* ) const
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


uiTreeItem* uiODTimesliceTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd, 
	    	     ODMainWin()->applMgr().visServer()->getObject(visid));
    return pdd && pdd->getType()==2 ? new uiODTimesliceTreeItem(visid) : 0;
}


uiODTimesliceParentTreeItem::uiODTimesliceParentTreeItem()
    : uiODTreeItem( "Timeslice" )
{}


bool uiODTimesliceParentTreeItem::showSubMenu()
{
    mParentShowSubMenu( addChild( new uiODTimesliceTreeItem(-1)); );
}


uiODTimesliceTreeItem::uiODTimesliceTreeItem( int id )
    : uiODPlaneDataTreeItem( id, 2 )
{}


uiODSceneTreeItem::uiODSceneTreeItem( const char* name__, int displayid_ )
    : uiODTreeItem( name__ )
    , displayid( displayid_ )
{}


#define mAnnotText	0
#define mAnnotScale	1
#define mSurveyBox	2
#define mBackgroundCol	3
#define mDumpIV		4
#define mSubMnuSnapshot	5


bool uiODSceneTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet(visSurvey::Scene*,scene,visserv->getObject(displayid))

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

    mnu.insertItem( new uiMenuItem("Background color ..."), mBackgroundCol );


    bool yn = false;
    Settings::common().getYN( IOPar::compKey("dTect","Dump OI Menu"), yn );
    if ( yn )
	mnu.insertItem( new uiMenuItem("Dump OI ..."), mDumpIV );

    yn = true;
    Settings::common().getYN( IOPar::compKey("dTect","Enable snapshot"), yn );
    if ( yn )
	mnu.insertItem( new uiMenuItem("Make snapshot..."), mSubMnuSnapshot );

    const int mnuid=mnu.exec();
    if ( mnuid==mAnnotText )
	scene->showAnnotText( !scene->isAnnotTextShown() );
    else if ( mnuid==mAnnotScale )
	scene->showAnnotScale( !scene->isAnnotScaleShown() );
    else if ( mnuid==mSurveyBox )
	scene->showAnnot( !scene->isAnnotShown() );
    else if ( mnuid==mBackgroundCol )
    {
	Color col = viewer()->getBackgroundColor();
	if ( selectColor(col,getUiParent(),"Color selection",false) )
	    viewer()->setBackgroundColor( col );
    }
    else if ( mnuid==mDumpIV )
	visserv->dumpOI( displayid );
    else if ( mnuid==mSubMnuSnapshot )
	viewer()->renderToFile();

    return true;
}
