/*+
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: uiodtreeitem.cc,v 1.5 2004-04-27 12:00:16 kristofer Exp $";


#include "uiodtreeitemimpl.h"
#include "errh.h"
#include "ptrman.h"
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
#include "uivispartserv.h"
#include "uiwellpartserv.h"
#include "uipickpartserv.h"
#include "uiwellattribpartserv.h"
#include "uiattribpartserv.h"

#include "visrandomtrackdisplay.h"
#include "vissurvsurf.h"
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



#define mMultiIDInit( creationfunc, checkfunc ) \
\
    if ( displayid==-1 ) \
    { \
	displayid = applMgr()->visServer()->creationfunc( sceneID(), mid ); \
	if ( displayid==-1 ) \
	{\
	    return false;\
	}\
    } \
    else if ( applMgr()->visServer()->checkfunc( displayid ) ) \
    { \
	mid = applMgr()->visServer()->getMultiID(displayid); \
    } \
    else \
        return false; \
\
    if ( !uiODDisplayTreeItem::init() ) \
	return false; 


#define mFactoryShowSubMenu( creation ) \
    uiPopupMenu mnu( getUiParent(), "Action" ); \
    mnu.insertItem( new uiMenuItem("Add"), 0 ); \
    const int mnuid = mnu.exec(); \
    if ( mnuid==0 ) \
	creation; \
 \
    return true; \


#define mSelAttributeStart                0
#define mSelAttributeStop               499
#define mSubMnuPickRename               500
#define mSubMnuPickStore                501
#define mSubMnuPickSetDirs              502
#define mSubMnuScenePr                  510
#define mRemoveMnuItem			511
#define mDuplicateMnuItem		512
#define mReloadSurface                  520
#define mStoreSurface			521
#define mStoreMultiIDObject		522
#define mSelectLogs			540
#define mWellAttribSel			541
#define mRestoreSurfDataStart           600
#define mRestoreSurfDataStop            800

bool uiODDisplayTreeItem::factory( uiTreeItem* treeitem, uiODApplMgr* applmgr,
				int displayid )
{
    /*
    uiVisPartServer* visserv = applmgr->visServer();
    uiTreeItem* res = 0;
    if ( visserv->isInlCrlTsl( displayid, 0 ) )
	res = new uiODInlineTreeItem(displayid);
    else if ( visserv->isInlCrlTsl( displayid, 1 ) )
	res = new uiODCrosslineTreeItem(displayid);
    else if ( visserv->isInlCrlTsl( displayid, 2 ) )
	res = new uiODTimesliceTreeItem(displayid);
    else if ( visserv->isVolView(displayid) )
	res = new uiODVolumeTreeItem(displayid);
    else if ( visserv->isRandomLine(displayid) )
	res = new uiODRandomLineTreeItem(displayid);
    else if ( visserv->isHorizon(displayid) )
	res = new uiODHorizonTreeItem(displayid);
    else if ( visserv->isFault(displayid) )
	res = new uiODFaultTreeItem(displayid);
    else if ( visserv->isStickSet(displayid) )
	res = new uiODFaultStickTreeItem(displayid);
    else if ( visserv->isWell(displayid) )
	res = new uiODWellTreeItem(displayid);
    else if ( visserv->isPickSet(displayid) )
	res = new uiODPickSetTreeItem(displayid);

    return res ? treeitem->addChild( res ) : 0;
*/
    return true;
}

	

uiODDisplayTreeItem::uiODDisplayTreeItem( )
    : uiODTreeItem( 0 )
    , displayid( -1 )
{
}


uiODDisplayTreeItem::~uiODDisplayTreeItem( )
{
    uiVisPartServer* visserv = applMgr()->visServer();
    uiVisMenuFactory* menu = visserv->getMenuFactory( displayid, false );
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

    name_ = visserv->getDisplayName( displayid );

    uiVisMenuFactory* menu = visserv->getMenuFactory( displayid, true );
    menu->createnotifier.notify( mCB(this,uiODDisplayTreeItem,createMenuCB));
    menu->handlenotifier.notify( mCB(this,uiODDisplayTreeItem,handleMenuCB));

    return true;
}


void uiODDisplayTreeItem::updateColumnText( int col )
{
    if ( !col )
    {
	name_ = applMgr()->visServer()->getDisplayName( displayid );
    }
    else
    {
	BufferString text = applMgr()->visServer()->getTreeInfo(displayid);
	uilistviewitem->setText( text, col );
    }

    uiTreeItem::updateColumnText(col);
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


const char* uiODDisplayTreeItem::attrselmnutxt = "Select Attribute ...";


void uiODDisplayTreeItem::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet( uiVisMenuFactory*, menu, cb );
    uiVisPartServer* visserv = applMgr()->visServer();
    if ( visserv->hasAttrib(displayid) )
    {
	uiPopupMenu* selattrmnu = new uiPopupMenu( menu->getParent(),
						   attrselmnutxt);
	firstsetattrmnuid = menu->getFreeIdx();
	applMgr()->attrServer()->createAttribSubMenu( *selattrmnu,
					  firstsetattrmnuid,
					  *visserv->getSelSpec(displayid));
	menu->addItem(selattrmnu);
	lastsetattrmnuid = menu->getCurrentIdx()-1;
    }
    else 
	firstsetattrmnuid = -1;

    duplicatemnuid = visserv->canDuplicate(displayid)
	? menu->addItem( new uiMenuItem("Duplicate") ) 
	: -1;

    removemnuid = menu->addItem( new uiMenuItem("Remove"), -1000 );
}


void uiODDisplayTreeItem::handleMenuCB(CallBacker* cb)
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( uiVisMenuFactory*, menu, caller );
    uiVisPartServer* visserv = applMgr()->visServer();
    if ( mnuid==-1 || menu->isHandled() ) return;
    if ( mnuid==duplicatemnuid )
    {
	int newid =visserv->duplicateObject(displayid,sceneID());
	if ( newid!=1 ) uiODDisplayTreeItem::factory( this, applMgr(), newid );
	menu->setIsHandled(true);
    }
    else if ( mnuid==removemnuid )
    {
	visserv->removeObject( displayid, sceneID() );
	parent->removeChild( this );
	menu->setIsHandled(true);
    }
    else if ( firstsetattrmnuid!=-1 && mnuid>=firstsetattrmnuid &&
	    mnuid<=lastsetattrmnuid )
    {
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
	menu->setIsHandled(true);
    }
}


uiODEarthModelSurfaceTreeItem::uiODEarthModelSurfaceTreeItem(
						const MultiID& mid_)
    : mid( mid_ )
{}


bool uiODEarthModelSurfaceTreeItem::init()
{
    uiVisPartServer* visserv = applMgr()->visServer();
    if ( displayid==-1 )
    {
	visSurvey::SurfaceDisplay* sd = visSurvey::SurfaceDisplay::create();
	displayid = sd->id();
	visserv->addObject( sd, sceneID(), true );
	PtrMan<Executor> exec = sd->createSurface( mid );
	if ( !exec )
	{
	    visserv->removeObject(displayid,sceneID());
	    return false;
	}

	uiExecutor uiexec (getUiParent(), *exec );
	if ( !uiexec.execute() )
	{
	    visserv->removeObject(displayid,sceneID());
	    return false;
	}
    }
    else
    {
	mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,
			visserv->getObject(displayid));
	if ( !sd ) return false;
	mid = sd->surfaceId();
    }

    if ( !uiODDisplayTreeItem::init() )
	return false; 

    return true;
}




void uiODEarthModelSurfaceTreeItem::createMenuCB(CallBacker* cb)
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet( uiVisMenuFactory*, menu, cb );
    storemnusel = menu->addItem( new uiMenuItem("Store ...") );

    uiPopupMenu* attrmnu = menu->getMenu( attrselmnutxt );
    if ( attrmnu )
    {
	uiVisPartServer* visserv = applMgr()->visServer();
	attribstartmnusel = menu->getCurrentIdx();
	const AttribSelSpec* as = visserv->getSelSpec(displayid);
	const bool hasauxdata = as && as->id() == -1;
	int nraddeditems = applMgr()->EMServer()->createAuxDataSubMenu(
				*attrmnu, attribstartmnusel, mid, hasauxdata );

	for ( int idx=0; idx<nraddeditems; idx++ )
	    menu->getFreeIdx();

	attribstopmnusel = menu->getCurrentIdx()-1;
    }
    else
    {
	attribstartmnusel = -1;
	attribstopmnusel = -1;
    }

#ifdef __debug__
    reloadmnusel = menu->addItem( new uiMenuItem("Reload") );
#else
    reloadmnusel = -1;
#endif
}


void uiODEarthModelSurfaceTreeItem::handleMenuCB(CallBacker* cb)
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( uiVisMenuFactory*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
	return;
	
    if ( mnuid==storemnusel )
    {
	menu->setIsHandled(true);
	applMgr()->storeSurface(displayid);
    }
    else if ( mnuid==reloadmnusel )
    {
	menu->setIsHandled(true);
	const MultiID& emsurfid = applMgr()->visServer()->getMultiID(displayid);
	uiTreeItem* parent__ = parent;

	applMgr()->visServer()->removeObject(displayid, sceneID());

	if ( !applMgr()->EMServer()->loadSurface( emsurfid ) )
	    return;

	displayid = applMgr()->visServer()->addSurface( sceneID(), emsurfid );
    }
    else if ( mnuid>=attribstartmnusel && mnuid<=attribstopmnusel )
    {
	menu->setIsHandled(true);
	if ( applMgr()->EMServer()->loadAuxData(mid, mnuid-attribstartmnusel) )
	    applMgr()->handleStoredSurfaceData( displayid );
    }
}



/*
uiODFaultStickFactoryTreeItem::uiODFaultStickFactoryTreeItem()
    : uiODTreeItem("FaultSticks" )
{}


bool uiODFaultStickFactoryTreeItem::showSubMenu()
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


uiODRandomLineFactoryTreeItem::uiODRandomLineFactoryTreeItem()
    : uiODTreeItem( "Random line" )
{}


bool uiODRandomLineFactoryTreeItem::showSubMenu()
{
    mFactoryShowSubMenu( addChild(new uiODRandomLineTreeItem(-1)); );
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
	if ( rtd ) return false;
    }

    return uiODDisplayTreeItem::init();
}


void uiODRandomLineTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet( uiVisMenuFactory*, menu, cb );
    uiVisPartServer* visserv = applMgr()->visServer();
    editnodesmnusel = menu->addItem( new uiMenuItem("Edit nodes ...") );

    uiPopupMenu* insertnodemnu = new uiPopupMenu( menu->getParent(),
						  "Insert node before ...");
    mDynamicCastGet( visSurvey::RandomTrackDisplay*,rtd,
	    	     visserv->getObject(displayid));

    for ( int idx=0; idx<rtd->nrKnots(); idx++ )
    {
	BufferString nodename = "node ";
	nodename += idx;
	const int mnusel = menu->getFreeIdx();
	insertnodemnu->insertItem( new uiMenuItem(nodename), mnusel );
	if ( !idx )
	    insertnodemnusel = mnusel;
    }

    menu->addItem(insertnodemnu);
}


void uiODRandomLineTreeItem::handleMenuCB(CallBacker* cb)
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( uiVisMenuFactory*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
	return;
	
    uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet( visSurvey::RandomTrackDisplay*,rtd,
	    	     visserv->getObject(displayid));

    if ( mnuid==editnodesmnusel )
    {
	editNodes();
	menu->setIsHandled(true);
    }
    else if ( insertnodemnusel!=-1 && mnuid>=insertnodemnusel &&
	      mnuid<insertnodemnusel+rtd->nrKnots() )
    {
	rtd->setResolution( mnuid-insertnodemnusel );
	menu->setIsHandled(true);
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
		rtd->setKnotPos( idx, bid );
	    else
		rtd->addKnot( bid );
	}

	visserv->setSelObjectId( rtd->id() );
	visserv->calculateAttrib( rtd->id(), false );
	visserv->calculateColorAttrib( rtd->id(), false );
	if ( viewmodeswap ) visserv->setViewMode( true );
    }

}

uiODFaultFactoryTreeItem::uiODFaultFactoryTreeItem()
   : uiODTreeItem( "Fault" )
{}


bool uiODFaultFactoryTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("Load"), 0 );

    const int mnuid = mnu.exec();

    MultiID mid;
    bool success = false;
    
    if ( mnuid == 0 )
	success = applMgr()->EMServer()->selectFault(mid);

    if ( !success )
	return false;

    addChild( new uiODFaultTreeItem(mid) );

    return true;
}


uiODFaultTreeItem::uiODFaultTreeItem( const MultiID& mid_ )
    : uiODEarthModelSurfaceTreeItem( mid_ )
{}


uiODFaultTreeItem::uiODFaultTreeItem( int id )
    : uiODEarthModelSurfaceTreeItem( 0 )
{ displayid=id; }


uiODHorizonFactoryTreeItem::uiODHorizonFactoryTreeItem()
    : uiODTreeItem( "Horizon" )
{}


bool uiODHorizonFactoryTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("Add"), 0 );

    const int mnuid = mnu.exec();

    MultiID mid;
    bool success = false;
    
    if ( !mnuid )
    {
	success = applMgr()->EMServer()->selectHorizon(mid);
    }

    if ( !success )
	return false;

    addChild( new uiODHorizonTreeItem(mid) );

    return true;
}


uiODHorizonTreeItem::uiODHorizonTreeItem( const MultiID& mid_ )
    : uiODEarthModelSurfaceTreeItem( mid_ )
{}


uiODHorizonTreeItem::uiODHorizonTreeItem( int id )
    : uiODEarthModelSurfaceTreeItem( 0 )
{ displayid=id; }


void uiODHorizonTreeItem::createMenuCB(CallBacker* cb)
{
    uiODEarthModelSurfaceTreeItem::createMenuCB(cb);
    mDynamicCastGet( uiVisMenuFactory*, menu, cb );
    shifthormnusel = menu->addItem( new uiMenuItem("Shift ..."), 100 );
}


void uiODHorizonTreeItem::handleMenuCB(CallBacker* cb)
{
    uiODEarthModelSurfaceTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( uiVisMenuFactory*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( mnuid==shifthormnusel )
    {
	menu->setIsHandled(true);
	uiVisPartServer* visserv = applMgr()->visServer();
	mDynamicCastGet(visSurvey::SurfaceDisplay*,sd,
			visserv->getObject(displayid))
	float shift = sd->getShift();
	BufferString lbl( "Shift " ); lbl += SI().getZUnit();
	DataInpSpec* inpspec = new FloatInpSpec( shift );
	uiGenInputDlg dlg( getUiParent(),"Specify horizon shift", lbl, inpspec);
	if ( !dlg.go() ) return;

	float newshift = dlg.getfValue();
	if ( shift == newshift ) return;

	sd->setShift( dlg.getfValue() );
	if ( sd->hasStoredAttrib() )
	{
	    uiMSG().error( "Cannot calculate this attribute on new location"
			    "\nDepth will be displayed instead" );
	    sd->setZValues();
	    updateColumnText(0);
	}
	else
	    visserv->calculateAttrib( displayid, false );
    }
}

/*

uiODWellFactoryTreeItem::uiODWellFactoryTreeItem()
    : uiODTreeItem( "Well" )
{}



bool uiODWellFactoryTreeItem::showSubMenu()
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


mMultiIDDisplayConstructor( Well )


bool uiODWellTreeItem::init()
{
    mMultiIDInit( addWell, isWell);
    return true;
}


uiPopupMenu* uiODWellTreeItem::createMenu( uiPopupMenu** selattrmnu )
{
    uiPopupMenu* mnu = uiODDisplayTreeItem::createMenu( selattrmnu );
    mnu->insertItem( new uiMenuItem("Select Attribute ..."), mWellAttribSel );
    mnu->insertItem( new uiMenuItem("Select logs ..."), mSelectLogs );
    return mnu;
}


bool uiODWellTreeItem::handleMenu( int mnuid )
{
    const MultiID& wellid = applMgr()->visServer()->getMultiID( displayid );
    if ( mnuid == mSelectLogs )
    {
	int selidx = -1;
	int lognr = 1;
	applMgr()->wellServer()->selectLogs( wellid, selidx, lognr );
	if ( selidx > -1 )
	    applMgr()->visServer()->displayLog( displayid, selidx, lognr );
    }
    else if ( mnuid == mWellAttribSel )
    {
	applMgr()->wellAttribServer()->setAttribSet( 
				*applMgr()->attrServer()->curDescSet() );
	applMgr()->wellAttribServer()->selectAttribute( wellid );
    }

    return uiODDisplayTreeItem::handleMenu( mnuid );
}



uiODPickSetFactoryTreeItem::uiODPickSetFactoryTreeItem()
    : uiODTreeItem( "PickSet" )
{}


bool uiODPickSetFactoryTreeItem::showSubMenu()
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


uiODPickSetTreeItem::uiODPickSetTreeItem( const PickSet* ps_ )
    : ps( ps_ )
{}


uiODPickSetTreeItem::uiODPickSetTreeItem( int id )
{ displayid = id; }


bool uiODPickSetTreeItem::init()
{
    mDisplayInit( uiODDisplayTreeItem, addPickSet(sceneID(), *ps ),
	     	  isPickSet(displayid) );
    ps = 0;
    return true;
}




bool uiODPickSetTreeItem::showSubMenu()
{
    PtrMan<uiPopupMenu> mnu = createMenu(0);
    mnu->insertItem( new uiMenuItem("Rename ..."), mSubMnuPickRename );
    mnu->insertItem( new uiMenuItem("Store ..."), mSubMnuPickStore );
    mnu->insertItem( new uiMenuItem("Set directions ..."), mSubMnuPickSetDirs );
    const int mnuid = mnu && mnu->nrItems() ? mnu->exec() : -1;

    if ( mnuid<0 )
	return false;

    if ( handleMenu( mnuid ) )
	return true;
    else if ( mnuid==mSubMnuPickRename )
	applMgr()->renamePickset( displayid );
    else if ( mnuid==mSubMnuPickStore )
	applMgr()->storeSinglePickSet( displayid );
    else if ( mnuid==mSubMnuPickSetDirs )
	applMgr()->setPickSetDirs( displayid );

    updateColumnText(0);
    updateColumnText(1);

    return true;
}


uiODVolumeFactoryTreeItem::uiODVolumeFactoryTreeItem()
    : uiODTreeItem( "Volume" )
{}


bool uiODVolumeFactoryTreeItem::showSubMenu()
{
    mFactoryShowSubMenu( addChild(new uiODVolumeTreeItem); );
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


uiODInlineFactoryTreeItem::uiODInlineFactoryTreeItem()
    : uiODTreeItem( "Inline" )
{ }


bool uiODInlineFactoryTreeItem::showSubMenu()
{
    mFactoryShowSubMenu( addChild( new uiODInlineTreeItem); );
}


uiODInlineTreeItem::uiODInlineTreeItem( int id )
{ displayid = id; }


bool uiODInlineTreeItem::init()
{
    mDisplayInit( uiODDisplayTreeItem, addInlCrlTsl(sceneID(),0),
	     	  isInlCrlTsl(displayid,0) );
    return true;
}


uiODCrosslineFactoryTreeItem::uiODCrosslineFactoryTreeItem()
    : uiODTreeItem( "Crossline" )
{ }


bool uiODCrosslineFactoryTreeItem::showSubMenu()
{
    mFactoryShowSubMenu( addChild(new uiODCrosslineTreeItem); );
}


uiODCrosslineTreeItem::uiODCrosslineTreeItem( int id )
{ displayid = id; }


bool uiODCrosslineTreeItem::init()
{
    mDisplayInit( uiODDisplayTreeItem, addInlCrlTsl(sceneID(),1),
	     	  isInlCrlTsl(displayid,1) );
    return true;
}


uiODTimesliceFactoryTreeItem::uiODTimesliceFactoryTreeItem()
    : uiODTreeItem( "Time" )
{ }


bool uiODTimesliceFactoryTreeItem::showSubMenu()
{
    mFactoryShowSubMenu( addChild(new uiODTimesliceTreeItem); );
}


uiODTimesliceTreeItem::uiODTimesliceTreeItem( int id )
{ displayid = id; }


bool uiODTimesliceTreeItem::init()
{
    mDisplayInit( uiODDisplayTreeItem, addInlCrlTsl(sceneID(),2),
	     	  isInlCrlTsl(displayid,2) );
    return true;
}


uiODSceneTreeItem::uiODSceneTreeItem( const char* name__, int displayid_)
    : uiODTreeItem( name__ )
    , displayid( displayid_ )
{}


bool uiODSceneTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    uiVisPartServer* visserv = applMgr()->visServer();
    visserv->makeSubMenu(mnu,sceneID(),displayid);

    if ( getenv("DTECT_ENABLE_SCENE_PRINT") )
	mnu.insertItem( new uiMenuItem("'Print' to file ..."), mSubMnuScenePr );

    const int mnuid=mnu.exec();
    if ( mnuid>1023 )
	applMgr()->visServer()->handleSubMenuSel( mnuid, sceneID(), displayid );
    else if ( mnuid==mSubMnuScenePr )
    {
	viewer()->renderToFile();
    }
    return true;
}
*/
