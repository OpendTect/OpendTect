/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		May 2006
 RCS:		$Id: uiodrandlinetreeitem.cc,v 1.14 2007-12-12 11:55:44 cvsnanne Exp $
___________________________________________________________________

-*/

#include "uiodrandlinetreeitem.h"

#include "ctxtioobj.h"
#include "ptrman.h"
#include "randomlinetr.h"
#include "randomlinegeom.h"
#include "survinfo.h"

#include "uibinidtable.h"
#include "uibutton.h"
#include "uicursor.h"
#include "uidialog.h"
#include "uiempartserv.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiselsimple.h"
#include "uivispartserv.h"
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
    mnu.insertItem( new uiMenuItem("&New"), 0 );
    mnu.insertItem( new uiMenuItem("&Load ..."), 1 );
    uiPopupMenu* genmnu = new uiPopupMenu( getUiParent(), "&Generate" );
    genmnu->insertItem( new uiMenuItem("From &Existing"), 2 );
    genmnu->insertItem( new uiMenuItem("Along &Contours"), 3 );
    mnu.insertItem( genmnu );
    addStandardItems( mnu );
    const int mnuid = mnu.exec();
    if ( mnuid == 0 )
	addChild( new uiODRandomLineTreeItem(-1), false );
    else if ( mnuid == 1 )
	load();
    else if ( mnuid == 2 || mnuid == 3 )
	genRandLine( mnuid == 3 );

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

    const ObjectSet<Geometry::RandomLine>& lines = lset.lines();
    BufferStringSet linenames;
    for ( int idx=0; idx<lines.size(); idx++ )
	linenames.add( lines[idx]->name() );

    bool lockgeom = false;
    TypeSet<int> selitms;
    if ( linenames.isEmpty() )
	return false;
    else if ( linenames.size() == 1 )
	selitms += 0;
    else
    {
	uiSelectFromList seldlg( getUiParent(),
		uiSelectFromList::Setup("Random lines",linenames) );
	seldlg.selFld()->setMultiSelect( true );
	uiCheckBox* cb = new uiCheckBox( &seldlg, "Editable" );
	cb->attach( alignedBelow, seldlg.selFld() );
	if ( !seldlg.go() )
	    return false;

	seldlg.selFld()->getSelectedItems( selitms );
	lockgeom = !cb->isChecked();
    }

    uiCursorChanger cursorchgr( uiCursor::Wait );
    for ( int idx=0; idx<selitms.size(); idx++ )
    {
	uiODRandomLineTreeItem* itm = new uiODRandomLineTreeItem(-1);
	addChild( itm, false );
	mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
	    ODMainWin()->applMgr().visServer()->getObject(itm->displayID()));
	if ( !rtd )
	    return false;

	TypeSet<BinID> bids;
	const Geometry::RandomLine& rln = *lset.lines()[ selitms[idx] ];
	rln.allNodePositions( bids ); rtd->setKnotPositions( bids );
	rtd->setDepthInterval( rln.zRange() );
	BufferString rlnm = dlg.ioObj()->name();
	if ( !rln.name().isEmpty() )
	{ rlnm += ": "; rlnm += rln.name(); }
	rtd->setName( rlnm );
	rtd->lockGeometry( lockgeom );
    }
    updateColumnText( uiODSceneMgr::cNameColumn() );
    return true;
}


void uiODRandomLineParentTreeItem::genRandLine( bool cont )
{
    const char* res = applMgr()->EMServer()->genRandLine( cont );
}


uiODRandomLineTreeItem::uiODRandomLineTreeItem( int id )
    : editnodesmnuitem_("&Edit nodes ...")
    , insertnodemnuitem_("&Insert node")
    , usewellsmnuitem_("&Create from wells ...")
    , saveasmnuitem_("&Save ...")
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

    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
		    visserv_->getObject(displayid_));
    const bool islocked = rtd->isGeometryLocked() || rtd->isLocked();
    mAddMenuItem( menu, &editnodesmnuitem_, !islocked, false );
    mAddMenuItem( menu, &insertnodemnuitem_, !islocked, false );
    insertnodemnuitem_.removeItems();

    for ( int idx=0; !islocked && idx<=rtd->nrKnots(); idx++ )
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

	mAddManagedMenuItem(&insertnodemnuitem_,new MenuItem(nodename), 
			    rtd->canAddKnot(idx), false );
    }
    mAddMenuItem( menu, &usewellsmnuitem_, !islocked, false );
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
	    
	TypeSet<BinID> bids; rtd->getAllKnotPos( bids );
	Geometry::RandomLine* rln = new Geometry::RandomLine;
	for ( int idx=0; idx<bids.size(); idx++ )
	    rln->addNode( bids[idx] );
	rln->setZRange( rtd->getDepthInterval() );
	Geometry::RandomLineSet lset; lset.addLine( rln );
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
