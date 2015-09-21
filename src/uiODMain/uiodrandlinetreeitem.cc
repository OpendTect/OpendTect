/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		May 2006
___________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
#include "od_helpids.h"


class uiRandomLinePolyLineDlg : public uiDialog
{ mODTextTranslationClass(uiRandomLinePolyLineDlg)
public:
uiRandomLinePolyLineDlg(uiParent* p, visSurvey::RandomTrackDisplay* rtd )
    : uiDialog(p,Setup("Create Random Line from Polyline",
			uiString::emptyString(),
			mODHelpKey(mRandomLinePolyLineDlgHelpID) )
		 .modal(false))
    , rtd_(rtd)
{
    showAlwaysOnTop();

    label_ = new uiLabel( this, "Pick Nodes on Z-Slices or Horizons" );
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


bool acceptOK( CallBacker* )
{
    if ( !rtd_->createFromPolyLine() )
    {
	uiMSG().error(tr("Please select at least two points"
			 " on TimeSlice/Horizon"));
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
    : uiODTreeItem( uiStrings::sRandomLine() )
    , rdlpolylinedlg_(0)
{}


const char* uiODRandomLineParentTreeItem::iconName() const
{ return "tree-randomline"; }


bool uiODRandomLineParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(tr("Add Default Data")), 0 );
    mnu.insertItem( new uiAction(m3Dots(tr("Add Stored"))), 2 );

    uiMenu* rgbmnu =
	new uiMenu( getUiParent(), uiStrings::sAddColBlend() );
    rgbmnu->insertItem( new uiAction(tr("Empty")), 1 );
    rgbmnu->insertItem( new uiAction( m3Dots(uiStrings::sStored()) ), 3 );
    mnu.insertItem( rgbmnu );

    uiMenu* newmnu = new uiMenu( getUiParent(), uiStrings::sNew() );
    newmnu->insertItem( new uiAction(m3Dots(tr("Interactive "))), 4 );
//    newmnu->insertItem( new uiAction(m3Dots(tr("Along Contours"))), 5 );
    newmnu->insertItem( new uiAction(m3Dots(tr("From Existing"))), 6 );
    newmnu->insertItem( new uiAction(m3Dots(tr("From Polygon"))), 7 );
    newmnu->insertItem( new uiAction(m3Dots(tr("From Table"))), 8 );
    newmnu->insertItem( new uiAction(m3Dots(tr("From Wells"))), 9 );
    mnu.insertItem( newmnu );
    addStandardItems( mnu );
    const int mnuid = mnu.exec();

    if ( mnuid==0 || mnuid==1 )
    {
	uiODRandomLineTreeItem* itm =
		new uiODRandomLineTreeItem(-1, getType(mnuid) );
	addChild( itm, false );
	if ( mnuid==0 )
	    itm->displayDefaultData();
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
    uiIOObjSelDlg dlg( getUiParent(), *ctio );
    if ( !dlg.go() || !dlg.ioObj() ) return false;

    const IOObj* ioobj = dlg.ioObj();
    return load( *ioobj, mnuid );
}


bool uiODRandomLineParentTreeItem::load( const IOObj& ioobj, int mnuid )
{
    RefMan<Geometry::RandomLine> rl = Geometry::RLM().get( ioobj.key() );

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
    const char* multiid = applMgr()->EMServer()->genRandLine( opt );
    if ( multiid && applMgr()->EMServer()->dispLineOnCreation() )
    {
	PtrMan<IOObj> ioobj = IOM().get( multiid );
	load( *ioobj, (int)uiODRandomLineTreeItem::Empty );
    }
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
	if ( !rtd ) return;

	TypeSet<BinID> newbids;
	table->getBinIDs( newbids );
	rtd->setNodePositions( newbids );

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
    const char* multiid =  applMgr()->wellServer()->getRandLineMultiID();
    if ( multiid && applMgr()->wellServer()->dispLineOnCreation() )
    {
	PtrMan<IOObj> ioobj = IOM().get( multiid );
	load( *ioobj, (int)uiODRandomLineTreeItem::Empty );
    }

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


void uiODRandomLineTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || menu->menuID()!=displayID() )
	return;

    mAddMenuOrTBItem( istb, 0, menu, &create2dgridmnuitem_, true, false );

    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
		    visserv_->getObject(displayid_));
    if (  rtd->nrNodes() <= 0 ) return;
    const bool islocked = rtd->isGeometryLocked() || rtd->isLocked();
    mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &editnodesmnuitem_,
		      !islocked, false );
    mAddMenuOrTBItem( istb, 0, &displaymnuitem_, &insertnodemnuitem_,
		      !islocked, false );
    insertnodemnuitem_.removeItems();

    for ( int idx=0; !islocked && idx<=rtd->nrNodes(); idx++ )
    {
	BufferString nodename;
	if ( idx==rtd->nrNodes() )
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
			    rtd->canAddNode(idx), false );
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

    if ( mnuid==editnodesmnuitem_.id )
    {
	editNodes();
	menu->setIsHandled(true);
    }
    else if ( insertnodemnuitem_.itemIndex(mnuid)!=-1 )
    {
	menu->setIsHandled(true);
	rtd->addNode(insertnodemnuitem_.itemIndex(mnuid));
    }
    else
    {
	mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneID()))
	const bool hasztf = scene && scene->getZAxisTransform();

	TypeSet<BinID> bids; rtd->getAllNodePos( bids );
	RefMan<Geometry::RandomLine> rln = new Geometry::RandomLine;
	const Interval<float> rtdzrg = rtd->getDepthInterval();
	rln->setZRange( hasztf ? SI().zRange(false) : rtdzrg );
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

	    BufferString bs;
	    if ( !RandomLineSetTranslator::store(lset,ioobj,bs) )
		uiMSG().error( bs );
	    else
	    {
		applMgr()->visServer()->setObjectName( displayID(),
						       ioobj->name() );
		updateColumnText( uiODSceneMgr::cNameColumn() );
	    }
	}
	else if ( mnuid == saveas2dmnuitem_.id )
	{
	    rln->setName( mFromUiStringTodo(rtd->name()) );
	    applMgr()->seisServer()->storeRlnAs2DLine( *rln );
	}
	else if ( mnuid == create2dgridmnuitem_.id )
	{
	    rln->setName( mFromUiStringTodo(rtd->name()) );
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
    rtd->getAllNodePos( bids );
    uiDialog dlg( getUiParent(),
		  uiDialog::Setup("Random lines","Specify node positions",
				  mODHelpKey(mODRandomLineTreeItemHelpID) ) );
    uiPositionTable* table = new uiPositionTable( &dlg, true, true, true );
    table->setBinIDs( bids );

    Interval<float> zrg = rtd->getDataTraceRange();
    zrg.scale( mCast(float,SI().zDomain().userFactor() ) );
    table->setZRange( zrg );
    if ( dlg.go() )
    {
	TypeSet<BinID> newbids;
	table->getBinIDs( newbids );
	rtd->setNodePositions( newbids );

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
    ODMainWin()->viewer2DMgr().remove2DViewer( displayid_, true );
}
