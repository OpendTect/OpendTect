/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id$
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
#include "uitaskrunner.h"
#include "uitreeview.h"
#include "filepath.h"
#include "ioobj.h"

#include "attribdatapackzaxistransformer.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetsholder.h"
#include "flatposdata.h"
#include "seisioobjinfo.h"
#include "visvw2dseismic.h"
#include "visvw2ddataman.h"
#include "zaxistransform.h"


uiODVW2DWiggleVarAreaTreeItem::uiODVW2DWiggleVarAreaTreeItem()
    : uiODVw2DTreeItem( "Wiggle" )
    , dummyview_(0)
    , menu_(0)
    , selattrmnuitem_(tr("Select Attribute"))
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
    vwr.dataChanged.notify(
	    mCB(this,uiODVW2DWiggleVarAreaTreeItem,dataChangedCB) );

    const FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;
    uitreeviewitem_->setCheckable( vwr.isVisible(false) &&
	    			   viewer2D()->selSpec(true).id().isValid() );
    uitreeviewitem_->setChecked( ddp.wva_.show_ );

    checkStatusChange()->notify(
	    mCB(this,uiODVW2DWiggleVarAreaTreeItem,checkCB) );

    dummyview_ = new VW2DSeis();
    viewer2D()->dataMgr()->addObject( dummyview_ );

    return true;
}


bool uiODVW2DWiggleVarAreaTreeItem::select()
{
    if ( !uitreeviewitem_->isSelected() )
	return false;

    viewer2D()->dataMgr()->setSelected( dummyview_ );

    return true;
}


void uiODVW2DWiggleVarAreaTreeItem::checkCB( CallBacker* )
{
    const uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    if ( !vwr.hasPack(true) )
    {
	if ( !isChecked() ) return;
	const DataPack::ID dpid = viewer2D()->getDataPackID( true );
	if ( dpid != DataPack::cNoID() ) viewer2D()->setUpView( dpid, true );
	return;
    }

    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
	viewer2D()->viewwin()->viewer(ivwr).setVisible( true, isChecked() );
}


void uiODVW2DWiggleVarAreaTreeItem::dataChangedCB( CallBacker* )
{
    if ( !viewer2D()->viewwin()->nrViewers() )
	return;

    const uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    const FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;
    uitreeviewitem_->setCheckable( vwr.isVisible(false) &&
				   viewer2D()->selSpec(true).id().isValid() );
    uitreeviewitem_->setChecked( ddp.wva_.show_ );
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
    ConstDataPackRef<FlatDataPack> dp = vwr.obtainPack( true, true );
    if ( !dp ) return;

    const Attrib::SelSpec& as = viewer2D()->selSpec( true );
    MenuItem* subitem = 0;
    applMgr()->attrServer()->resetMenuItems();

    mDynamicCastGet(const Attrib::Flat2DDHDataPack*,dp2ddh,dp.ptr());
    if ( dp2ddh )
    {
//	BufferString lnm;
//	dp2ddh->getLineName( lnm );
// TODO: Use lnm to get attributes for this line only
	subitem = applMgr()->attrServer()->storedAttribMenuItem(as,true,false);
    }
    else
	subitem = applMgr()->attrServer()->storedAttribMenuItem(as,false,false);
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );

    subitem = applMgr()->attrServer()->calcAttribMenuItem( as, dp2ddh, true );
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );

    if ( dp2ddh )
    {
//	BufferString lnm;
//	dp2ddh->getLineName( lnm );
// TODO: Use lnm to get attributes for this line only
	subitem = applMgr()->attrServer()->storedAttribMenuItem(as,true,true);
    }
    else
	subitem = applMgr()->attrServer()->storedAttribMenuItem(as,false,true);
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );
}


bool uiODVW2DWiggleVarAreaTreeItem::handleSelMenu( int mnuid )
{
    const Attrib::SelSpec& as = viewer2D()->selSpec( true );
    Attrib::SelSpec selas( as );

    uiAttribPartServer* attrserv = applMgr()->attrServer();

    const uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    ConstDataPackRef<FlatDataPack> dp = vwr.obtainPack( true, true );
    if ( !dp ) return false;

    mDynamicCastGet(const Attrib::FlatRdmTrcsDataPack*,dprdm,dp.ptr());
    mDynamicCastGet(const Attrib::Flat2DDHDataPack*,dp2ddh,dp.ptr());

    DataPack::ID newid = DataPack::cNoID();
    RefMan<ZAxisTransform> zat = viewer2D()->getZAxisTransform();
    bool dousemulticomp = false;
    if ( dp2ddh )
    {
	BufferString attrbnm; bool stored = false;
	bool steering = false;
	attrserv->info2DAttribSubMenu( mnuid, attrbnm, steering, stored );

	uiTaskRunner uitr( &viewer2D()->viewwin()->viewer() );
	CubeSampling cs = dp2ddh->getCubeSampling();
	if ( zat ) cs.zrg = zat->getZInterval( true );

	if ( !stored )
	{
	    if ( !attrserv->handleAttribSubMenu(mnuid,selas,dousemulticomp) )
		return false;

	    attrserv->setTargetSelSpec( selas );
	    newid = attrserv->create2DOutput( cs, dp2ddh->getGeomID(), uitr );
	}
	else
	{
	    const SeisIOObjInfo objinfo( attrbnm );
	    if ( !objinfo.ioObj() )
		return false;

	    Attrib::DescID attribid = attrserv->getStoredID(
			    objinfo.ioObj()->key(), true, steering ? 1 : 0 );
	    selas.set( attrbnm, attribid, false, 0 );
	    selas.set2DFlag();

	    const Attrib::DescSet* ds = Attrib::DSHolder().getDescSet( true,
		    						       true );
	    if ( !ds ) return false;
	    selas.setRefFromID( *ds );
	    selas.setUserRef( attrbnm );

	    const Attrib::Desc* targetdesc = ds->getDesc( attribid );
	    if ( !targetdesc ) return false;

	    BufferString defstring;
	    targetdesc->getDefStr( defstring );
	    selas.setDefString( defstring );
	    attrserv->setTargetSelSpec( selas );
	    newid = attrserv->create2DOutput( cs, dp2ddh->getGeomID(), uitr );
	}
    }
    else if ( attrserv->handleAttribSubMenu(mnuid,selas,dousemulticomp) )
    {
	if ( dprdm )
	{
	    attrserv->setTargetSelSpec( selas );
	    const Interval<float> zrg = zat ? zat->getZInterval(true) :
		Interval<float>((float)dprdm->posData().range(false).start,
				(float)dprdm->posData().range(false).stop);

	    TypeSet<BinID> bids;
	    if ( dprdm->pathBIDs() )
		bids = *dprdm->pathBIDs();
	    newid = attrserv->createRdmTrcsOutput( zrg, &bids, &bids );
	}
	else
	{
	    newid = viewer2D()->createDataPack( selas );
	}
    }

    if ( zat && (dp2ddh || dprdm) )
    {
	ConstDataPackRef<FlatDataPack> newdp =
		DPM(DataPackMgr::FlatID()).obtain( newid );
	Attrib::FlatDataPackZAxisTransformer transformer( *zat );
	transformer.setInput( newdp.ptr() );
	transformer.setOutput( newid );
	transformer.execute();
    }

    if ( newid == DataPack::cNoID() ) return true;

    viewer2D()->setSelSpec( &selas, true );

    PtrMan<IOObj> ioobj = attrserv->getIOObj( selas );
    if ( ioobj )
    {
	FilePath fp( ioobj->fullUserExpr(true) );
	fp.setExtension( "par" );
	IOPar iop;
	if ( iop.read(fp.fullPath(),sKey::Pars()) && !iop.isEmpty() )
	{
	    ColTab::MapperSetup mapper;
	    mapper.usePar( iop );

	    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
	    {
		FlatView::DataDispPars& ddp =
		    viewer2D()->viewwin()->viewer(ivwr).appearance().ddpars_;
    		ddp.wva_.mappersetup_ = mapper;
	    }
	}
    }

    viewer2D()->setUpView( newid, true );

    return false;
}


uiTreeItem* uiODVW2DWiggleVarAreaTreeItemFactory::createForVis(
					    const uiODViewer2D&, int id ) const
{
    return 0;
}

