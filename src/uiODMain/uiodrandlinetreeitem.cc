/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		May 2006
 RCS:		$Id: uiodrandlinetreeitem.cc,v 1.10 2007-11-06 16:31:49 cvsbert Exp $
___________________________________________________________________

-*/

#include "uiodrandlinetreeitem.h"

#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uivispartserv.h"

#include "ctxtioobj.h"
#include "ptrman.h"
#include "randomlinetr.h"
#include "randomlinegeom.h"
#include "survinfo.h"

#include "uibinidtable.h"
#include "uidialog.h"
#include "uiioobjsel.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodscenemgr.h"
#include "uiwellpartserv.h"
#include "visrandomtrackdisplay.h"


uiTreeItem* uiODRandomLineTreeItemFactory::create( int visid, uiTreeItem* ) const
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
    mDynamicCastGet(visSurvey::Scene*,scene,
	    	    ODMainWin()->applMgr().visServer()->getObject(sceneID()));
    if ( scene && scene->getDataTransform() )
    {
	uiMSG().message( "Cannot add Random lines to this scene (yet)" );
	return false;
    }

    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("Add"), 0 );
    mnu.insertItem( new uiMenuItem("Load ..."), 1 );
    addStandardItems( mnu );
    const int mnuid = mnu.exec();
    if ( mnuid == 0 )
	addChild( new uiODRandomLineTreeItem(-1), false );
    else if ( mnuid == 1 )
	load();

    handleStandardItems( mnuid );
    return true;
}


bool uiODRandomLineParentTreeItem::load()
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj( RandomLineSet );
    ctio->ctxt.forread = true;
    uiIOObjSelDlg dlg( getUiParent(), *ctio, "Select", false );
    if ( !dlg.go() )
	return false;

    Geometry::RandomLineSet lset;
    BufferString errmsg;
    if ( !RandomLineSetTranslator::retrieve(lset,dlg.ioObj(),errmsg) )
	{ uiMSG().error( errmsg ); return false; }

    uiODRandomLineTreeItem* itm = new uiODRandomLineTreeItem(-1);
    addChild( itm, false );
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
	ODMainWin()->applMgr().visServer()->getObject(itm->displayID()));
    if ( !rtd )
	return false;

    //TODO handle subselection in set
    TypeSet<BinID> bids;
    const Geometry::RandomLine& rln = *lset.lines()[0];
    rln.allNodePositions( bids );
    rtd->setKnotPositions( bids );
    rtd->setDepthInterval( rln.zRange() );
    rtd->setName( dlg.ioObj()->name() );
    updateColumnText( uiODSceneMgr::cNameColumn() );
    return true;
}


uiODRandomLineTreeItem::uiODRandomLineTreeItem( int id )
    : editnodesmnuitem_("Edit nodes ...")
    , insertnodemnuitem_("Insert node")
    , usewellsmnuitem_("Create from wells ...")
    , saveasmnuitem_("Save as ...")
{ displayid_ = id; } 


bool uiODRandomLineTreeItem::init()
{
    visSurvey::RandomTrackDisplay* rtd = 0;
    if ( displayid_==-1 )
    {
	rtd = visSurvey::RandomTrackDisplay::create();
	displayid_ = rtd->id();
	visserv_->addObject( rtd, sceneID(), true );
    }
    else
    {
	mDynamicCastGet(visSurvey::RandomTrackDisplay*,disp,
			visserv_->getObject(displayid_));
	if ( !disp ) return false;
	rtd = disp;
    }

    if ( rtd )
    {
	rtd->getMovementNotifier()->notify(
		mCB(this,uiODRandomLineTreeItem,remove2DViewerCB) );
	rtd->getManipulationNotifier()->notify(
		mCB(this,uiODRandomLineTreeItem,remove2DViewerCB) );
    }

    return uiODDisplayTreeItem::init();
}


void uiODRandomLineTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( menu->menuID()!=displayID() )
	return;

    mAddMenuItem( menu, &editnodesmnuitem_, !visserv_->isLocked(displayid_), 
	    	  false );
    mAddMenuItem( menu, &insertnodemnuitem_, !visserv_->isLocked(displayid_), 
	    	  false );
    insertnodemnuitem_.removeItems();

    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
		    visserv_->getObject(displayid_));
    for ( int idx=0; idx<=rtd->nrKnots(); idx++ )
    {
	if ( visserv_->isLocked(displayid_) ) break;
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

	mAddManagedMenuItem(&insertnodemnuitem_,new MenuItem(nodename), 
			    rtd->canAddKnot(idx), false );
    }
    mAddMenuItem( menu, &usewellsmnuitem_, !visserv_->isLocked(displayid_), 
	    	  false );
    mAddMenuItem( menu, &saveasmnuitem_, true, false );
}


void uiODRandomLineTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( menu->menuID()!=displayID() || mnuid==-1 || menu->isHandled() )
	return;
	
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
		    visserv_->getObject(displayid_));

    if ( mnuid==editnodesmnuitem_.id )
    {
	editNodes();
	menu->setIsHandled(true);
    }
    else if ( insertnodemnuitem_.itemIndex(mnuid)!=-1 )
    {
	menu->setIsHandled(true);
	rtd->addKnot(insertnodemnuitem_.itemIndex(mnuid));
    }
    else if ( mnuid==usewellsmnuitem_.id )
    {
	menu->setIsHandled(true);
	applMgr()->wellServer()->selectWellCoordsForRdmLine();
    }
    else if ( mnuid == saveasmnuitem_.id )
    {
	PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj( RandomLineSet );
	ctio->ctxt.forread = false;
	ctio->setName( rtd->name() );
	uiIOObjSelDlg dlg( getUiParent(), *ctio, "Select", false );
	if ( !dlg.go() )
	    return;
	    
	TypeSet<BinID> bids;
	rtd->getAllKnotPos( bids );
	Geometry::RandomLineSet lset;
	Geometry::RandomLine* rln = new Geometry::RandomLine;
	for ( int idx=0; idx<bids.size(); idx++ )
	    rln->addNode( bids[idx] );
	rln->setZRange( rtd->getDepthInterval() );
	lset.addLine( rln );
	BufferString bs;
	if ( !RandomLineSetTranslator::store(lset,dlg.ioObj(),bs) )
	    uiMSG().error( bs );
    }
}


void uiODRandomLineTreeItem::editNodes()
{
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
		    visserv_->getObject(displayid_));

    TypeSet<BinID> bids;
    rtd->getAllKnotPos( bids );
    uiDialog dlg( getUiParent(),
	    	  uiDialog::Setup("Random lines","Specify node positions","") );
    uiBinIDTable* table = new uiBinIDTable( &dlg, true );
    table->setBinIDs( bids );

    Interval<float> zrg = rtd->getDataTraceRange();
    zrg.scale( SI().zFactor() );
    table->setZRange( zrg );
    if ( dlg.go() )
    {
	TypeSet<BinID> newbids;
	table->getBinIDs( newbids );
	rtd->setKnotPositions( newbids );

	table->getZRange( zrg );
	zrg.scale( 1/SI().zFactor() );
	rtd->setDepthInterval( zrg );

	visserv_->setSelObjectId( rtd->id() );
	for ( int attrib=0; attrib<visserv_->getNrAttribs(rtd->id()); attrib++ )
	    visserv_->calculateAttrib( rtd->id(), attrib, false );

	ODMainWin()->sceneMgr().updateTrees();
    }
}


void uiODRandomLineTreeItem::remove2DViewerCB( CallBacker* )
{
    ODMainWin()->sceneMgr().remove2DViewer( displayid_ );
}
