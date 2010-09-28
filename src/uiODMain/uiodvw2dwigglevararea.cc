/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id: uiodvw2dwigglevararea.cc,v 1.4 2010-09-28 06:02:31 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uiodvw2dwigglevararea.h"

#include "uiattribpartserv.h"
#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uilistview.h"
#include "uimenuhandler.h"
#include "uiodapplmgr.h"
#include "uiodviewer2d.h"

#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "visvw2dseismic.h"
#include "visvw2ddataman.h"


uiODVW2DWiggleVarAreaTreeItem::uiODVW2DWiggleVarAreaTreeItem()
    : uiODVw2DTreeItem( "Wiggle Var Area" )
    , dpid_(DataPack::cNoID())
    , dummyview_(0)
    , menu_(0)
    , selattrmnuitem_("Select &Attribute")
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
    
    viewer2D()->dataMgr()->removeObject( dummyview_ );
}


bool uiODVW2DWiggleVarAreaTreeItem::init()
{
    if ( !viewer2D()->viewwin()->nrViewers() )
	return false;

    uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);

    const DataPack* fdpw = vwr.pack( true );
    if ( fdpw )
	dpid_ = fdpw->id();
    const DataPack* fdpv = vwr.pack( false );

    vwr.dataChanged.notify(
	    mCB(this,uiODVW2DWiggleVarAreaTreeItem,dataChangedCB) );

    uilistviewitem_->setChecked( fdpw );
    uilistviewitem_->setCheckable( fdpv && dpid_!=DataPack::cNoID() );

    checkStatusChange()->notify(
	    mCB(this,uiODVW2DWiggleVarAreaTreeItem,checkCB) );

    dummyview_ = new VW2DSeis();
    viewer2D()->dataMgr()->addObject( dummyview_ );

    return true;
}


bool uiODVW2DWiggleVarAreaTreeItem::select()
{
    if ( !uilistviewitem_->isSelected() )
	return false;

    viewer2D()->dataMgr()->setSelected( dummyview_ );

    return true;
}


void uiODVW2DWiggleVarAreaTreeItem::checkCB( CallBacker* )
{
    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	DataPack::ID id = DataPack::cNoID();

	if ( isChecked() )
	    id = dpid_;

	viewer2D()->viewwin()->viewer(ivwr).usePack( true, id, false );
    }
}


void uiODVW2DWiggleVarAreaTreeItem::dataChangedCB( CallBacker* )
{
    if ( !viewer2D()->viewwin()->nrViewers() )
	return;

    uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);

    const DataPack* fdpw = vwr.pack( true );

    const DataPack* fdpv = vwr.pack( false );
    
    uilistviewitem_->setChecked( fdpw );
    uilistviewitem_->setCheckable( fdpv &&
	    			   (dpid_!=DataPack::cNoID() || fdpw) );

    if ( fdpw )
	dpid_ = fdpw->id();
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
    uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    const DataPack* dp = vwr.pack( false );
    if ( !dp )
	dp = vwr.pack( true );
    if ( !dp ) return;
    
    mDynamicCastGet(const Attrib::Flat3DDataPack*,dp3d,dp);
    mDynamicCastGet(const Attrib::Flat2DDataPack*,dp2d,dp);
    mDynamicCastGet(const Attrib::Flat2DDHDataPack*,dp2ddh,dp);

    const Attrib::SelSpec& as = viewer2D()->selSpec( true );
    MenuItem* subitem;
    applMgr()->attrServer()->resetMenuItems();
    subitem = applMgr()->attrServer()->storedAttribMenuItem(as, dp2ddh, false);
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );
    subitem = applMgr()->attrServer()->calcAttribMenuItem( as, dp2ddh, true );
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );
    subitem = applMgr()->attrServer()->storedAttribMenuItem( as, dp2ddh, true );
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );
}


bool uiODVW2DWiggleVarAreaTreeItem::handleSelMenu( int mnuid )
{
    const Attrib::SelSpec& as = viewer2D()->selSpec( true );
    Attrib::SelSpec selas( as );

    uiAttribPartServer* attrserv = applMgr()->attrServer();
    bool dousemulticomp = false;
    if ( attrserv->handleAttribSubMenu(mnuid,selas,dousemulticomp) )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
	const DataPack* dp = vwr.pack( true );
	if ( !dp )
	    dp = vwr.pack( false );
	if ( !dp ) return false;

	mDynamicCastGet(const Attrib::Flat3DDataPack*,dp3d,dp);
	mDynamicCastGet(const Attrib::Flat2DDataPack*,dp2d,dp);
	mDynamicCastGet(const Attrib::Flat2DDHDataPack*,dp2ddh,dp);

	if ( dp3d )
	{
	    attrserv->setTargetSelSpec( selas );
	    const DataPack::ID newid = attrserv->createOutput(
		    	dp3d->cube().cubeSampling(), DataPack::cNoID() );

	    if ( newid != DataPack::cNoID() )
	    {
		viewer2D()->setSelSpec( &selas, true );
		viewer2D()->setUpView( newid, true );
	    }
	}

	return true;
    }

    return false;
}
