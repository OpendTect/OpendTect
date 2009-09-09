/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author: 	K. Tingdahl
 Date: 		May 2006
___________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodrandlinetreeitem.cc,v 1.30 2009-09-09 09:29:16 cvsnanne Exp $";

#include "uiodrandlinetreeitem.h"

#include "ctxtioobj.h"
#include "ptrman.h"
#include "randomlinetr.h"
#include "randomlinegeom.h"
#include "survinfo.h"
#include "strmprov.h"
#include "trigonometry.h"

#include "uibinidtable.h"
#include "uibutton.h"
#include "mousecursor.h"
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
#include "uiwellrdmlinedlg.h"
#include "uiseispartserv.h"
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
    genmnu->insertItem( new uiMenuItem("From &Existing ..."), 2 );
    genmnu->insertItem( new uiMenuItem("Along &Contours ..."), 3 );
    genmnu->insertItem( new uiMenuItem("From &Polygon ..."), 4 );
    genmnu->insertItem( new uiMenuItem("From &Wells ..."), 5 );
    mnu.insertItem( genmnu );
    addStandardItems( mnu );
    const int mnuid = mnu.exec();
    if ( mnuid == 0 )
	addChild( new uiODRandomLineTreeItem(-1), false );
    else if ( mnuid == 1 )
    {
	const IOObj* ioobj = selRandomLine();
	if ( ioobj )
	    load( *ioobj );
    }
    else if ( mnuid>1 && mnuid<5 )
	genRandLine( mnuid-2 );
    else if ( mnuid == 5 )
	genRandLineFromWell();

    handleStandardItems( mnuid );
    return true;
}


const IOObj* uiODRandomLineParentTreeItem::selRandomLine()
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj( RandomLineSet );
    ctio->ctxt.forread = true;
    uiIOObjSelDlg dlg( getUiParent(), *ctio, "Select", false );
    return dlg.go() && dlg.ioObj() ? dlg.ioObj()->clone() : 0;
}


bool uiODRandomLineParentTreeItem::load( const IOObj& ioobj )
{
    Geometry::RandomLineSet lset;
    BufferString errmsg;
    if ( !RandomLineSetTranslator::retrieve(lset,&ioobj,errmsg) )
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

    MouseCursorChanger cursorchgr( MouseCursor::Wait );
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
	BufferString rlnm = ioobj.name();
	if ( lines.size()>1 && !rln.name().isEmpty() )
	{ rlnm += ": "; rlnm += rln.name(); }
	rtd->setName( rlnm );
	rtd->lockGeometry( lockgeom );
    }
    updateColumnText( uiODSceneMgr::cNameColumn() );
    return true;
}


void uiODRandomLineParentTreeItem::genRandLine( int opt )
{
    const char* multiid = applMgr()->EMServer()->genRandLine( opt );
    if ( multiid && applMgr()->EMServer()->dispLineOnCreation() )
    {
	PtrMan<IOObj> ioobj = IOM().get( multiid );
	load( *ioobj );
    }
}


void uiODRandomLineParentTreeItem::genRandLineFromWell()
{
    applMgr()->wellServer()->setSceneID( sceneID() );
    applMgr()->wellServer()->selectWellCoordsForRdmLine();
    applMgr()->wellServer()->randLineDlgClosed.notify(
	    mCB(this,uiODRandomLineParentTreeItem,loadRandLineFromWell) );
}


void uiODRandomLineParentTreeItem::loadRandLineFromWell( CallBacker* )
{
    const char* multiid =  applMgr()->wellServer()->getRandLineMultiID();
    if ( multiid && applMgr()->wellServer()->dispLineOnCreation() )
    {
	PtrMan<IOObj> ioobj = IOM().get( multiid );
	load( *ioobj );
    }

    applMgr()->wellServer()->randLineDlgClosed.remove(
	    mCB(this,uiODRandomLineParentTreeItem,loadRandLineFromWell) );
}


uiODRandomLineTreeItem::uiODRandomLineTreeItem( int id )
    : editnodesmnuitem_("&Edit nodes ...")
    , insertnodemnuitem_("&Insert node")
    , usewellsmnuitem_("&Create from wells ...")
    , saveasmnuitem_("&Save ...")
    , saveas2dmnuitem_("Save as &2D ...")
    , create2dgridmnuitem_("Create 2D &Grid ...")
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
    mAddMenuItem( menu, &saveas2dmnuitem_, true, false );
    mAddMenuItem( menu, &create2dgridmnuitem_, true, false );
}


void uiODRandomLineTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() || mnuid==-1 )
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
    else
    {
	TypeSet<BinID> bids; rtd->getAllKnotPos( bids );
	PtrMan<Geometry::RandomLine> rln = new Geometry::RandomLine;
	rln->setZRange( rtd->getDepthInterval() );
	for ( int idx=0; idx<bids.size(); idx++ )
	    rln->addNode( bids[idx] );
	    
	if ( mnuid == saveasmnuitem_.id )
	{
	    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj( RandomLineSet );
	    ctio->ctxt.forread = false;
	    ctio->setName( rtd->name() );
	    uiIOObjSelDlg dlg( getUiParent(), *ctio, "Select", false );
	    if ( !dlg.go() ) return;

	    Geometry::RandomLineSet lset; lset.addLine( rln );
	    BufferString bs;
	    if ( !RandomLineSetTranslator::store(lset,dlg.ioObj(),bs) )
		uiMSG().error( bs );
	    else
	    {
		applMgr()->visServer()->setObjectName( displayID(),
						       dlg.ioObj()->name() );
		updateColumnText( uiODSceneMgr::cNameColumn() );
	    }
	}
	else if ( mnuid == saveas2dmnuitem_.id )
	{
	    rln->setName( rtd->name() );
	    applMgr()->seisServer()->storeRlnAs2DLine( *rln );
	}
	else if ( mnuid == create2dgridmnuitem_.id )
	{
	    if ( rtd->nrKnots() > 2 )
	    {
		uiMSG().error( "Grids can only be created from Random Lines\n"
			       "with only 2 nodes" );
		return;
	    }

	    rln->setName( rtd->name() );
	    applMgr()->seisServer()->create2DGridFromRln( *rln );
	}
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
