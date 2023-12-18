/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
#include "uistrings.h"
#include "uiseispartserv.h"
#include "uitreeview.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetsholder.h"
#include "coltabsequence.h"
#include "ioobj.h"
#include "ioman.h"
#include "seisdatapack.h"
#include "seisioobjinfo.h"
#include "survinfo.h"
#include "view2dseismic.h"
#include "view2ddataman.h"


uiODView2DVariableDensityTreeItem::uiODView2DVariableDensityTreeItem()
    : uiODView2DTreeItem( tr("VD") )
    , dummyview_(0)
    , menu_(0)
    , coltabinitialized_(false)
    , selattrmnuitem_(uiStrings::sSelAttrib())
{}


uiODView2DVariableDensityTreeItem::~uiODView2DVariableDensityTreeItem()
{
    detachAllNotifiers();
    if ( menu_ )
	menu_->unRef();

    if ( dummyview_ )
	viewer2D()->dataMgr()->removeObject( dummyview_ );
}


const char* uiODView2DVariableDensityTreeItem::iconName() const
{ return "tree-vd"; }


bool uiODView2DVariableDensityTreeItem::init()
{
    if ( !viewer2D()->viewwin()->nrViewers() )
	return false;

    uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    mAttachCB( vwr.dataChanged,
	       uiODView2DVariableDensityTreeItem::dataChangedCB );
    mAttachCB( vwr.dispParsChanged,
	       uiODView2DVariableDensityTreeItem::dataChangedCB );

    const FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;
    uitreeviewitem_->setCheckable( true );
    uitreeviewitem_->setChecked( ddp.vd_.show_ );
    mAttachCB( checkStatusChange(), uiODView2DVariableDensityTreeItem::checkCB);

    dummyview_ = new View2D::Seismic();
    viewer2D()->dataMgr()->addObject( dummyview_ );
    displayid_ = dummyview_->id();
    mAttachCB( dummyview_->deSelection(),
	       uiODView2DVariableDensityTreeItem::deSelectCB );

    if ( vwr.hasPack(false) )
    {
	if ( !vwr.control() )
	    displayMiniCtab(0);

	ColTab::Sequence seq( vwr.appearance().ddpars_.vd_.ctab_ );
	displayMiniCtab( &seq );
    }

    return uiODView2DTreeItem::init();
}


void uiODView2DVariableDensityTreeItem::initColTab()
{
    if ( coltabinitialized_ )
	return;

    mAttachCB( viewer2D()->viewControl()->colTabEd()->colTabChgd,
	       uiODView2DVariableDensityTreeItem::colTabChgCB );

    if ( uitreeviewitem_->treeView() &&
	 uitreeviewitem_->treeView()->nrSelected() > 0 )
	return;

    uitreeviewitem_->setSelected( true );
    select(); coltabinitialized_ = true;
}


bool uiODView2DVariableDensityTreeItem::select()
{
    if ( !uitreeviewitem_->isSelected() )
	return false;

    if ( dummyview_ )
	viewer2D()->dataMgr()->setSelected( dummyview_ );
    uiFlatViewColTabEd* coltabed = viewer2D()->viewControl()->colTabEd();
    const uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    coltabed->setColTab( vwr.appearance().ddpars_.vd_ );
    return true;
}


void uiODView2DVariableDensityTreeItem::deSelectCB( CallBacker* )
{
    viewer2D()->viewControl()->colTabEd()->setSensitive( false );
}


void uiODView2DVariableDensityTreeItem::checkCB( CallBacker* )
{
    const FlatView::Viewer::VwrDest dest = FlatView::Viewer::VD;
    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
	viewer2D()->viewwin()->viewer(ivwr).setVisible( dest, isChecked() );
}


void uiODView2DVariableDensityTreeItem::colTabChgCB( CallBacker* cb )
{
    if ( !dummyview_ || viewer2D()->dataMgr()->selectedID() != dummyview_->id())
	return;

    mDynamicCastGet(uiFlatViewColTabEd*,coltabed,cb);
    if ( !coltabed )
	return;

    const FlatView::DataDispPars::VD& vdpars = coltabed->getDisplayPars();
    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(ivwr);
	vwr.appearance().ddpars_.vd_ = vdpars;
	vwr.handleChange( FlatView::Viewer::DisplayPars );
    }
}


void uiODView2DVariableDensityTreeItem::dataChangedCB( CallBacker* )
{
    if ( !viewer2D()->viewwin()->nrViewers() || !viewer2D()->viewControl() )
	return;

    const uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    const FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;
    uitreeviewitem_->setCheckable( ddp.wva_.show_ && vwr.hasPack(false) );
    uitreeviewitem_->setChecked( ddp.vd_.show_ );

    if ( !coltabinitialized_ ) initColTab();

    if ( dummyview_ && viewer2D()->dataMgr()->selectedID() == dummyview_->id() )
	viewer2D()->viewControl()->colTabEd()->setColTab( ddp.vd_ );

    ColTab::Sequence seq( ddp.vd_.ctab_ );
    displayMiniCtab( &seq );
}


void uiODView2DVariableDensityTreeItem::dataTransformCB( CallBacker* )
{
    Attrib::SelSpec& selspec = viewer2D()->selSpec( false );
    if ( selspec.isZTransformed() )
	return;

    auto dp = createDataPackRM( selspec );
    if ( dp )
	viewer2D()->makeUpView( dp, FlatView::Viewer::VD );
}


void uiODView2DVariableDensityTreeItem::displayMiniCtab(
						const ColTab::Sequence* seq )
{
    if ( !seq )
    {
	uiTreeItem::updateColumnText( uiODViewer2DMgr::cColorColumn() );
	return;
    }

    uitreeviewitem_->setPixmap( uiODViewer2DMgr::cColorColumn(), *seq );
}


bool uiODView2DVariableDensityTreeItem::showSubMenu()
{
    if ( !menu_ )
    {
	menu_ = new uiMenuHandler( getUiParent(), -1 );
	menu_->ref();
	mAttachCB( menu_->createnotifier,
		   uiODView2DVariableDensityTreeItem::createMenuCB );
	mAttachCB( menu_->handlenotifier,
		   uiODView2DVariableDensityTreeItem::handleMenuCB );
    }
    return menu_->executeMenu(uiMenuHandler::fromTree());
}


void uiODView2DVariableDensityTreeItem::createMenuCB( CallBacker* cb )
{
    selattrmnuitem_.removeItems();
    createSelMenu( selattrmnuitem_ );

    mDynamicCastGet(MenuHandler*,menu,cb);
    if ( selattrmnuitem_.nrItems() )
	mAddMenuItem( menu, &selattrmnuitem_, true, false );
}


void uiODView2DVariableDensityTreeItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( handleSelMenu(mnuid) )
	menu->setIsHandled(true);
}


void uiODView2DVariableDensityTreeItem::createSelMenu( MenuItem& mnu )
{
    const uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    ConstRefMan<FlatDataPack> dp = vwr.getPack( false, true ).get();
    if ( !dp )
	return;

    uiAttribPartServer* attrserv = applMgr()->attrServer();
    const Attrib::SelSpec& as = viewer2D()->selSpec( false );
    MenuItem* subitem = 0;
    attrserv->resetMenuItems();

    mDynamicCastGet(const RegularFlatDataPack*,regfdp,dp.ptr());
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


bool uiODView2DVariableDensityTreeItem::handleSelMenu( int mnuid )
{
    uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    ConstRefMan<FlatDataPack> dp = vwr.getPack( false, true ).get();
    if ( !dp )
	return false;

    uiAttribPartServer* attrserv = applMgr()->attrServer();
    bool dousemulticomp, stored, steering;
    dousemulticomp = stored = steering = false;

    BufferString attrbnm;
    mDynamicCastGet(const RegularFlatDataPack*,regfdp,dp.ptr());
    if ( regfdp && regfdp->is2D() )
	attrserv->info2DAttribSubMenu( mnuid, attrbnm, steering, stored );

    Attrib::SelSpec selas = viewer2D()->selSpec( false );
    if ( !stored && !attrserv->handleAttribSubMenu(mnuid,selas,dousemulticomp) )
	return false;

    auto fdp = createDataPackRM( selas, attrbnm.buf(), steering, stored );
    if ( !fdp )
	return false;

    viewer2D()->setSelSpec( &selas, FlatView::Viewer::VD );
    if ( !viewer2D()->useStoredDispPars(FlatView::Viewer::VD) )
    {
	ColTab::MapperSetup& vdmapper =
	    vwr.appearance().ddpars_.vd_.mappersetup_;
	if ( vdmapper.type_ != ColTab::MapperSetup::Fixed )
	    vdmapper.range_ = Interval<float>::udf();
    }

    const ColTab::Sequence seq( vwr.appearance().ddpars_.vd_.ctab_ );
    displayMiniCtab( &seq );

    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	FlatView::DataDispPars& ddpars =
	    viewer2D()->viewwin()->viewer(ivwr).appearance().ddpars_;
	ddpars.vd_.show_ = true;
    }

    viewer2D()->makeUpView( fdp, FlatView::Viewer::VD );
    return true;
}


RefMan<SeisFlatDataPack> uiODView2DVariableDensityTreeItem::createDataPackRM(
			Attrib::SelSpec& selas, const BufferString& attrbnm,
			const bool steering, const bool stored )
{
    const uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    ConstRefMan<FlatDataPack> dp = vwr.getPack( false, true ).get();
    if ( !dp )
	return nullptr;

    uiAttribPartServer* attrserv = applMgr()->attrServer();
    attrserv->setTargetSelSpec( selas );

    mDynamicCastGet(const RegularFlatDataPack*,regfdp,dp.ptr());
    mDynamicCastGet(const RandomFlatDataPack*,randfdp,dp.ptr());
    if ( regfdp && regfdp->is2D() )
    {
	if ( stored )
	{
	    const SeisIOObjInfo objinfo( attrbnm, Seis::Line );
	    if ( !objinfo.ioObj() )
		return nullptr;

	    Attrib::DescID attribid = attrserv->getStoredID(
			    objinfo.ioObj()->key(), true, steering ? 1 : 0 );
	    selas.set( attrbnm, attribid, false, 0 );
	    selas.set2DFlag();

	    const Attrib::DescSet* ds =
			Attrib::DSHolder().getDescSet( true, true );
	    if ( !ds )
		return nullptr;

	    selas.setRefFromID( *ds );
	    selas.setUserRef( attrbnm );

	    const Attrib::Desc* targetdesc = ds->getDesc( attribid );
	    if ( !targetdesc )
		return nullptr;

	    BufferString defstring;
	    targetdesc->getDefStr( defstring );
	    selas.setDefString( defstring );
	    attrserv->setTargetSelSpec( selas );
	}
    }
    else if ( randfdp )
    {
	auto rdp = attrserv->createRdmTrcsOutputRM( randfdp->zRange(),
						randfdp->getRandomLineID() );

	return rdp ? viewer2D()->createFlatDataPackRM( *rdp, 0 ) : nullptr;
    }

    return viewer2D()->createDataPackRM( selas );
}


uiTreeItem* uiODView2DVariableDensityTreeItemFactory::createForVis(
					const uiODViewer2D&, Vis2DID) const
{
    return nullptr;
}


DataPackID uiODView2DVariableDensityTreeItem::createDataPack(
			Attrib::SelSpec& selas, const BufferString& attrbnm,
			const bool steering, const bool stored )
{
    return DataPack::cNoID();
}

