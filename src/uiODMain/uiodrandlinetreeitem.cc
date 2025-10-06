/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodrandlinetreeitem.h"

#include "attribdescsetsholder.h"
#include "attribsel.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "od_helpids.h"
#include "ptrman.h"
#include "randcolor.h"
#include "randomlinegeom.h"
#include "randomlinetr.h"
#include "survinfo.h"
#include "visrgbatexturechannel2rgba.h"
#include "visselman.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uidialog.h"
#include "uiemattribpartserv.h"
#include "uiempartserv.h"
#include "uigisexp.h"
#include "uiioobjseldlg.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiodviewer2dmgr.h"
#include "uipickpartserv.h"
#include "uipositiontable.h"
#include "uiseispartserv.h"
#include "uistrings.h"
#include "uivispartserv.h"
#include "uivisslicepos3d.h"
#include "uiwellpartserv.h"
#include "uiwellrdmlinedlg.h"


static const int cAddDefIdx	= 0;
static const int cAddStoredIdx	= 2;
static const int cAddRGBNewIdx		= 31;
static const int cAddRGBStoredIdx	= 32;
static const int cNewInteractiveIdx	= 41;
static const int cNewFromContoursIdx	= 42;
static const int cNewFromExistingIdx	= 43;
static const int cNewFromPolygonIdx	= 44;
static const int cNewFromTableIdx	= 45;
static const int cNewFromWellsIdx	= 46;
static const int cExpGISIdx	= 5;


class uiRandomLinePolyLineDlg : public uiDialog
{ mODTextTranslationClass(uiRandomLinePolyLineDlg)
public:
uiRandomLinePolyLineDlg( uiParent* p, visSurvey::RandomTrackDisplay* rtd )
    : uiDialog(p,Setup(tr("Create Random Line from Polyline"),
		       uiString::emptyString(),
		       mODHelpKey(mRandomLinePolyLineDlgHelpID))
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
    mAttachCB( colsel_->colorChanged, uiRandomLinePolyLineDlg::colorChangeCB );

    rtd_->removeAllNodes();
    rtd->setPolyLineMode( true );
    rtd_->setColor( colsel_->color() );
}


~uiRandomLinePolyLineDlg()
{
    detachAllNotifiers();
}


void colorChangeCB( CallBacker* )
{
    rtd_->setColor( colsel_->color() );
}


bool acceptOK( CallBacker* ) override
{
    showAlwaysOnTop( false );
    if ( !rtd_->createFromPolyLine() )
    {
	showAlwaysOnTop( true );
	showAndActivate();
	uiMSG().error(tr("Please select at least two points"
			 " on Z-Slice, Horizon, or Survey Top/Bottom"));
	return false;
    }
    rtd_->setPolyLineMode( false );
    return true;
}


bool rejectOK( CallBacker* ) override
{
    rtd_->setPolyLineMode( false );
    return true;
}


VisID getDisplayID() const
{ return rtd_->id(); }


const Attrib::SelSpec*	getSelSpec() const
{
    return rtd_->nrAttribs()>0 ? rtd_->getSelSpec( 0 ) : nullptr;
}


private:

    RefMan<visSurvey::RandomTrackDisplay> rtd_;
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
uiTreeItem* uiODRandomLineTreeItemFactory::createForVis( const VisID& visid,
							 uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
		    ODMainWin()->applMgr().visServer()->getObject(visid));
    return rtd ? new uiODRandomLineTreeItem(visid) : nullptr;
}


CNotifier<uiODRandomLineParentTreeItem,uiMenu*>&
	uiODRandomLineParentTreeItem::showMenuNotifier()
{
    static CNotifier<uiODRandomLineParentTreeItem,uiMenu*> notif( nullptr );
    return notif;
}


uiODRandomLineParentTreeItem::uiODRandomLineParentTreeItem()
    : uiODParentTreeItem( uiStrings::sRandomLine() )
{}


uiODRandomLineParentTreeItem::~uiODRandomLineParentTreeItem()
{
    detachAllNotifiers();
}


const char* uiODRandomLineParentTreeItem::iconName() const
{ return "tree-randomline"; }


bool uiODRandomLineParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertAction( new uiAction(tr("Add Default Data")), cAddDefIdx );
    mnu.insertAction( new uiAction(m3Dots(tr("Add Stored"))), cAddStoredIdx );

    auto* rgbmnu = new uiMenu( getUiParent(), uiStrings::sAddColBlend() );
    rgbmnu->insertAction( new uiAction(tr("Empty")), cAddRGBNewIdx );
    rgbmnu->insertAction( new uiAction( m3Dots(uiStrings::sStored()) ),
			  cAddRGBStoredIdx );
    mnu.addMenu( rgbmnu );

    auto* newmnu = new uiMenu( getUiParent(), uiStrings::sNew() );
    newmnu->insertAction( new uiAction(m3Dots(tr("Interactive "))),
			  cNewInteractiveIdx );
/*    newmnu->insertAction( new uiAction(m3Dots(tr("Along Contours"))),
			  cNewFromContoursIdx ); */
    newmnu->insertAction( new uiAction(m3Dots(tr("From Existing"))),
			  cNewFromExistingIdx );
    newmnu->insertAction( new uiAction(m3Dots(tr("From Polygon"))),
			  cNewFromPolygonIdx );
    newmnu->insertAction( new uiAction(m3Dots(tr("From Table"))),
			  cNewFromTableIdx );
    newmnu->insertAction( new uiAction(m3Dots(tr("From Wells"))),
			  cNewFromWellsIdx );
    mnu.addMenu( newmnu );

    if ( !children_.isEmpty() )
	mnu.insertAction( new uiAction(m3Dots(uiGISExpStdFld::sToolTipTxt()),
				uiGISExpStdFld::strIcon()), cExpGISIdx );

    showMenuNotifier().trigger( &mnu, this );

    addStandardItems( mnu );
    const int mnuid = mnu.exec();

    if ( mnuid==cAddDefIdx )
    {
	auto* itm = new uiODRandomLineTreeItem( VisID::udf(), getType(mnuid) );
	addChild( itm, false );
	itm->displayDefaultData();
    }
    else if ( mnuid==cAddRGBNewIdx )
    {
	auto* itm = new uiODRandomLineTreeItem( VisID::udf(), getType(mnuid) );
	addChild( itm, false );
    }
    else if ( mnuid==cAddStoredIdx || mnuid==cAddRGBStoredIdx )
	addStored( mnuid );
    else if ( mnuid == cNewInteractiveIdx )
	genFromPicks();
    else if ( mnuid==cNewFromContoursIdx )
	genFromContours();
    else if ( mnuid==cNewFromExistingIdx )
	genFromExisting();
    else if ( mnuid==cNewFromPolygonIdx )
	genFromPolygon();
    else if ( mnuid == cNewFromTableIdx )
	genFromTable();
    else if ( mnuid == cNewFromWellsIdx )
	genFromWell();
    else if ( mnuid == cExpGISIdx )
	exportToGIS();

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

    auto* itm = new uiODRandomLineTreeItem( VisID::udf(), getType(mnuid),
					    rl->ID() );
    addChild( itm, false );
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
		    applMgr()->visServer()->getObject(itm->displayID()))
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
    mAttachCB( applMgr()->wellServer()->randLineDlgClosed,
	       uiODRandomLineParentTreeItem::loadRandLineFromWell );
}


void uiODRandomLineParentTreeItem::exportToGIS()
{
    uiPickPartServer& pickserver = *applMgr()->pickServer();
    RefObjectSet<const Pick::Set> gisdatas;
    for ( const auto* child : children_ )
    {
	mDynamicCastGet(const uiODRandomLineTreeItem*,rdltreeitm,child)
	if ( !rdltreeitm )
	    continue;

	ConstRefMan<visSurvey::RandomTrackDisplay> rtd =
						rdltreeitm->getDisplay();
	if ( !rtd )
	    continue;

	const Geometry::RandomLine* rl = rtd->getRandomLine();
	if ( !rl || rl->size() < 2 )
	    continue;

	RefMan<Pick::Set> pickset = new Pick::Set();
	pickserver.convert( *rl,*pickset.ptr() );
	pickset->pars_.setYN( uiGISExportDlg::sKeyIsOn(), child->isSelected() );
	gisdatas.add( pickset.ptr() );
    }

    pickserver.exportRandomLinesToGIS( getUiParent(), gisdatas );
}


void uiODRandomLineParentTreeItem::genFromTable()
{
    uiDialog dlg( getUiParent(),
		  uiDialog::Setup(tr("Random lines"),
				  tr("Specify node positions"),
				  mODHelpKey(mODRandomLineTreeItemHelpID) ) );
    auto* table = new uiPositionTable( &dlg, true, true, true );
    Interval<float> zrg = SI().zRange(true);
    zrg.scale( mCast(float,SI().zDomain().userFactor()) );
    table->setZRange( zrg );

    if ( dlg.go() )
    {
	auto* itm = new uiODRandomLineTreeItem( VisID::udf());
	addChild( itm, false );
	mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
			applMgr()->visServer()->getObject(itm->displayID()));
	if ( !rtd || !rtd->getRandomLine() )
	    return;

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
	    deleteAndNullPtr( rdlpolylinedlg_ );
    }

    uiTreeItem::removeChild( item );
}


void uiODRandomLineParentTreeItem::genFromPicks()
{
    if ( rdlpolylinedlg_ )
	return;

    applMgr()->visServer()->setViewMode( false );
    auto* itm = new uiODRandomLineTreeItem( VisID::udf() );
    addChild( itm, false );

    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
		    applMgr()->visServer()->getObject(itm->displayID()));

    delete rdlpolylinedlg_;
    rdlpolylinedlg_ = new uiRandomLinePolyLineDlg( getUiParent(), rtd );
    mAttachCB( rdlpolylinedlg_->windowClosed,
	       uiODRandomLineParentTreeItem::rdlPolyLineDlgCloseCB );
    rdlpolylinedlg_->go();
}


void uiODRandomLineParentTreeItem::rdlPolyLineDlgCloseCB( CallBacker* )
{
    const VisID id = rdlpolylinedlg_->getDisplayID();
    mDynamicCastGet(uiODRandomLineTreeItem*,itm,findChild(id.asInt()))
    if ( rdlpolylinedlg_->uiResult() )
    {
	const auto* selspec = rdlpolylinedlg_->getSelSpec();
	itm->displayData( selspec );
    }
    else
    {
	removeChild( itm );
	applMgr()->visServer()->removeObject( id, sceneID() );
    }

    rdlpolylinedlg_ = nullptr;
}


void uiODRandomLineParentTreeItem::loadRandLineFromWell( CallBacker* )
{
    const MultiID multiid =  applMgr()->wellServer()->getRandLineMultiID();
    PtrMan<IOObj> ioobj = IOM().get( multiid );
    if ( ioobj && applMgr()->wellServer()->dispLineOnCreation() )
	load( *ioobj, (int)uiODRandomLineTreeItem::Empty );

    mDetachCB( applMgr()->wellServer()->randLineDlgClosed,
	       uiODRandomLineParentTreeItem::loadRandLineFromWell );
}


// uiODRandomLineTreeItem

uiODRandomLineTreeItem::uiODRandomLineTreeItem( const VisID& id, Type tp,
						const RandomLineID& rid )
    : editnodesmnuitem_(m3Dots(tr("Position")))
    , insertnodemnuitem_(tr("Insert Node"))
    , saveasmnuitem_(m3Dots(uiStrings::sSaveAs()))
    , saveas2dmnuitem_(m3Dots(tr("Save as 2D")))
    , create2dgridmnuitem_(m3Dots(tr("Create 2D Grid")))
    , type_(tp)
    , rlid_(rid)
{
    displayid_ = id;
    editnodesmnuitem_.iconfnm = "orientation64";
    saveasmnuitem_.iconfnm = "saveas";
}


uiODRandomLineTreeItem::~uiODRandomLineTreeItem()
{
    detachAllNotifiers();
    visserv_->removeObject( displayid_, sceneID() );
}


bool uiODRandomLineTreeItem::init()
{
    if ( !displayid_.isValid() )
    {
	RefMan<visSurvey::RandomTrackDisplay> rtd =
					new visSurvey::RandomTrackDisplay;
	displayid_ = rtd->id();
	if ( type_ == RGBA )
	{
	    RefMan<visBase::RGBATextureChannel2RGBA> text2rgba =
				visBase::RGBATextureChannel2RGBA::create();
	    rtd->setChannels2RGBA( text2rgba.ptr() );
	    rtd->addAttrib();
	    rtd->addAttrib();
	    rtd->addAttrib();
	}

	visserv_->addObject( rtd.ptr(), sceneID(), true);
    }

    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
		    visserv_->getObject(displayid_));
    if ( !rtd )
	return false;

    if ( rlid_.isValid() && rlid_ != rtd->getRandomLineID() )
	rtd->setRandomLineID( rlid_ );

    mAttachCB( rtd->selection(), uiODRandomLineTreeItem::selChg );
    mAttachCB( rtd->deSelection(), uiODRandomLineTreeItem::selChg );
    mAttachCB( visserv_->getUiSlicePos()->positionChg,
	       uiODRandomLineTreeItem::posChange );
    mAttachCB( *rtd->getMovementNotifier(),
	       uiODRandomLineTreeItem::remove2DViewerCB );
    mAttachCB( *rtd->getManipulationNotifier(),
	       uiODRandomLineTreeItem::remove2DViewerCB );

    rtd_ = rtd;
    return uiODDisplayTreeItem::init();
}


ConstRefMan<visSurvey::RandomTrackDisplay>
	uiODRandomLineTreeItem::getDisplay() const
{
    return rtd_.get();
}


RefMan<visSurvey::RandomTrackDisplay> uiODRandomLineTreeItem::getDisplay()
{
    return rtd_.get();
}


void uiODRandomLineTreeItem::setRandomLineID( const RandomLineID& id )
{
    RefMan<visSurvey::RandomTrackDisplay> rtd = getDisplay();
    if ( !rtd )
	return;

    rtd->setRandomLineID( id );
}


bool uiODRandomLineTreeItem::displayDefaultData()
{
    Attrib::DescID descid;
    if ( !applMgr()->getDefaultDescID(descid) )
	return false;

    const Attrib::DescSet* ads = Attrib::DSHolder().getDescSet( false, true );
    Attrib::SelSpec as( 0, descid, false, "" );
    as.setRefFromID( *ads );
    return displayData( &as );
}


bool uiODRandomLineTreeItem::displayData( const Attrib::SelSpec* selspec )
{
    if ( !selspec )
	return displayDefaultData();

    visserv_->setSelSpec( displayid_, 0, *selspec );
    const bool res = visserv_->calculateAttrib( displayid_, 0, false );
    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText( uiODSceneMgr::cColorColumn() );
    if ( !children_.isEmpty() )
    {
	children_[0]->select();
	children_[0]->select(); // hack to update toolbar
    }

    if ( res )
	applMgr()->useDefColTab( displayid_, 0 );

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
    if ( !menu || !isDisplayID(menu->menuID()) )
	return;

    mAddMenuOrTBItem( istb, 0, menu, &create2dgridmnuitem_, true, false );

    ConstRefMan<visSurvey::RandomTrackDisplay> rtd = getDisplay();
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


void uiODRandomLineTreeItem::selChg( CallBacker* cb )
{
    RefMan<visSurvey::RandomTrackDisplay> rtd = getDisplay();
    if ( !rtd )
	return;

    OD::SliceType orientation = rtd->getOrientation();
    if ( orientation!=OD::SliceType::Inline ||
	 orientation!=OD::SliceType::Crossline )
	visserv_->getUiSlicePos()->setDisplay( rtd->id() );
}


void uiODRandomLineTreeItem::posChange( CallBacker* cb )
{
    uiSlicePos3DDisp* slicepos = visserv_->getUiSlicePos();
    if ( slicepos->getDisplayID() != displayid_ )
	return;

    visserv_->moveRandomLineAndCalcAttribs( displayid_,
				       slicepos->getTrcKeyZSampling() );
}


void uiODRandomLineTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || !isDisplayID(menu->menuID()) || mnuid==-1 )
	return;

    RefMan<visSurvey::RandomTrackDisplay> rtd = getDisplay();
    if ( !rtd )
	return;

    mGetPickedPanelIdx( menu, rtd, panelidx );

    if ( mnuid==editnodesmnuitem_.id )
    {
	editNodes();
	menu->setIsHandled( true );
    }
    else if ( mnuid==insertnodemnuitem_.id )
    {
	RefMan<visSurvey::Scene> scene = visserv_->getScene( sceneID() );
	if ( scene ) scene->selectPosModeManipObj( displayid_ );

	rtd->addNode( panelidx+1 );
	menu->setIsHandled( true );
    }
    else if ( insertnodemnuitem_.itemIndex(mnuid)!=-1 )
    {
	int nodeidx = insertnodemnuitem_.itemIndex( mnuid );
	if ( panelidx >= 0 )
	    nodeidx += panelidx==0 ? panelidx : panelidx+1;

	RefMan<visSurvey::Scene> scene = visserv_->getScene( sceneID() );
	if ( scene ) scene->selectPosModeManipObj( displayid_ );

	rtd->addNode( nodeidx );
	menu->setIsHandled( true );
    }
    else
    {
	RefMan<visSurvey::Scene> scene = visserv_->getScene( sceneID() );
	const bool hasztf = scene && scene->getZAxisTransform();

	TrcKeySet nodes; rtd->getAllNodePos( nodes );
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
	    if ( !dlg.go() )
		return;

	    Geometry::RandomLineSet lset;
	    lset.addLine( *rln );

	    const IOObj* ioobj = dlg.ioObj();
	    if ( !ioobj )
		return;

	    uiString errmsg;
	    if ( !RandomLineSetTranslator::store(lset,ioobj,errmsg) )
		uiMSG().error( errmsg );
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
	    applMgr()->EMAttribServer()->create2DGrid( rln.ptr() );
	}
    }
}


void uiODRandomLineTreeItem::editNodes()
{
    RefMan<visSurvey::RandomTrackDisplay> rtd = getDisplay();
    if ( !rtd || !rtd->getRandomLine() )
	return;

    TrcKeySet nodes;
    rtd->getRandomLine()->allNodePositions( nodes );
    TypeSet<BinID> bids;
    for ( const auto& tk : nodes )
	bids += tk.position();

    uiDialog dlg( getUiParent(),
	  uiDialog::Setup(uiStrings::sRandomLine(mPlural),
			  tr("Specify node positions"),
			  mODHelpKey(mODRandomLineTreeItemHelpID)) );
    auto* table = new uiPositionTable( &dlg, true, true, true );
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
    if ( ODMainWin() )
	ODMainWin()->viewer2DMgr().remove2DViewer( displayid_ );
}
