/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		May 2006
___________________________________________________________________

-*/


#include "uiodrandlinetreeitem.h"

#include "attribdescsetsholder.h"
#include "attribsel.h"
#include "ctxtioobj.h"
#include "ioman.h"
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
#include "uidialog.h"
#include "uiemattribpartserv.h"
#include "uiempartserv.h"
#include "uiioobjseldlg.h"
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
#include "uistrings.h"
#include "uitreeview.h"
#include "uivispartserv.h"
#include "uiwellpartserv.h"
#include "uiwellrdmlinedlg.h"
#include "uiseispartserv.h"
#include "visrandomtrackdisplay.h"
#include "visrgbatexturechannel2rgba.h"
#include "visselman.h"
#include "od_helpids.h"


class uiRandomLinePolyLineDlg : public uiDialog
{ mODTextTranslationClass(uiRandomLinePolyLineDlg)
public:
uiRandomLinePolyLineDlg(uiParent* p, visSurvey::RandomTrackDisplay* rtd )
    : uiDialog(p,Setup(tr("Create Random Line from Polyline"),
			uiString::emptyString(),
			mODHelpKey(mRandomLinePolyLineDlgHelpID) )
		 .modal(false))
    , rtd_(rtd)
{
    showAlwaysOnTop();

    label_ = new uiLabel( this,
	tr("Pick Nodes on Z-Slices, Horizons, or Survey Inner Top/Bottom.\n"
	   "(Shift-Click for Outer Top/Bottom)") );

    colsel_ = new uiColorInput( this,
				uiColorInput::Setup(OD::getRandStdDrawColor())
				      .lbltxt(uiStrings::sColor()) );
    colsel_->attach( alignedBelow, label_ );
    colsel_->colorChanged.notify(
	    mCB(this,uiRandomLinePolyLineDlg,colorChangeCB) );

    rtd_->removeAllNodes();
    rtd->setPolyLineMode( true );
    rtd_->setColor( colsel_->color() );
}


void colorChangeCB( CallBacker* )
{
    rtd_->setColor( colsel_->color() );
}


bool acceptOK( CallBacker* )
{
    if ( !rtd_->createFromPolyLine() )
    {
	uiMSG().error(tr("Please select at least two points"
			 " on Z-Slice, Horizon, or Survey Top/Bottom"));
	return false;
    }
    rtd_->setPolyLineMode( false );
    return true;
}


bool rejectOK( CallBacker* )
{
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


static uiODRandomLineTreeItem::Type getType( int mnuid )
{
    switch ( mnuid )
    {
	case 0: return uiODRandomLineTreeItem::Empty; break;
	case 2: return uiODRandomLineTreeItem::Select; break;
	case 1: case 3: return uiODRandomLineTreeItem::RGBA; break;
	default: return uiODRandomLineTreeItem::Empty;
    }
}


// Tree Items
uiTreeItem*
    uiODRandomLineTreeItemFactory::createForVis( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
		    ODMainWin()->applMgr().visServer()->getObject(visid));
    return rtd ? new uiODRandomLineTreeItem(visid) : 0;
}


uiODRandomLineParentTreeItem::uiODRandomLineParentTreeItem()
    : uiODParentTreeItem( uiStrings::sRandomLine() )
    , rdlpolylinedlg_(0)
{}


uiODRandomLineParentTreeItem::~uiODRandomLineParentTreeItem()
{}


const char* uiODRandomLineParentTreeItem::iconName() const
{ return "tree-randomline"; }


bool uiODRandomLineParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertAction( new uiAction(tr("Add Default Data")), 0 );
    mnu.insertAction( new uiAction(m3Dots(tr("Add Stored"))), 2 );

    uiMenu* rgbmnu =
	new uiMenu( getUiParent(), uiStrings::sAddColBlend() );
    rgbmnu->insertAction( new uiAction(tr("Empty")), 1 );
    rgbmnu->insertAction( new uiAction( m3Dots(uiStrings::sStored()) ), 3 );
    mnu.addMenu( rgbmnu );

    uiMenu* newmnu = new uiMenu( getUiParent(), uiStrings::sNew() );
    newmnu->insertAction( new uiAction(m3Dots(tr("Interactive "))), 4 );
//    newmnu->insertAction( new uiAction(m3Dots(tr("Along Contours"))), 5 );
    newmnu->insertAction( new uiAction(m3Dots(tr("From Existing"))), 6 );
    newmnu->insertAction( new uiAction(m3Dots(tr("From Polygon"))), 7 );
    newmnu->insertAction( new uiAction(m3Dots(tr("From Table"))), 8 );
    newmnu->insertAction( new uiAction(m3Dots(tr("From Wells"))), 9 );
    mnu.addMenu( newmnu );
    addStandardItems( mnu );
    const int mnuid = mnu.exec();

    if ( mnuid==0 )
    {
	auto* itm = new uiODRandomLineTreeItem(-1, getType(mnuid) );
	addChild( itm, false );
	itm->displayDefaultData();
    }
    else if ( mnuid==1 )
    {
	uiODRandomLineTreeItem* itm =
		new uiODRandomLineTreeItem(-1, getType(mnuid) );
	addChild( itm, false );
    }
    else if ( mnuid==2 || mnuid==3 )
	addStored( mnuid );
    else if ( mnuid == 4 )
	genFromPicks();
    else if ( mnuid==5 )
	genFromContours();
    else if ( mnuid==6 )
	genFromExisting();
    else if ( mnuid==7 )
	genFromPolygon();
    else if ( mnuid == 8 )
	genFromTable();
    else if ( mnuid == 9 )
	genFromWell();

    handleStandardItems( mnuid );
    return true;
}


bool uiODRandomLineParentTreeItem::addStored( int mnuid )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj( RandomLineSet );
    ctio->ctxt_.forread_ = true;
    uiIOObjSelDlg::Setup sdsu; sdsu.multisel( true );
    uiIOObjSelDlg dlg( getUiParent(), sdsu, *ctio );
    if ( !dlg.go() )
	return false;

    TypeSet<MultiID> keys;
    dlg.getChosen( keys );
    for ( int idx=0; idx<keys.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( keys[idx] );
	if ( ioobj )
	    load( *ioobj, mnuid );
    }

    return true;
}


bool uiODRandomLineParentTreeItem::load( const IOObj& ioobj, int mnuid )
{
    RefMan<Geometry::RandomLine> rl = Geometry::RLM().get( ioobj.key() );

    if ( !rl )
	return false;

    uiODRandomLineTreeItem* itm =
		new uiODRandomLineTreeItem( -1, getType(mnuid), rl->ID() );
    addChild( itm, false );
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
	    ODMainWin()->applMgr().visServer()->getObject(itm->displayID()));
    if ( !rtd )
	return false;

    rtd->setName( ioobj.name() );
    itm->displayDefaultData();

    updateColumnText( uiODSceneMgr::cNameColumn() );
    return true;
}


void uiODRandomLineParentTreeItem::genRandLine( int opt )
{
    const MultiID multiid = applMgr()->EMServer()->genRandLine( opt );
    PtrMan<IOObj> ioobj = IOM().get( multiid );
    if ( ioobj && applMgr()->EMServer()->dispLineOnCreation() )
	load( *ioobj, (int)uiODRandomLineTreeItem::Empty );
}


void uiODRandomLineParentTreeItem::genFromExisting()
{ genRandLine( 0 ); }

void uiODRandomLineParentTreeItem::genFromContours()
{ genRandLine( 1 ); }

void uiODRandomLineParentTreeItem::genFromPolygon()
{ genRandLine( 2 ); }


void uiODRandomLineParentTreeItem::genFromWell()
{
    applMgr()->wellServer()->selectWellCoordsForRdmLine();
    applMgr()->wellServer()->randLineDlgClosed.notify(
	    mCB(this,uiODRandomLineParentTreeItem,loadRandLineFromWell) );
}


void uiODRandomLineParentTreeItem::genFromTable()
{
    uiDialog dlg( getUiParent(),
		  uiDialog::Setup(tr("Random lines"),
				  tr("Specify node positions"),
				  mODHelpKey(mODRandomLineTreeItemHelpID) ) );
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
	if ( !rtd || !rtd->getRandomLine() ) return;

	TypeSet<BinID> newbids;
	table->getBinIDs( newbids );

	NotifyStopper notifystopper( visBase::DM().selMan().updateselnotifier );
	rtd->getRandomLine()->setNodePositions( newbids );
	notifystopper.enableNotification();

	table->getZRange( zrg );
	zrg.scale( 1.f/SI().zDomain().userFactor() );
	rtd->setDepthInterval( zrg );
	itm->displayDefaultData();
    }
}


void uiODRandomLineParentTreeItem::removeChild( uiTreeItem* item )
{
    if ( rdlpolylinedlg_ )
    {
	mDynamicCastGet(uiODDisplayTreeItem*,dispitem,item)
	if ( dispitem &&
	     (dispitem->displayID() == rdlpolylinedlg_->getDisplayID()) )
	{ delete rdlpolylinedlg_; rdlpolylinedlg_ = 0; }
    }

    uiTreeItem::removeChild( item );
}


void uiODRandomLineParentTreeItem::genFromPicks()
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
    const int id = rdlpolylinedlg_->getDisplayID();
    mDynamicCastGet(uiODRandomLineTreeItem*,itm,findChild(id))
    if ( !rdlpolylinedlg_->uiResult() )
    {
	removeChild( itm );
	ODMainWin()->applMgr().visServer()->removeObject( id, sceneID() );
    }
    else
	itm->displayDefaultData();

    rdlpolylinedlg_ = 0;
}


void uiODRandomLineParentTreeItem::loadRandLineFromWell( CallBacker* )
{
    const MultiID multiid =  applMgr()->wellServer()->getRandLineMultiID();
    PtrMan<IOObj> ioobj = IOM().get( multiid );
    if ( ioobj && applMgr()->wellServer()->dispLineOnCreation() )
	load( *ioobj, (int)uiODRandomLineTreeItem::Empty );

    applMgr()->wellServer()->randLineDlgClosed.remove(
	    mCB(this,uiODRandomLineParentTreeItem,loadRandLineFromWell) );
}


uiODRandomLineTreeItem::uiODRandomLineTreeItem( int id, Type tp, int rlid )
    : type_(tp)
    , rlid_(rlid)
    , editnodesmnuitem_(m3Dots(tr("Position")))
    , insertnodemnuitem_(tr("Insert Node"))
    , saveasmnuitem_(m3Dots(uiStrings::sSaveAs()))
    , saveas2dmnuitem_(m3Dots(tr("Save as 2D")))
    , create2dgridmnuitem_(m3Dots(tr("Create 2D Grid")))
{
    editnodesmnuitem_.iconfnm = "orientation64";
    saveasmnuitem_.iconfnm = "saveas";
    displayid_ = id;
}


uiODRandomLineTreeItem::~uiODRandomLineTreeItem()
{}


bool uiODRandomLineTreeItem::init()
{
    visSurvey::RandomTrackDisplay* rtd = 0;
    if ( displayid_==-1 )
    {
	rtd = new visSurvey::RandomTrackDisplay;
	if ( type_ == RGBA )
	{
	    rtd->setChannels2RGBA( visBase::RGBATextureChannel2RGBA::create() );
	    rtd->addAttrib();
	    rtd->addAttrib();
	    rtd->addAttrib();
	}

	displayid_ = rtd->id();
	if ( rlid_ >= 0 )
	    setRandomLineID( rlid_ );
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


void uiODRandomLineTreeItem::setRandomLineID( int id )
{
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
		    visserv_->getObject(displayid_));
    if ( !rtd ) return;

    rtd->setRandomLineID( id );
}


bool uiODRandomLineTreeItem::displayDefaultData()
{
    Attrib::DescID descid;
    if ( !applMgr()->getDefaultDescID(descid) )
	return false;

    const Attrib::DescSet* ads =
	Attrib::DSHolder().getDescSet( false, true );
    Attrib::SelSpec as( 0, descid, false, "" );
    as.setRefFromID( *ads );
    visserv_->setSelSpec( displayid_, 0, as );
    const bool res = visserv_->calculateAttrib( displayid_, 0, false );
    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText( uiODSceneMgr::cColorColumn() );
    if ( !children_.isEmpty() )
    {
	children_[0]->select();
	children_[0]->select(); // hack to update toolbar
    }

    return res;
}


#define mGetPickedPanelIdx( menu, rtd, panelidx ) \
    mDynamicCastGet( uiMenuHandler*, uimenuhandler, menu ); \
    int panelidx = -1; \
    if ( rtd && uimenuhandler && uimenuhandler->getPath() ) \
	panelidx = rtd->getClosestPanelIdx(uimenuhandler->getPickedPos() );

#define mAddInsertNodeMnuItm( nodeidx, nodename ) \
    mAddManagedMenuItem( &insertnodemnuitem_, new MenuItem(nodename), \
			 rtd->canAddNode(nodeidx), false )

void uiODRandomLineTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || menu->menuID()!=displayID() )
	return;

    mAddMenuOrTBItem( istb, 0, menu, &create2dgridmnuitem_, true, false );

    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
		    visserv_->getObject(displayid_));
    if ( !rtd || rtd->nrNodes() <= 0 )
	return;

    const bool islocked = rtd->isGeometryLocked() || rtd->isLocked();
    mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &editnodesmnuitem_,
		      !islocked, false );
    mAddMenuOrTBItem( istb, 0, &displaymnuitem_, &insertnodemnuitem_,
		      !islocked, false );
    insertnodemnuitem_.removeItems();

    if ( !islocked && rtd->nrNodes()>1 )
    {
	mGetPickedPanelIdx( menu, rtd, panelidx );
	if ( panelidx >=0 )
	{
	    if ( panelidx == 0 )
		mAddInsertNodeMnuItm( panelidx, tr("before clicked panel") );
	    if ( panelidx==0 || panelidx==rtd->nrNodes()-2 )
		mAddInsertNodeMnuItm( panelidx+1, tr("on clicked panel") );
	    if ( panelidx == rtd->nrNodes()-2 )
		mAddInsertNodeMnuItm( panelidx+2, tr("after clicked panel") );
	}
	else if ( rtd->nrNodes() <= 20 )
	{
	    for ( int idx=0; idx<=rtd->nrNodes(); idx++ )
	    {
		if ( idx == 0 )
		    mAddInsertNodeMnuItm( idx, tr("before first node") )
		else if ( idx == rtd->nrNodes() )
		    mAddInsertNodeMnuItm( idx, tr("after last node") )
		else
		    mAddInsertNodeMnuItm( idx,
			    tr("between node %1 && %2").arg(idx).arg(idx+1) );
	    }
	}
	else				// too many nodes for tree menu
	    mAddInsertNodeMnuItm( -1,
			tr("<use right-click menu of random line in scene>") );
    }

    mAddMenuOrTBItem( istb, 0, menu, &saveasmnuitem_, true, false );
    mAddMenuOrTBItem( istb, 0, menu, &saveas2dmnuitem_, true, false );
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
    if ( !rtd ) return;

    mGetPickedPanelIdx( menu, rtd, panelidx );

    if ( mnuid==editnodesmnuitem_.id )
    {
	editNodes();
	menu->setIsHandled( true );
    }
    else if ( mnuid==insertnodemnuitem_.id )
    {
	mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneID()))
	if ( scene ) scene->selectPosModeManipObj( displayid_ );

	rtd->addNode( panelidx+1 );
	menu->setIsHandled( true );
    }
    else if ( insertnodemnuitem_.itemIndex(mnuid)!=-1 )
    {
	int nodeidx = insertnodemnuitem_.itemIndex( mnuid );
	if ( panelidx >= 0 )
	    nodeidx += panelidx==0 ? panelidx : panelidx+1;

	mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneID()))
	if ( scene ) scene->selectPosModeManipObj( displayid_ );

	rtd->addNode( nodeidx );
	menu->setIsHandled( true );
    }
    else
    {
	mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneID()))
	const bool hasztf = scene && scene->getZAxisTransform();

	TrcKeyPath nodes; rtd->getAllNodePos( nodes );
	RefMan<Geometry::RandomLine> rln = new Geometry::RandomLine;
	const Interval<float> rtdzrg = rtd->getDepthInterval();
	rln->setZRange( hasztf ? SI().zRange(false) : rtdzrg );
	for ( const auto& node : nodes )
	    rln->addNode( node.position() );

	if ( mnuid == saveasmnuitem_.id )
	{
	    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj( RandomLineSet );
	    ctio->ctxt_.forread_ = false;
	    uiIOObjSelDlg dlg( getUiParent(), *ctio );
	    if ( !dlg.go() ) return;

	    Geometry::RandomLineSet lset;
	    lset.addLine( *rln );

	    const IOObj* ioobj = dlg.ioObj();
	    if ( !ioobj ) return;

	    BufferString bs;
	    if ( !RandomLineSetTranslator::store(lset,ioobj,bs) )
		uiMSG().error( mToUiStringTodo(bs) );
	    else
	    {
        const BufferString rdlname = ioobj->name();
        applMgr()->visServer()->setObjectName( displayID(), rdlname );
        rtd->getRandomLine()->setName( rdlname );

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
	    applMgr()->EMAttribServer()->create2DGrid( rln );
	}
    }
}


void uiODRandomLineTreeItem::editNodes()
{
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
		    visserv_->getObject(displayid_));
    if ( !rtd || !rtd->getRandomLine() )
	return;

    TrcKeyPath nodes;
    rtd->getRandomLine()->allNodePositions( nodes );
    TypeSet<BinID> bids;
    for ( const auto& tk : nodes )
	bids += tk.position();

    uiDialog dlg( getUiParent(),
	  uiDialog::Setup(uiStrings::sRandomLine(mPlural),
	      tr("Specify node positions"),
	      mODHelpKey(mODRandomLineTreeItemHelpID) ) );
    uiPositionTable* table = new uiPositionTable( &dlg, true, true, true );
    table->setBinIDs( bids );

    Interval<float> zrg = rtd->getDataTraceRange();
    zrg.scale( mCast(float,SI().zDomain().userFactor() ) );
    table->setZRange( zrg );
    if ( !dlg.go() )
	return;

    rtd->annotateNextUpdateStage( true );

    TypeSet<BinID> newbids;
    table->getBinIDs( newbids );

    NotifyStopper notifystopper( visBase::DM().selMan().updateselnotifier );
    rtd->getRandomLine()->setNodePositions( newbids );
    notifystopper.enableNotification();

    table->getZRange( zrg );
    zrg.scale( 1.f/SI().zDomain().userFactor() );
    rtd->setDepthInterval( zrg );

    visserv_->setSelObjectId( rtd->id() );
    rtd->annotateNextUpdateStage( true );
    for ( int attrib=0; attrib<visserv_->getNrAttribs(rtd->id()); attrib++ )
	visserv_->calculateAttrib( rtd->id(), attrib, false );
    rtd->annotateNextUpdateStage( false );

    ODMainWin()->sceneMgr().updateTrees();
}


void uiODRandomLineTreeItem::remove2DViewerCB( CallBacker* )
{
    ODMainWin()->viewer2DMgr().remove2DViewer( displayid_, true );
}
