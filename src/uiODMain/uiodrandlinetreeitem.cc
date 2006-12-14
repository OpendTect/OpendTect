/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		May 2006
 RCS:		$Id: uiodrandlinetreeitem.cc,v 1.3 2006-12-14 21:44:35 cvsnanne Exp $
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
    {
    	PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj( RandomLine );
	ctio->ctxt.forread = true;
	uiIOObjSelDlg dlg( getUiParent(), *ctio, "Select", false );
	if ( !dlg.go() )
	    return false;

	Geometry::RandomLine geom;
	BufferString errmsg;
	RandomLineTranslator::retrieve( geom, dlg.ioObj(), errmsg );
    }

    handleStandardItems( mnuid );
    return true;
}


uiODRandomLineTreeItem::uiODRandomLineTreeItem( int id )
    : editnodesmnuitem_("Edit nodes ...")
    , insertnodemnuitem_("Insert node")
    , usewellsmnuitem_("Create from wells ...")
    , savemnuitem_("Save ...")
{ displayid_ = id; } 


bool uiODRandomLineTreeItem::init()
{
    if ( displayid_==-1 )
    {
	visSurvey::RandomTrackDisplay* rtd =
				    visSurvey::RandomTrackDisplay::create();
	displayid_ = rtd->id();
	visserv->addObject( rtd, sceneID(), true );
    }
    else
    {
	mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
			visserv->getObject(displayid_));
	if ( !rtd ) return false;
    }

    return uiODDisplayTreeItem::init();
}


void uiODRandomLineTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( menu->menuID()!=displayID() )
	return;

    mAddMenuItem( menu, &editnodesmnuitem_, !visserv->isLocked(displayid_), 
	    	  false );
    mAddMenuItem( menu, &insertnodemnuitem_, !visserv->isLocked(displayid_), 
	    	  false );
    insertnodemnuitem_.removeItems();

    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
		    visserv->getObject(displayid_));
    for ( int idx=0; idx<=rtd->nrKnots(); idx++ )
    {
	if ( visserv->isLocked(displayid_) ) break;
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
    mAddMenuItem( menu, &usewellsmnuitem_, !visserv->isLocked(displayid_), 
	    	  false );
    mAddMenuItem( menu, &savemnuitem_, true, false );
}


void uiODRandomLineTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( menu->menuID()!=displayID() || mnuid==-1 || menu->isHandled() )
	return;
	
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
		    visserv->getObject(displayid_));

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
    else if ( mnuid == savemnuitem_.id )
    {
	PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj( RandomLine );
	ctio->ctxt.forread = false;
	uiIOObjSelDlg dlg( getUiParent(), *ctio, "Select", false );
	if ( !dlg.go() )
	    return;
	    
	TypeSet<BinID> bids;
	rtd->getAllKnotPos( bids );
	Geometry::RandomLine geom;
	geom.setZRange( rtd->getDepthInterval() );
	for ( int idx=0; idx<bids.size(); idx++ )
	    geom.addNode( bids[idx] );
	BufferString bs;
	RandomLineTranslator::store( geom, dlg.ioObj(), bs );
    }
}


void uiODRandomLineTreeItem::editNodes()
{
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
		    visserv->getObject(displayid_));

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

	Interval<float> zrg;
	table->getZRange( zrg );
	zrg.scale( 1/SI().zFactor() );
	rtd->setDepthInterval( zrg );

	visserv->setSelObjectId( rtd->id() );
	for ( int attrib=0; attrib<visserv->getNrAttribs(rtd->id()); attrib++ )
	    visserv->calculateAttrib( rtd->id(), attrib, false );

	ODMainWin()->sceneMgr().updateTrees();
    }
}
