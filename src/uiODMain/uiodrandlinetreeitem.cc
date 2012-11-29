/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author: 	K. Tingdahl
 Date: 		May 2006
___________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiodrandlinetreeitem.h"

#include "ctxtioobj.h"
#include "mousecursor.h"
#include "ptrman.h"
#include "randomlinetr.h"
#include "randomlinegeom.h"
#include "randcolor.h"
#include "survinfo.h"
#include "strmprov.h"
#include "trigonometry.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uicreate2dgrid.h"
#include "uidialog.h"
#include "uiempartserv.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uilabel.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiodviewer2dmgr.h"
#include "uipositiontable.h"
#include "uiselsimple.h"
#include "uivispartserv.h"
#include "uiwellpartserv.h"
#include "uiwellrdmlinedlg.h"
#include "uiseispartserv.h"
#include "visrandomtrackdisplay.h"
#include "visrgbatexturechannel2rgba.h"


class uiRandomLinePolyLineDlg : public uiDialog
{
public:
uiRandomLinePolyLineDlg(uiParent* p, visSurvey::RandomTrackDisplay* rtd )
    : uiDialog(p,Setup("Create Random Line from Polyline","","109.0.6")
		 .modal(false))
    , rtd_(rtd) 
{
    label_ = new uiLabel( this, "Pick nodes on Z-Slices or Horizons" );
    colsel_ = new uiColorInput( this, uiColorInput::Setup(getRandStdDrawColor())
				      .lbltxt("Color") );
    colsel_->attach( alignedBelow, label_ );
    colsel_->colorChanged.notify(
	    mCB(this,uiRandomLinePolyLineDlg,colorChangeCB) );

    rtd_->removeAllKnots();
    rtd->setPolyLineMode( true );
    rtd_->setColor( colsel_->color() );
}


void colorChangeCB( CallBacker* )
{ rtd_->setColor( colsel_->color() ); }


bool acceptOK( CallBacker* )
{
    if ( !rtd_->createFromPolyLine() )
    {
	uiMSG().error("Please select at least two points on TimeSlice/Horizon");
	return false;
    }
    rtd_->setPolyLineMode( false );
    return true;
}


int getDisplayID() const
{ return rtd_->id(); }


protected:
    visSurvey::RandomTrackDisplay* rtd_;
    uiLabel*		label_;
    uiColorInput*	colsel_;
};


// Tree Items
uiTreeItem*
    uiODRandomLineTreeItemFactory::createForVis( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd, 
		    ODMainWin()->applMgr().visServer()->getObject(visid));
    return rtd ? new uiODRandomLineTreeItem(visid) : 0;
}


uiODRandomLineParentTreeItem::uiODRandomLineParentTreeItem()
    : uiODTreeItem( "Random line" )
    , rdlpolylinedlg_(0)
{}


bool uiODRandomLineParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("Add &Empty"), 0 );
    mnu.insertItem( new uiMenuItem("Add &Stored ..."), 7 );

    uiPopupMenu* rgbmnu =
	new uiPopupMenu( getUiParent(), "Add &Color blended" );
    rgbmnu->insertItem( new uiMenuItem("&Empty"), 8 );
    rgbmnu->insertItem( new uiMenuItem("&Stored ..."), 9 );
    mnu.insertItem( rgbmnu );

    uiPopupMenu* newmnu = new uiPopupMenu( getUiParent(), "&New" );
    newmnu->insertItem( new uiMenuItem("&Interactive  ..."), 6 );
    newmnu->insertItem( new uiMenuItem("Along &Contours ..."), 2 );
    newmnu->insertItem( new uiMenuItem("From &Existing ..."), 1 );
    newmnu->insertItem( new uiMenuItem("From &Polygon ..."), 3 );
    newmnu->insertItem( new uiMenuItem("From &Table ..."), 5 );
    newmnu->insertItem( new uiMenuItem("From &Wells ..."), 4 );
    mnu.insertItem( newmnu );
    addStandardItems( mnu );
    const int mnuid = mnu.exec();

    if ( mnuid==0 )
	addChild( new uiODRandomLineTreeItem(-1), false );
    else if ( mnuid==8 )
	addChild( new uiODRandomLineTreeItem(-1,uiODRandomLineTreeItem::RGBA),
		  false );
    if ( mnuid>=1 && mnuid<4 )
	genRandLine( mnuid-1 );
    else if ( mnuid == 4 )
	genRandLineFromWell();
    else if ( mnuid == 5 )
	genRandLineFromTable();
    else if ( mnuid == 6 )
	genRandomLineFromPickPolygon();
    else if ( mnuid==7 || mnuid==9 )
    {
	const IOObj* ioobj = selRandomLine();
	if ( ioobj )
	{
	    uiODRandomLineTreeItem::Type tp = mnuid==7
		? uiODRandomLineTreeItem::Empty : uiODRandomLineTreeItem::RGBA;
	    load( *ioobj, (int)tp );
	}
    }

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


bool uiODRandomLineParentTreeItem::load( const IOObj& ioobj, int tp )
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
	uiODRandomLineTreeItem* itm =
	    new uiODRandomLineTreeItem( -1, (uiODRandomLineTreeItem::Type)tp );
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
	load( *ioobj, (int)uiODRandomLineTreeItem::Empty );
    }
}


void uiODRandomLineParentTreeItem::genRandLineFromWell()
{
    applMgr()->wellServer()->setSceneID( sceneID() );
    applMgr()->wellServer()->selectWellCoordsForRdmLine();
    applMgr()->wellServer()->randLineDlgClosed.notify(
	    mCB(this,uiODRandomLineParentTreeItem,loadRandLineFromWell) );
}


void uiODRandomLineParentTreeItem::genRandLineFromTable()
{
    uiDialog dlg( getUiParent(),
	    	  uiDialog::Setup("Random lines","Specify node positions",
		      		  "109.0.4") );
    uiPositionTable* table = new uiPositionTable( &dlg, true, true, true );
    Interval<float> zrg = SI().zRange(true);
    zrg.scale( mCast(float,SI().zDomain().userFactor()) );
    table->setZRange( zrg );

    if ( dlg.go() )
    {
	uiODRandomLineTreeItem* itm = new uiODRandomLineTreeItem(-1);
	addChild( itm, false );
	mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
	    ODMainWin()->applMgr().visServer()->getObject(itm->displayID()));
	if ( !rtd ) return;

	TypeSet<BinID> newbids;
	table->getBinIDs( newbids );
	rtd->setKnotPositions( newbids );

	table->getZRange( zrg );
	zrg.scale( 1.f/SI().zDomain().userFactor() );
	rtd->setDepthInterval( zrg );
    }
}


void uiODRandomLineParentTreeItem::genRandomLineFromPickPolygon()
{
    if ( rdlpolylinedlg_ )
	return;

    ODMainWin()->applMgr().visServer()->setViewMode( false );
    uiODRandomLineTreeItem* itm = new uiODRandomLineTreeItem(-1);
    addChild( itm, false );
   
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
        ODMainWin()->applMgr().visServer()->getObject(itm->displayID()));

    rdlpolylinedlg_ = new uiRandomLinePolyLineDlg( getUiParent(), rtd );
    rdlpolylinedlg_->windowClosed.notify(
	mCB(this,uiODRandomLineParentTreeItem,rdlPolyLineDlgCloseCB) );
    rdlpolylinedlg_->go();
}


void uiODRandomLineParentTreeItem::rdlPolyLineDlgCloseCB( CallBacker* )
{
    if ( !rdlpolylinedlg_->uiResult() )
    {
	const int id = rdlpolylinedlg_->getDisplayID();
	removeChild( findChild(id) );
	ODMainWin()->applMgr().visServer()->removeObject( id, sceneID() );
    }

    rdlpolylinedlg_ = 0;
}


void uiODRandomLineParentTreeItem::loadRandLineFromWell( CallBacker* )
{
    const char* multiid =  applMgr()->wellServer()->getRandLineMultiID();
    if ( multiid && applMgr()->wellServer()->dispLineOnCreation() )
    {
	PtrMan<IOObj> ioobj = IOM().get( multiid );
	load( *ioobj, (int)uiODRandomLineTreeItem::Empty );
    }

    applMgr()->wellServer()->randLineDlgClosed.remove(
	    mCB(this,uiODRandomLineParentTreeItem,loadRandLineFromWell) );
}


uiODRandomLineTreeItem::uiODRandomLineTreeItem( int id, Type tp )
    : type_(tp)
    , editnodesmnuitem_("&Edit nodes ...")
    , insertnodemnuitem_("&Insert node")
    , saveasmnuitem_("&Save As ...")
    , saveas2dmnuitem_("Save As &2D ...")
    , create2dgridmnuitem_("Create 2D &Grid ...")
{
    editnodesmnuitem_.iconfnm = "orientation64";
    saveasmnuitem_.iconfnm = "saveas";
    displayid_ = id;
}


bool uiODRandomLineTreeItem::init()
{
    visSurvey::RandomTrackDisplay* rtd = 0;
    if ( displayid_==-1 )
    {
	rtd = visSurvey::RandomTrackDisplay::create();
	if ( type_ == RGBA )
	{
	    rtd->setChannels2RGBA( visBase::RGBATextureChannel2RGBA::create() );
	    rtd->addAttrib();
	    rtd->addAttrib();
	    rtd->addAttrib();
	}

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


void uiODRandomLineTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || menu->menuID()!=displayID() || istb )
	return;

    mAddMenuItem( menu, &create2dgridmnuitem_, true, false );

    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
		    visserv_->getObject(displayid_));
    if (  rtd->nrKnots() <= 0 ) return;
    const bool islocked = rtd->isGeometryLocked() || rtd->isLocked();
    mAddMenuItem( &displaymnuitem_, &editnodesmnuitem_, !islocked, false );
    mAddMenuItem( &displaymnuitem_, &insertnodemnuitem_, !islocked, false );
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

    mAddMenuItem( menu, &saveasmnuitem_, true, false );
    mAddMenuItem( menu, &saveas2dmnuitem_, true, false );
}


void uiODRandomLineTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
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
	    rln.set( 0, false ); //rln belongs to lset now.
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
	    rln->setName( rtd->name() );
	    uiCreate2DGrid dlg( ODMainWin(), rln );
	    dlg.go();
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
	    	  uiDialog::Setup("Random lines","Specify node positions",
		      		  "109.0.4") );
    uiPositionTable* table = new uiPositionTable( &dlg, true, true, true );
    table->setBinIDs( bids );

    Interval<float> zrg = rtd->getDataTraceRange();
    zrg.scale( mCast(float,SI().zDomain().userFactor() ) );
    table->setZRange( zrg );
    if ( dlg.go() )
    {
	TypeSet<BinID> newbids;
	table->getBinIDs( newbids );
	rtd->setKnotPositions( newbids );

	table->getZRange( zrg );
	zrg.scale( 1.f/SI().zDomain().userFactor() );
	rtd->setDepthInterval( zrg );

	visserv_->setSelObjectId( rtd->id() );
	for ( int attrib=0; attrib<visserv_->getNrAttribs(rtd->id()); attrib++ )
	    visserv_->calculateAttrib( rtd->id(), attrib, false );

	ODMainWin()->sceneMgr().updateTrees();
    }
}


void uiODRandomLineTreeItem::remove2DViewerCB( CallBacker* )
{
    ODMainWin()->viewer2DMgr().remove2DViewer( displayid_ );
}
