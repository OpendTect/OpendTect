/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id: uiodvw2dvariabledensity.cc,v 1.7 2010-09-29 07:04:42 cvsumesh Exp $
________________________________________________________________________

-*/

#include "uiodvw2dvariabledensity.h"

#include "uiattribpartserv.h"
#include "uicolortable.h"
#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"
#include "uiflatviewcoltabed.h"
#include "uilistview.h"
#include "uimenuhandler.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"

#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "coltabmapper.h"
#include "coltabsequence.h"
#include "filepath.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "pixmap.h"
#include "visvw2dseismic.h"
#include "visvw2ddataman.h"


uiODVW2DVariableDensityTreeItem::uiODVW2DVariableDensityTreeItem()
    : uiODVw2DTreeItem( "Variable Density" )
    , dpid_(DataPack::cNoID())
    , dummyview_(0)
    , menu_(0)
    , selattrmnuitem_("Select &Attribute")
{}


uiODVW2DVariableDensityTreeItem::~uiODVW2DVariableDensityTreeItem()
{
    if ( viewer2D()->viewwin()->nrViewers() )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
	vwr.dataChanged.remove(
		mCB(this,uiODVW2DVariableDensityTreeItem,dataChangedCB) );
    }

    if ( menu_ )
    {
	menu_->createnotifier.remove(
		mCB(this,uiODVW2DVariableDensityTreeItem,createMenuCB) );
	menu_->handlenotifier.remove(
		mCB(this,uiODVW2DVariableDensityTreeItem,handleMenuCB) );
	menu_->unRef();
    }

    viewer2D()->dataMgr()->removeObject( dummyview_ );
}


bool uiODVW2DVariableDensityTreeItem::init()
{
    if ( !viewer2D()->viewwin()->nrViewers() )
	return false;

    uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);

    const DataPack* fdpw = vwr.pack( true );

    const DataPack* fdpv = vwr.pack( false );
    if ( fdpv )
	dpid_ = fdpv->id();

    vwr.dataChanged.notify(
	    mCB(this,uiODVW2DVariableDensityTreeItem,dataChangedCB) );

    uilistviewitem_->setChecked( fdpv );
    uilistviewitem_->setCheckable( fdpw && dpid_!=DataPack::cNoID() );

    checkStatusChange()->notify(
	    mCB(this,uiODVW2DVariableDensityTreeItem,checkCB) );

    dummyview_ = new VW2DSeis();
    viewer2D()->dataMgr()->addObject( dummyview_ );

    if ( fdpv )
    {
	if ( !vwr.control() )
	    displayMiniCtab(0);

	ColTab::Sequence seq( vwr.appearance().ddpars_.vd_.ctab_ );
	displayMiniCtab( &seq );
    }

    return true;
}


bool uiODVW2DVariableDensityTreeItem::select()
{
    if ( !uilistviewitem_->isSelected() )
	return false;

    viewer2D()->dataMgr()->setSelected( dummyview_ );

    return true;
}


void uiODVW2DVariableDensityTreeItem::checkCB( CallBacker* )
{
    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	DataPack::ID id = DataPack::cNoID();

	if ( isChecked() )
	    id = dpid_;

	viewer2D()->viewwin()->viewer(ivwr).usePack( false, id, false );
    }
}


void uiODVW2DVariableDensityTreeItem::dataChangedCB( CallBacker* )
{
    if ( !viewer2D()->viewwin()->nrViewers() )
	return;

    displayMiniCtab(0);

    uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    const DataPack* fdpw = vwr.pack( true );
    const DataPack* fdpv = vwr.pack( false );
    
    uilistviewitem_->setChecked( fdpv );
    uilistviewitem_->setCheckable( fdpw &&
	    			   (dpid_!=DataPack::cNoID() || fdpw) );

    if ( fdpv )
	dpid_ = fdpv->id();

    if ( !fdpv )
	displayMiniCtab(0);
    else
    {
	if ( !vwr.control() )
	    displayMiniCtab(0);

	ColTab::Sequence seq( vwr.appearance().ddpars_.vd_.ctab_ );
	displayMiniCtab( &seq );
    }
}


void uiODVW2DVariableDensityTreeItem::displayMiniCtab(
						const ColTab::Sequence* seq )
{
    if ( !seq )
    {
	uiTreeItem::updateColumnText( uiODViewer2DMgr::cColorColumn() );
	return;
    }

    PtrMan<ioPixmap> pixmap = new ioPixmap( *seq, cPixmapWidth(),
	    				    cPixmapHeight(), true );
    uilistviewitem_->setPixmap( uiODViewer2DMgr::cColorColumn(), *pixmap );
}


bool uiODVW2DVariableDensityTreeItem::showSubMenu()
{
    if ( !menu_ )
    {
	menu_ = new uiMenuHandler( getUiParent(), -1 );
	menu_->ref();
	menu_->createnotifier.notify(
		mCB(this,uiODVW2DVariableDensityTreeItem,createMenuCB) );
	menu_->handlenotifier.notify(
		mCB(this,uiODVW2DVariableDensityTreeItem,handleMenuCB) );
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
    uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    const DataPack* dp = vwr.pack( true );
    if ( !dp )
	dp = vwr.pack( false );

    if ( !dp ) return;

    mDynamicCastGet(const Attrib::Flat3DDataPack*,dp3d,dp);
    mDynamicCastGet(const Attrib::Flat2DDataPack*,dp2d,dp);
    mDynamicCastGet(const Attrib::Flat2DDHDataPack*,dp2ddh,dp)

    const Attrib::SelSpec& as = viewer2D()->selSpec( false );
    MenuItem* subitem;
    applMgr()->attrServer()->resetMenuItems();
    subitem = applMgr()->attrServer()->storedAttribMenuItem(as, dp2ddh, false);
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );
    subitem = applMgr()->attrServer()->calcAttribMenuItem( as, dp2ddh, true );
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );
    subitem = applMgr()->attrServer()->storedAttribMenuItem( as, dp2ddh, true );
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );
}


bool uiODVW2DVariableDensityTreeItem::handleSelMenu( int mnuid )
{
    const Attrib::SelSpec& as = viewer2D()->selSpec( false );
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

	    if ( newid == DataPack::cNoID() ) return true;
	    
	    viewer2D()->setSelSpec( &selas, false );

	    ColTab::MapperSetup mapper;
	    ColTab::Sequence seq( 0 );
	    PtrMan<IOObj> ioobj = attrserv->getIOObj( selas );
	    if ( ioobj )
	    {
		FilePath fp( ioobj->fullUserExpr(true) );
		fp.setExtension( "par" );
		IOPar iop;
		if ( iop.read( fp.fullPath(), sKey::Pars) && !iop.isEmpty() )
		{
		    const char* ctname = iop.find( sKey::Name );
		    vwr.appearance().ddpars_.vd_.ctab_ = ctname;
		    seq = ColTab::Sequence( ctname );
		    displayMiniCtab( &seq );
		}
	    }
	    viewer2D()->setUpView( newid, false );
	}

	return true;
    }

    return false;
}
