/*+
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: uiodtreeitem.cc,v 1.4 2004-03-01 14:39:31 nanne Exp $";


#include "uiodtreeitemimpl.h"
#include "errh.h"
#include "ptrman.h"
#include "uimenu.h"
#include "pickset.h"
#include "uilistview.h"
#include "uisoviewer.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiempartserv.h"
#include "uivispartserv.h"
#include "uiwellpartserv.h"
#include "uipickpartserv.h"
#include "uiwellattribpartserv.h"
#include "uiattribpartserv.h"


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



#define mMultiIDDisplayConstructor( type ) \
uiOD##type##TreeItem::uiOD##type##TreeItem( const MultiID& mid_ ) \
    : uiODMultiIDTreeItem( mid_ ) \
{} \
\
uiOD##type##TreeItem::uiOD##type##TreeItem( int id ) \
    : uiODMultiIDTreeItem( 0 )\
{ displayid=id; }

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
}
	

uiODDisplayTreeItem::uiODDisplayTreeItem( )
    : uiODTreeItem( 0 )
    , displayid( -1 )
{
}


int uiODDisplayTreeItem::selectionKey() const { return displayid; }


bool uiODDisplayTreeItem::init()
{
    if ( !uiTreeItem::init() ) return false;

    applMgr()->visServer()->setSelObjectId( displayid );
    uilistviewitem->setChecked( applMgr()->visServer()->isOn(displayid) );
    uilistviewitem->stateChanged.notify( mCB(this,uiODDisplayTreeItem,checkCB));

    name_ = applMgr()->visServer()->getDisplayName( displayid );
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


void uiODDisplayTreeItem::checkCB(CallBacker*)
{
    applMgr()->visServer()->turnOn( displayid, uilistviewitem->isChecked() );
}


int uiODDisplayTreeItem::uiListViewItemType() const
{
    return uiListViewItem::CheckBox;
}




bool uiODDisplayTreeItem::showSubMenu()
{
    uiPopupMenu* attrselmnu = 0;
    PtrMan<uiPopupMenu> mnu = createMenu( &attrselmnu );
    const int mnuid = mnu && mnu->nrItems() ? mnu->exec() : -1;

    if ( mnuid>=0 )
	handleMenu( mnuid );

    return true;
}


uiPopupMenu* uiODDisplayTreeItem::createMenu( uiPopupMenu** attrsel )
{
    uiPopupMenu* mnu = new uiPopupMenu( getUiParent(), "Action" );
    uiVisPartServer* visserv = applMgr()->visServer();
    visserv->makeSubMenu(*mnu,sceneID(),displayid);

    if ( visserv->hasAttrib(displayid) )
    {
	uiPopupMenu* selattrmnu = new uiPopupMenu(getUiParent(),
						  "Select Attribute");
	mnu->insertItem( selattrmnu, -1, 0 );
	applMgr()->createSubMenu( *selattrmnu, mSelAttributeStart, displayid,0);
	if ( attrsel ) (*attrsel)=selattrmnu;
    }

    if ( visserv->canDuplicate(displayid) )
	mnu->insertItem( new uiMenuItem("Duplicate"), mDuplicateMnuItem );
    
    mnu->insertItem( new uiMenuItem("Remove") , mRemoveMnuItem );

    return mnu;
}


bool uiODDisplayTreeItem::handleMenu( int mnuid )
{
    if ( mnuid>1023 )
    {
	applMgr()->visServer()->handleSubMenuSel( mnuid, sceneID(), displayid );
	return true;
    }
    else if ( mnuid==mDuplicateMnuItem )
    {
	int newid =applMgr()->visServer()->duplicateObject(displayid,sceneID());
	if ( newid!=1 )
	    uiODDisplayTreeItem::factory( this, applMgr(), newid );

	return true;
    }
    else if ( mnuid==mRemoveMnuItem )
    {
	applMgr()->visServer()->removeObject( displayid, sceneID() );
	parent->removeChild( this );
	return true;
    }
    else if ( mnuid>=mSelAttributeStart && mnuid<=mSelAttributeStop )
	applMgr()->handleSubMenu( mnuid-mSelAttributeStart, displayid, 0 );

    return false;
}


uiODMultiIDTreeItem::uiODMultiIDTreeItem( const MultiID& mid_ )
    : mid( mid_ )
{}


uiPopupMenu* uiODMultiIDTreeItem::createMenu( uiPopupMenu** attrsel )
{
    uiPopupMenu* res = uiODDisplayTreeItem::createMenu( attrsel );
    res->insertItem( new uiMenuItem("Store ..."), mStoreMultiIDObject );
    return res;
}


bool uiODMultiIDTreeItem::handleMenu( int mnuid )
{
    if ( mnuid!=mStoreMultiIDObject )
	return  uiODDisplayTreeItem::handleMenu(mnuid);

    applMgr()->EMServer()->storeObject(mid);
    return true;
}


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


uiODRandomLineFactoryTreeItem::uiODRandomLineFactoryTreeItem()
    : uiODTreeItem( "Random line" )
{}


bool uiODRandomLineFactoryTreeItem::showSubMenu()
{
    mFactoryShowSubMenu( addChild(new uiODRandomLineTreeItem); );
}


uiODRandomLineTreeItem::uiODRandomLineTreeItem( int id )
{ displayid = id; } 


bool uiODRandomLineTreeItem::init()
{
    mDisplayInit( uiODDisplayTreeItem, addRandomLine(sceneID()),
	     	  isRandomLine(displayid) );
    return true;
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


mMultiIDDisplayConstructor( Fault )


bool uiODFaultTreeItem::init()
{
    mMultiIDInit( addSurface, isFault );
    return true;
}



uiPopupMenu* uiODFaultTreeItem::createMenu( uiPopupMenu** selattrmnu )
{
    uiPopupMenu* mnu = uiODDisplayTreeItem::createMenu( selattrmnu );
    mnu->insertItem( new uiMenuItem("Store ..."), mStoreSurface );
    if ( selattrmnu && *selattrmnu )
	applMgr()->createSubMenu( **selattrmnu,mRestoreSurfDataStart,
		displayid,1);
#ifdef __debug__
    mnu->insertItem( new uiMenuItem("Reload"), mReloadSurface );
#endif
    return mnu;
}


bool uiODFaultTreeItem::handleMenu( int mnuid )
{
    if ( mnuid==mStoreSurface )
    {
	applMgr()->storeSurface(displayid);
	return true;
    }
    else if ( mnuid==mReloadSurface )
    {
	const MultiID& emsurfid = applMgr()->visServer()->getMultiID(displayid);
	uiTreeItem* parent__ = parent;

	applMgr()->visServer()->removeObject(displayid, sceneID());

	if ( !applMgr()->EMServer()->loadSurface( emsurfid ) )
	    return false;

	displayid = applMgr()->visServer()->addSurface( sceneID(), emsurfid );
	return true;
    }
    else if ( mnuid>=mRestoreSurfDataStart && mnuid<=mRestoreSurfDataStop )
    {
	applMgr()->handleSubMenu( mnuid-mRestoreSurfDataStart, displayid, 1 );
	return true;
    }

    return uiODDisplayTreeItem::handleMenu( mnuid );
}


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
    
    if ( mnuid==0 )
    {
	success = applMgr()->EMServer()->selectHorizon(mid);
    }

    if ( !success )
	return false;

    addChild( new uiODHorizonTreeItem(mid) );

    return true;
}


mMultiIDDisplayConstructor( Horizon )


bool uiODHorizonTreeItem::init()
{
    mMultiIDInit( addSurface, isHorizon );
    return true;
}


uiPopupMenu* uiODHorizonTreeItem::createMenu( uiPopupMenu** selattrmnu )
{
    uiPopupMenu* mnu = uiODDisplayTreeItem::createMenu( selattrmnu );
    mnu->insertItem( new uiMenuItem("Store ..."), mStoreSurface );
    if ( selattrmnu && *selattrmnu )
	applMgr()->createSubMenu( **selattrmnu,mRestoreSurfDataStart,
		displayid,1);
#ifdef __debug__
    mnu->insertItem( new uiMenuItem("Reload"), mReloadSurface );
#endif
    return mnu;
}


bool uiODHorizonTreeItem::handleMenu(int mnuid)
{
    if ( mnuid==mStoreSurface )
    {
	applMgr()->storeSurface(displayid);
	return true;
    }
    else if ( mnuid==mReloadSurface )
    {
	const MultiID& emsurfid = applMgr()->visServer()->getMultiID(displayid);
	uiTreeItem* parent__ = parent;

	applMgr()->visServer()->removeObject(displayid, sceneID());

	if ( !applMgr()->EMServer()->loadSurface( emsurfid ) )
	    return false;

	applMgr()->visServer()->addSurface( sceneID(), emsurfid );
	return true;
    }
    else if ( mnuid>=mRestoreSurfDataStart && mnuid<=mRestoreSurfDataStop )
    {
	applMgr()->handleSubMenu( mnuid-mRestoreSurfDataStart, displayid, 1 );
	return true;
    }

    return uiODDisplayTreeItem::handleMenu( mnuid );
}


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
