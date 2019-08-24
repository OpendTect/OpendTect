/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
________________________________________________________________________

-*/

#include "uiodvw2dvariabledensity.h"

#include "uiattribpartserv.h"
#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uiflatviewcoltabed.h"
#include "uiflatviewstdcontrol.h"
#include "uimenuhandler.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"
#include "uicoltabsel.h"
#include "uistrings.h"
#include "uitreeview.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribprobelayer.h"
#include "coltabseqmgr.h"
#include "ioobj.h"
#include "seisdatapack.h"
#include "seisioobjinfo.h"
#include "survinfo.h"
#include "view2dseismic.h"
#include "view2ddataman.h"


uiODVW2DVariableDensityTreeItem::uiODVW2DVariableDensityTreeItem()
    : uiODVw2DTreeItem( uiStrings::sVD() )
    , dummyview_(0)
    , menu_(0)
    , coltabinitialized_(false)
    , selattrmnuitem_(uiStrings::sSelAttrib())
{
}


uiODVW2DVariableDensityTreeItem::~uiODVW2DVariableDensityTreeItem()
{
    detachAllNotifiers();
    if ( menu_ )
	menu_->unRef();

    if ( dummyview_ )
	viewer2D()->dataMgr()->removeObject( dummyview_ );
}


const char* uiODVW2DVariableDensityTreeItem::iconName() const
{ return "tree-vd"; }


bool uiODVW2DVariableDensityTreeItem::init()
{
    if ( !viewer2D()->viewwin()->nrViewers() )
	return false;

    Probe& vwr2dprobe = viewer2D()->getProbe();
    for ( int idx=0; idx<vwr2dprobe.nrLayers(); idx++ )
    {
	mDynamicCastGet(AttribProbeLayer*,attrlayer,
			vwr2dprobe.getLayerByIdx(idx))
	if ( attrlayer && attrlayer->dispType()==AttribProbeLayer::VD )
	{
	    setAttribProbeLayer( attrlayer );
	    break;
	}
    }

    uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    mAttachCB( vwr.dataChanged,uiODVW2DVariableDensityTreeItem::dataChangedCB );
    mAttachCB( vwr.dispParsChanged,
	       uiODVW2DVariableDensityTreeItem::dataChangedCB );

    const FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;
    uitreeviewitem_->setCheckable( true );
    uitreeviewitem_->setChecked( ddp.vd_.show_ && attrlayer_ &&
				 attrlayer_->hasData() );
    mAttachCB( checkStatusChange(), uiODVW2DVariableDensityTreeItem::checkCB );

    dummyview_ = new VW2DSeis();
    viewer2D()->dataMgr()->addObject( dummyview_ );
    displayid_ = dummyview_->id();
    mAttachCB( dummyview_->deSelection(),
	       uiODVW2DVariableDensityTreeItem::deSelectCB );

    if ( vwr.hasPack(false) )
    {
	if ( !vwr.control() )
	    displayMiniCtab(0);
	else
	    displayMiniCtab( ColTab::SeqMGR().getAny(
				vwr.appearance().ddpars_.vd_.colseqname_) );
    }

    return uiODVw2DTreeItem::init();
}


void uiODVW2DVariableDensityTreeItem::setAttribProbeLayer(
		AttribProbeLayer* attrlayer )
{
    Monitorable::ChangeType ct = replaceMonitoredRef(attrlayer_,attrlayer,this);
    mAttachCBIfNotAttached( attrlayer_->objectChanged(),
	       uiODVW2DVariableDensityTreeItem::attrLayerChangedCB );
    mAttachCBIfNotAttached( attrlayer_->mapper().objectChanged(),
	       uiODVW2DVariableDensityTreeItem::attrLayerChangedCB );
    if ( ct )
	attrLayerChangedCB( 0 );
}


void uiODVW2DVariableDensityTreeItem::initColTab()
{
    if ( coltabinitialized_ ) return;

    mAttachCB( viewer2D()->viewControl()->colTabEd()->colTabChgd,
	       uiODVW2DVariableDensityTreeItem::colTabChgCB );
    mAttachCB( viewer2D()->viewControl()->colTabEd()->selTool().refreshReq,
	       uiODVW2DVariableDensityTreeItem::colTabChgCB );

    if ( uitreeviewitem_->treeView() &&
	 uitreeviewitem_->treeView()->nrSelected() > 0 )
	return;

    uitreeviewitem_->setSelected( true );
    select(); coltabinitialized_ = true;
}


bool uiODVW2DVariableDensityTreeItem::select()
{
    if ( !uitreeviewitem_->isSelected() )
	return false;

    if ( dummyview_ )
	viewer2D()->dataMgr()->setSelected( dummyview_ );
    uiFlatViewColTabEd* coltabed = viewer2D()->viewControl()->colTabEd();
    const uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    coltabed->setDisplayPars( vwr.appearance().ddpars_.vd_ );
    return true;
}


void uiODVW2DVariableDensityTreeItem::deSelectCB( CallBacker* )
{
    viewer2D()->viewControl()->colTabEd()->setSensitive( false );
}


void uiODVW2DVariableDensityTreeItem::checkCB( CallBacker* )
{
    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
	viewer2D()->viewwin()->viewer(ivwr).setVisible( false, isChecked() );
}


void uiODVW2DVariableDensityTreeItem::colTabChgCB( CallBacker* cb )
{
    if ( !dummyview_ || viewer2D()->dataMgr()->selectedID() != dummyview_->id())
	return;

    mDynamicCastGet(uiFlatViewColTabEd*,coltabed,cb);
    if ( !coltabed )
	return;

    const FlatView::DataDispPars::VD& vdpars = coltabed->displayPars();
    NotifyStopper ns( viewer2D()->viewControl()->colTabEd()->colTabChgd, this );
    attrlayer_->setSequence( *ColTab::SeqMGR().getAny(vdpars.colseqname_) );
    attrlayer_->mapper() = *vdpars.mapper_;
}


void uiODVW2DVariableDensityTreeItem::dataChangedCB( CallBacker* )
{
    if ( !viewer2D()->viewwin()->nrViewers() || !viewer2D()->viewControl() )
	return;

    const uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    const FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;
    uitreeviewitem_->setCheckable( ddp.wva_.show_ && vwr.hasPack(false) );
    uitreeviewitem_->setChecked( ddp.vd_.show_ && attrlayer_ &&
				 attrlayer_->hasData() );

    if ( !coltabinitialized_ ) initColTab();

    if ( dummyview_ && viewer2D()->dataMgr()->selectedID() == dummyview_->id() )
	viewer2D()->viewControl()->colTabEd()->setDisplayPars( ddp.vd_ );

    displayMiniCtab( ColTab::SeqMGR().getAny(
				vwr.appearance().ddpars_.vd_.colseqname_) );
}


void uiODVW2DVariableDensityTreeItem::attrLayerChangedCB( CallBacker* cb )
{
    if ( !viewer2D()->viewwin()->nrViewers() || !viewer2D()->viewControl() )
	return;

    Monitorable::ChangeData chgdata( Monitorable::ChangeData::AllChanged() );
    if ( cb )
    {
	mGetMonitoredChgData( cb, cd );
	chgdata = cd;
    }

    uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    if ( chgdata.changeType() == AttribProbeLayer::cDataChange() ||
	 chgdata.changeType() == AttribProbeLayer::cEntireObjectChange() )
	vwr.handleChange( FlatView::Viewer::BitmapData );
    else if ( chgdata.changeType() == AttribProbeLayer::cColSeqChange() ||
	      chgdata.changeType() == AttribProbeLayer::cMapperChange() )
    {
	vwr.appearance().ddpars_.vd_.colseqname_ =attrlayer_->sequence().name();
	vwr.handleChange( FlatView::Viewer::DisplayPars );
    }
}


void uiODVW2DVariableDensityTreeItem::displayMiniCtab(
						const ColTab::Sequence* seq )
{
    setPixmap( uiODViewer2DMgr::cColorColumn(), seq );
}


bool uiODVW2DVariableDensityTreeItem::showSubMenu()
{
    if ( !menu_ )
    {
	menu_ = new uiMenuHandler( getUiParent(), -1 );
	menu_->ref();
	mAttachCB( menu_->createnotifier,
		   uiODVW2DVariableDensityTreeItem::createMenuCB );
	mAttachCB( menu_->handlenotifier,
		   uiODVW2DVariableDensityTreeItem::handleMenuCB );
    }
    return menu_->executeMenu(uiMenuHandler::fromTree());
}


void uiODVW2DVariableDensityTreeItem::createMenuCB( CallBacker* cb )
{
    selattrmnuitem_.removeItems();
    createSelMenu( selattrmnuitem_ );

    mDynamicCastGet(MenuHandler*,menu,cb);
    if ( selattrmnuitem_.nrItems() )
	mAddMenuItem( menu, &selattrmnuitem_, true, false );
}


void uiODVW2DVariableDensityTreeItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( handleSelMenu(mnuid) )
	menu->setIsHandled(true);
}


void uiODVW2DVariableDensityTreeItem::createSelMenu( MenuItem& mnu )
{
    const uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    ConstRefMan<FlatDataPack> dp = vwr.getPack( false, true );
    if ( !dp ) return;

    uiAttribPartServer* attrserv = applMgr()->attrServer();
    const Attrib::SelSpec& as = viewer2D()->selSpec( false );
    MenuItem* subitem = 0;
    attrserv->resetMenuItems();

    mDynamicCastGet(const RegularSeisFlatDataPack*,regfdp,dp.ptr());
    const bool is2d = regfdp && regfdp->is2D();
    Pos::GeomID geomid = viewer2D()->geomID();
    subitem = attrserv->storedAttribMenuItem(as,is2d,false);
    if ( is2d )
	attrserv->filter2DMenuItems( *subitem, as, geomid, true, 0 );
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );

    subitem = attrserv->calcAttribMenuItem( as, is2d, true );
    if ( is2d )
	attrserv->filter2DMenuItems( *subitem, as, geomid, false, 2 );
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );

    subitem = attrserv->storedAttribMenuItem(as,is2d,true );
    if ( is2d )
	attrserv->filter2DMenuItems( *subitem, as, geomid, true, 1 );
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );
}


bool uiODVW2DVariableDensityTreeItem::handleSelMenu( int mnuid )
{
    uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    ConstRefMan<FlatDataPack> dp = vwr.getPack( false, true );
    if ( !dp ) return false;

    uiAttribPartServer* attrserv = applMgr()->attrServer();
    bool dousemulticomp, stored, steering;
    dousemulticomp = stored = steering = false;

    BufferString attrbnm;
    mDynamicCastGet(const RegularSeisFlatDataPack*,regfdp,dp.ptr());
    if ( regfdp && regfdp->is2D() )
	attrserv->info2DAttribSubMenu( mnuid, attrbnm, steering, stored );

    Attrib::SelSpec selas = viewer2D()->selSpec( false );
    if ( !stored && !attrserv->handleAttribSubMenu(mnuid,selas,dousemulticomp) )
	return false;

    if ( !attrlayer_ )
    {
	AttribProbeLayer* attrprobelayer = new AttribProbeLayer();
	attrprobelayer->setSelSpec( selas );

	viewer2D()->getProbe().addLayer( attrprobelayer );
	setAttribProbeLayer( attrprobelayer );
    }
    else
	attrlayer_->setSelSpec( selas );

    return true;
}


uiTreeItem* uiODVW2DVariableDensityTreeItemFactory::createForVis(
					const uiODViewer2D&,int id )const
{
    return 0;
}
