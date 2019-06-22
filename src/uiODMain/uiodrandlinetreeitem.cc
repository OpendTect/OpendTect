/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		May 2006
___________________________________________________________________

-*/


#include "uiodrandlinetreeitem.h"

#include "attribsel.h"
#include "attribprobelayer.h"
#include "ctxtioobj.h"
#include "mousecursor.h"
#include "ptrman.h"
#include "probemanager.h"
#include "randomlinetr.h"
#include "randomlineprobe.h"
#include "randomlinegeom.h"
#include "randcolor.h"
#include "survinfo.h"
#include "trigonometry.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uicreate2dgrid.h"
#include "uidialog.h"
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
			uiString::empty(),
			mODHelpKey(mRandomLinePolyLineDlgHelpID) )
		 .modal(false))
    , rtd_(rtd)
{
    showAlwaysOnTop();

    label_ = new uiLabel( this,
	tr("Pick Nodes on Z-Slices, Horizons, or Survey Inner Top/Bottom.\n"
	   "(Shift-Click for Outer Top/Bottom)") );

    colsel_ = new uiColorInput( this, uiColorInput::Setup(getRandStdDrawColor())
				      .lbltxt(uiStrings::sColor()) );
    colsel_->attach( alignedBelow, label_ );
    colsel_->colorChanged.notify(
	    mCB(this,uiRandomLinePolyLineDlg,colorChangeCB) );

    rtd_->removeAllNodes();
    rtd->setPolyLineMode( true );
    rtd_->setColor( colsel_->color() );
}


void colorChangeCB( CallBacker* )
{ rtd_->setColor( colsel_->color() ); }


bool acceptOK()
{
    if ( !rtd_->createFromPolyLine() )
    {
	uiMSG().error(uiStrings::phrSelect(tr("at least two points"
			 " on Z-Slice, Horizon, or Survey Top/Bottom")));
	return false;
    }
    rtd_->setPolyLineMode( false );
    return true;
}


bool rejectOK()
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


static int sInteractiveMenuID()			{ return 4; }
static int sFromExistingMenuID()		{ return 6; }
static int sFromPolygonMenuID()			{ return 7; }
static int sFromTableMenuID()			{ return 8; }
static int sFromWellMenuID()			{ return 9; }

// Tree Items
uiTreeItem*
    uiODRandomLineTreeItemFactory::createForVis( int visid, uiTreeItem* ) const
{
    pErrMsg( "Deprecated , to be removed later" );
    return 0;
}


uiODRandomLineParentTreeItem::uiODRandomLineParentTreeItem()
    : uiODSceneProbeParentTreeItem( uiStrings::sRandomLine() )
    , rdlpolylinedlg_(0)
    , rdltobeaddedid_(-1)
    , visrdltobeaddedid_(-1)
{
}


const char* uiODRandomLineParentTreeItem::iconName() const
{
    return "tree-randomline";
}


const char* uiODRandomLineParentTreeItem::childObjTypeKey() const
{
    return ProbePresentationInfo::sFactoryKey();
}


bool uiODRandomLineParentTreeItem::setProbeToBeAddedParams( int mnuid )
{
    rdlprobetobeadded_ = 0;
    visrdltobeaddedid_ = -1;
    rdltobeaddedid_ = -1;
    typetobeadded_ = uiODSceneProbeParentTreeItem::DefaultData;

    if ( isSceneAddMnuId(mnuid) )
    {
	if ( mnuid==getMenuID(uiODSceneProbeParentTreeItem::Select) )
	    return setSelRDLID();
	typetobeadded_ = getAddType(mnuid);
	return true;
    }
    else if ( mnuid == sInteractiveMenuID() )
	{ setRDLFromPicks(); return false; }
    else if ( mnuid==5 )
	return setRDLIDFromContours();
    else if ( mnuid==sFromExistingMenuID() )
	return setRDLIDFromExisting();
    else if ( mnuid==sFromPolygonMenuID() )
	return setRDLIDFromPolygon();
    else if ( mnuid == sFromTableMenuID() )
	return setRDLFromTable();
    else if ( mnuid == sFromWellMenuID() )
	{ setRDLFromWell(); return false; }

    return false;
}


uiPresManagedTreeItem* uiODRandomLineParentTreeItem::addChildItem(
	const Presentation::ObjInfo& prinfo )
{
    mDynamicCastGet(const ProbePresentationInfo*,probeprinfo,&prinfo)
    if ( !probeprinfo )
	return 0;

    RefMan<Probe> probe = ProbeMGR().fetchForEdit( probeprinfo->storedID() );
    mDynamicCastGet(RandomLineProbe*,rdlprobe,probe.ptr())
    if ( !rdlprobe )
	return 0;

    uiODRandomLineTreeItem* rdlitem =
	new uiODRandomLineTreeItem( *probe, visrdltobeaddedid_ );
    addChild( rdlitem, false );
    return rdlitem;
}


Probe* uiODRandomLineParentTreeItem::createNewProbe() const
{
    if ( rdlprobetobeadded_ )
	return rdlprobetobeadded_;
    return new RandomLineProbe( rdltobeaddedid_ );
}


void uiODRandomLineParentTreeItem::addMenuItems()
{
    uiODSceneProbeParentTreeItem::addMenuItems();

    uiAction* storedmenu = menu_->findAction(
		getMenuID(uiODSceneProbeParentTreeItem::Select) );
    if ( !storedmenu )
	return;

    storedmenu->setText( tr("Add Stored") );

    uiMenu* newmnu = new uiMenu( getUiParent(), uiStrings::sNew() );
    newmnu->setIcon( "addnew" );
    menu_->addMenu( newmnu );

#   define mAddItm( str, id, ic ) \
    newmnu->insertAction( new uiAction(m3Dots(str),ic), id );
    mAddItm( tr("Interactive"), sInteractiveMenuID(), "interaction" );
    mAddItm( tr("From Existing"), sFromExistingMenuID(), "tree-randomline" );
    mAddItm( tr("From Polygon"), sFromPolygonMenuID(), "polygon" );
    mAddItm( tr("From Table"), sFromTableMenuID(), "table" );
    mAddItm( tr("From Wells"), sFromWellMenuID(), "well" );
}


bool uiODRandomLineParentTreeItem::setSelRDLID()
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj( RandomLineSet );
    ctio->ctxt_.forread_ = true;
    uiIOObjSelDlg dlg( getUiParent(), *ctio );
    if ( !dlg.go() || !dlg.ioObj() )
	return false;

    const IOObj* ioobj = dlg.ioObj();
    rdltobeaddedid_ = Geometry::RLM().get( ioobj->key() )->ID();
    typetobeadded_ = uiODSceneProbeParentTreeItem::DefaultData;
    visrdltobeaddedid_ = -1;
    return true;
}


bool uiODRandomLineParentTreeItem::setRDLID( int opt )
{
    DBKey dbkey = applMgr()->EMServer()->genRandLine( opt );
    if ( !dbkey.isValid() || !applMgr()->EMServer()->dispLineOnCreation() )
	return false;

    Geometry::RandomLine* newrdl = Geometry::RLM().get( dbkey );
    if ( !newrdl )
	return false;

    typetobeadded_ = uiODSceneProbeParentTreeItem::DefaultData;
    visrdltobeaddedid_ = -1;
    rdltobeaddedid_ = newrdl->ID();
    return true;
}


bool uiODRandomLineParentTreeItem::setRDLIDFromExisting()
{ return setRDLID( 0 ); }

bool uiODRandomLineParentTreeItem::setRDLIDFromContours()
{ return setRDLID( 1 ); }

bool uiODRandomLineParentTreeItem::setRDLIDFromPolygon()
{ return setRDLID( 2 ); }


void uiODRandomLineParentTreeItem::setRDLFromWell()
{
    applMgr()->wellServer()->selectWellCoordsForRdmLine();
    applMgr()->wellServer()->randLineDlgClosed.notify(
	    mCB(this,uiODRandomLineParentTreeItem,loadRandLineFromWellCB) );
}


bool uiODRandomLineParentTreeItem::setRDLFromTable()
{
    uiDialog dlg( getUiParent(),
		  uiDialog::Setup(tr("Random lines"),
				  tr("Specify node positions"),
				  mODHelpKey(mODRandomLineTreeItemHelpID) ) );
    uiPositionTable* table = new uiPositionTable( &dlg, true, true, true );
    Interval<float> zrg = SI().zRange( OD::UsrWork );
    zrg.scale( mCast(float,SI().zDomain().userFactor()) );
    table->setZRange( zrg );

    if ( !dlg.go() )
	return false;

    Geometry::RandomLine* newrdl = RandomLineProbe::createNewDefaultRDL();
    TypeSet<BinID> newbids;
    table->getBinIDs( newbids );

    newrdl->setNodePositions( newbids );
    table->getZRange( zrg );
    zrg.scale( 1.f/SI().zDomain().userFactor() );
    newrdl->setZRange( zrg );

    typetobeadded_ = uiODSceneProbeParentTreeItem::DefaultData;
    visrdltobeaddedid_ = -1;
    rdltobeaddedid_ = newrdl->ID();
    return true;
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


void uiODRandomLineParentTreeItem::setRDLFromPicks()
{
    if ( rdlpolylinedlg_ )
	return;

    ODMainWin()->applMgr().visServer()->setViewMode( false );

    rdlprobetobeadded_ = new RandomLineProbe();
    visSurvey::RandomTrackDisplay* rtd = new visSurvey::RandomTrackDisplay();
    rtd->setProbe( rdlprobetobeadded_ );
    ODMainWin()->applMgr().visServer()->addObject( rtd, sceneID(), true );
    rtd->select();
    typetobeadded_ = uiODSceneProbeParentTreeItem::DefaultData;
    visrdltobeaddedid_ = rtd->id();
    rdltobeaddedid_ = -1;
    rdlpolylinedlg_ = new uiRandomLinePolyLineDlg( getUiParent(), rtd );
    rdlpolylinedlg_->windowClosed.notify(
	mCB(this,uiODRandomLineParentTreeItem,rdlPolyLineDlgCloseCB) );
    rdlpolylinedlg_->go();
}


void uiODRandomLineParentTreeItem::rdlPolyLineDlgCloseCB( CallBacker* )
{
    if ( rdlpolylinedlg_->uiResult() )
	addChildProbe();
    else
	ODMainWin()->applMgr().visServer()->removeObject(
		visrdltobeaddedid_, sceneID() );

    rdlpolylinedlg_ = 0;
}


void uiODRandomLineParentTreeItem::loadRandLineFromWellCB( CallBacker* )
{
    const DBKey dbkey =  applMgr()->wellServer()->getRandLineDBKey();
    if ( dbkey.isValid() && applMgr()->wellServer()->dispLineOnCreation() )
    {
	Geometry::RandomLine* newrdl = Geometry::RLM().get( dbkey );
	if ( !newrdl )
	    return;

	rdltobeaddedid_ = newrdl->ID();
	typetobeadded_ = uiODSceneProbeParentTreeItem::DefaultData;
	visrdltobeaddedid_ = -1;
	addChildProbe();
    }

    applMgr()->wellServer()->randLineDlgClosed.remove(
	    mCB(this,uiODRandomLineParentTreeItem,loadRandLineFromWellCB) );
}


uiODRandomLineTreeItem::uiODRandomLineTreeItem( Probe& probe, int id )
    : uiODSceneProbeTreeItem(probe)
    , editnodesmnuitem_(m3Dots(uiStrings::sPosition()))
    , insertnodemnuitem_(tr("Insert Node"))
    , saveasmnuitem_(m3Dots(uiStrings::sSaveAs()))
    , saveas2dmnuitem_(m3Dots(tr("Save as 2D")))
    , create2dgridmnuitem_(m3Dots(tr("Create 2D Grid")))
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
	rtd = new visSurvey::RandomTrackDisplay;
	displayid_ = rtd->id();
	rtd->setProbe( getProbe() );
	visserv_->addObject( rtd, sceneID(), true );
    }
    else
    {
	mDynamicCastGet(visSurvey::RandomTrackDisplay*,disp,
			visserv_->getObject(displayid_));
	if ( !disp ) return false;
	rtd = disp;
    }

    if ( !rtd )
    {
	pErrMsg( "Huh ? No RandomTrackDisplay ?" );
	return false;
    }

    mAttachCB( *rtd->posChanged(), uiODRandomLineTreeItem::rdlGeomChanged );
    return uiODSceneProbeTreeItem::init();
}


#define mGetPickedPanelIdx( menu, rtd, panelidx ) \
    mDynamicCastGet( uiMenuHandler*, uimenuhandler, menu ); \
    int panelidx = -1; \
    if ( rtd && uimenuhandler && uimenuhandler->getPath() ) \
	panelidx = \
	    rtd->getClosestPanelIdx(uimenuhandler->getPickedPos().getXY() );

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
	mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneID()));
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

	TypeSet<BinID> bids; rtd->getAllNodePos( bids );
	RefMan<Geometry::RandomLine> rln = new Geometry::RandomLine;
	const Interval<float> rtdzrg = rtd->getDepthInterval();
	rln->setZRange( hasztf ? SI().zRange(OD::UsrWork) : rtdzrg );
	for ( int idx=0; idx<bids.size(); idx++ )
	    rln->addNode( bids[idx] );

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

	    uiString bs;
	    if ( !RandomLineSetTranslator::store(lset,ioobj,bs) )
		mTIUiMsg().error( bs );
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
	    uiCreate2DGrid dlg( ODMainWin(), rln );
	    dlg.go();
	}
    }
}


void uiODRandomLineTreeItem::editNodes()
{
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
		    visserv_->getObject(displayid_));
    if ( !rtd || !rtd->getRandomLine() )
	return;

    TypeSet<BinID> bids;
    rtd->getRandomLine()->getNodePositions( bids );
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
    mDynamicCastGet(RandomLineProbe*,rdlprobe,getProbe());
    rdlprobe->geomUpdated();
    rtd->annotateNextUpdateStage( false );

    ODMainWin()->sceneMgr().updateTrees();
}


void uiODRandomLineTreeItem::rdlGeomChanged( CallBacker* cb )
{
    Probe* probe = getProbe();
    if ( !probe )
    { pErrMsg( "Huh! Shared Object not of type Probe" ); return; }

    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
		    visserv_->getObject(displayid_))
    if ( !rtd )
	return;

    rtd->annotateNextUpdateStage( true );
    rtd->acceptManipulation();
    rtd->annotateNextUpdateStage( true );
    mDynamicCastGet(RandomLineProbe*,rdlprobe,getProbe());
    rdlprobe->geomUpdated();
    rtd->annotateNextUpdateStage( false );
}


void uiODRandomLineTreeItem::remove2DViewerCB( CallBacker* )
{
    //TODO remove with probe related functions
    //ODMainWin()->viewer2DMgr().remove2DViewer( displayid_, true );
}


uiODRandomLineAttribTreeItem::uiODRandomLineAttribTreeItem( const char* ptype )
    : uiODAttribTreeItem(ptype)
{
}

uiODDataTreeItem* uiODRandomLineAttribTreeItem::create( ProbeLayer& prblay )
{
    const char* parenttype = typeid(uiODRandomLineTreeItem).name();
    uiODRandomLineAttribTreeItem* attribtreeitem =
	new uiODRandomLineAttribTreeItem( parenttype );
    attribtreeitem->setProbeLayer( &prblay );
    return attribtreeitem;

}


void uiODRandomLineAttribTreeItem::initClass()
{
    uiODDataTreeItem::fac().addCreateFunc(
	create, AttribProbeLayer::sFactoryKey(),RandomLineProbe::sFactoryKey());

}
