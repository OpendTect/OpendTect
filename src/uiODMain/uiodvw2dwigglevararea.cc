/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
________________________________________________________________________

-*/

#include "uiodvw2dwigglevararea.h"

#include "uiattribpartserv.h"
#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uimenuhandler.h"
#include "uiodapplmgr.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"
#include "uistrings.h"
#include "uitreeview.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribprobelayer.h"
#include "ioobj.h"
#include "seisdatapack.h"
#include "seisioobjinfo.h"
#include "survinfo.h"
#include "view2dseismic.h"
#include "view2ddataman.h"


uiODVW2DWiggleVarAreaTreeItem::uiODVW2DWiggleVarAreaTreeItem()
    : uiODVw2DTreeItem( uiStrings::sWVA() )
    , dummyview_(0)
    , menu_(0)
    , selattrmnuitem_(uiStrings::sSelAttrib())
{}


uiODVW2DWiggleVarAreaTreeItem::~uiODVW2DWiggleVarAreaTreeItem()
{
    if ( viewer2D()->viewwin()->nrViewers() )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
	vwr.dataChanged.remove(
		mCB(this,uiODVW2DWiggleVarAreaTreeItem,dataChangedCB) );
    }

    if ( menu_ )
    {
	menu_->createnotifier.remove(
		mCB(this,uiODVW2DWiggleVarAreaTreeItem,createMenuCB) );
	menu_->handlenotifier.remove(
		mCB(this,uiODVW2DWiggleVarAreaTreeItem,handleMenuCB) );
	menu_->unRef();
    }

    if ( dummyview_ )
	viewer2D()->dataMgr()->removeObject( dummyview_ );
}


const char* uiODVW2DWiggleVarAreaTreeItem::iconName() const
{ return "tree-wva"; }


void uiODVW2DWiggleVarAreaTreeItem::setAttribProbeLayer(
		AttribProbeLayer* attrlayer )
{
    Monitorable::ChangeType ct = replaceMonitoredRef(attrlayer_,attrlayer,this);
    mAttachCBIfNotAttached( attrlayer_->objectChanged(),
	       uiODVW2DWiggleVarAreaTreeItem::attrLayerChangedCB );
    if ( ct )
	attrLayerChangedCB( 0 );
}


bool uiODVW2DWiggleVarAreaTreeItem::init()
{
    if ( !viewer2D()->viewwin()->nrViewers() )
	return false;

    Probe& vwr2dprobe = viewer2D()->getProbe();
    for ( int idx=0; idx<vwr2dprobe.nrLayers(); idx++ )
    {
	mDynamicCastGet(AttribProbeLayer*,attrlayer,
			vwr2dprobe.getLayerByIdx(idx))
	if ( attrlayer && attrlayer->dispType()==AttribProbeLayer::Wiggle )
	{
	    setAttribProbeLayer( attrlayer );
	    break;
	}
    }

    uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    vwr.dataChanged.notify(
	    mCB(this,uiODVW2DWiggleVarAreaTreeItem,dataChangedCB) );

    const FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;
    uitreeviewitem_->setCheckable( true );
    uitreeviewitem_->setChecked( ddp.wva_.show_ && attrlayer_ &&
				 attrlayer_->hasData() );

    checkStatusChange()->notify(
	    mCB(this,uiODVW2DWiggleVarAreaTreeItem,checkCB) );

    dummyview_ = new VW2DSeis();
    viewer2D()->dataMgr()->addObject( dummyview_ );
    displayid_ = dummyview_->id();

    return uiODVw2DTreeItem::init();
}


bool uiODVW2DWiggleVarAreaTreeItem::select()
{
    if ( !uitreeviewitem_->isSelected() )
	return false;

    if ( dummyview_ )
	viewer2D()->dataMgr()->setSelected( dummyview_ );

    return true;
}


void uiODVW2DWiggleVarAreaTreeItem::checkCB( CallBacker* )
{
    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
	viewer2D()->viewwin()->viewer(ivwr).setVisible( true, isChecked() );
}


void uiODVW2DWiggleVarAreaTreeItem::dataChangedCB( CallBacker* )
{
    if ( !viewer2D()->viewwin()->nrViewers() )
	return;

    const uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    const FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;
    uitreeviewitem_->setCheckable( ddp.vd_.show_ && vwr.hasPack(true) );
    uitreeviewitem_->setChecked( ddp.wva_.show_ && attrlayer_ &&
				 attrlayer_->hasData() );
}


void uiODVW2DWiggleVarAreaTreeItem::attrLayerChangedCB( CallBacker* )
{
    if ( !viewer2D()->viewwin()->nrViewers() || !viewer2D()->viewControl() )
	return;

    viewer2D()->setUpView( attrlayer_->getID() );
}


bool uiODVW2DWiggleVarAreaTreeItem::showSubMenu()
{
    if ( !menu_ )
    {
	menu_ = new uiMenuHandler( getUiParent(), -1 );
	menu_->ref();
	menu_->createnotifier.notify(
		mCB(this,uiODVW2DWiggleVarAreaTreeItem,createMenuCB) );
	menu_->handlenotifier.notify(
		mCB(this,uiODVW2DWiggleVarAreaTreeItem,handleMenuCB) );
    }
    return menu_->executeMenu(uiMenuHandler::fromTree());
}


void uiODVW2DWiggleVarAreaTreeItem::createMenuCB( CallBacker* cb )
{
    selattrmnuitem_.removeItems();
    createSelMenu( selattrmnuitem_ );

     mDynamicCastGet(MenuHandler*,menu,cb);
     if ( selattrmnuitem_.nrItems() )
	  mAddMenuItem( menu, &selattrmnuitem_, true, false );
}


void uiODVW2DWiggleVarAreaTreeItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( handleSelMenu(mnuid) )
	menu->setIsHandled(true);
}


void uiODVW2DWiggleVarAreaTreeItem::createSelMenu( MenuItem& mnu )
{
    const uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    ConstRefMan<FlatDataPack> dp = vwr.getPack( true, true );
    if ( !dp ) return;

    const Attrib::SelSpec& as = viewer2D()->selSpec( true );
    MenuItem* subitem = 0;
    uiAttribPartServer* attrserv = applMgr()->attrServer();
    attrserv->resetMenuItems();

    mDynamicCastGet(const RegularSeisFlatDataPack*,regfdp,dp.ptr());
    const bool is2d = regfdp && regfdp->is2D();
    Pos::GeomID geomid = viewer2D()->geomID();
    subitem = applMgr()->attrServer()->storedAttribMenuItem(as,is2d,false);
    if ( is2d )
	attrserv->filter2DMenuItems( *subitem, as, geomid, true, 0 );
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );

    subitem = applMgr()->attrServer()->calcAttribMenuItem( as, is2d, true );
    if ( is2d )
	attrserv->filter2DMenuItems( *subitem, as, geomid, false, 2 );
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );

    subitem = applMgr()->attrServer()->storedAttribMenuItem(as,is2d,true);
    if ( is2d )
	attrserv->filter2DMenuItems( *subitem, as, geomid, true, 1 );
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );
}


bool uiODVW2DWiggleVarAreaTreeItem::handleSelMenu( int mnuid )
{
    uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    ConstRefMan<FlatDataPack> dp = vwr.getPack( true, true );
    if ( !dp ) return false;

    uiAttribPartServer* attrserv = applMgr()->attrServer();
    bool dousemulticomp, stored, steering;
    dousemulticomp = stored = steering = false;

    BufferString attrbnm;
    mDynamicCastGet(const RegularSeisFlatDataPack*,regfdp,dp.ptr());
    if ( regfdp && regfdp->is2D() )
	attrserv->info2DAttribSubMenu( mnuid, attrbnm, steering, stored );

    Attrib::SelSpec selas = viewer2D()->selSpec( true );
    if ( !stored && !attrserv->handleAttribSubMenu(mnuid,selas,dousemulticomp) )
	return false;

    if ( !attrlayer_ )
    {
	AttribProbeLayer* attrprobelayer =
	    new AttribProbeLayer( AttribProbeLayer::Wiggle );
	attrprobelayer->setSelSpec( selas );

	viewer2D()->getProbe().addLayer( attrprobelayer );
	setAttribProbeLayer( attrprobelayer );
    }
    else
	attrlayer_->setSelSpec( selas );

    return true;
}


uiTreeItem* uiODVW2DWiggleVarAreaTreeItemFactory::createForVis(
					    const uiODViewer2D&, int id ) const
{
    return 0;
}
