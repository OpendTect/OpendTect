/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uiodviewer2d.h"

#include "uiattribpartserv.h"
#include "uiflatauxdataeditor.h"
#include "uiflatviewdockwin.h"
#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
#include "uiflatviewslicepos.h"
#include "uiflatviewstdcontrol.h"
#include "uiflatviewpropdlg.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uiodmain.h"
#include "uiodviewer2dmgr.h"
#include "uiodvw2dtreeitem.h"
#include "uitoolbar.h"
#include "uivispartserv.h"
#include "pixmap.h"

#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "attribsel.h"
#include "settings.h"
#include "sorting.h"
#include "survinfo.h"

#include "visvw2ddataman.h"
#include "visvw2ddata.h"

static void initSelSpec( Attrib::SelSpec& as )
{ as.set( 0, Attrib::SelSpec::cNoAttrib(), false, 0 ); }

uiODViewer2D::uiODViewer2D( uiODMain& appl, int visid )
    : appl_(appl)
    , visid_(visid)
    , vdselspec_(*new Attrib::SelSpec)
    , wvaselspec_(*new Attrib::SelSpec)
    , viewwin_(0)
    , slicepos_(0)
    , viewstdcontrol_(0)
    , datamgr_(new Vw2DDataManager)
    , tifs_(0)
    , treetp_(0)
    , polyseltbid_(-1)
    , isPolySelect_(true)
{
    basetxt_ = "2D Viewer - ";
    BufferString info;
    appl.applMgr().visServer()->getObjectInfo( visid, info );
    if ( info.isEmpty() )
	info = appl.applMgr().visServer()->getObjectName( visid );
    basetxt_ += info;

    linesetid_.setEmpty();

    initSelSpec( vdselspec_ );
    initSelSpec( wvaselspec_ );
}


uiODViewer2D::~uiODViewer2D()
{
    mDynamicCastGet(uiFlatViewDockWin*,fvdw,viewwin())
    if ( fvdw )
	appl_.removeDockWindow( fvdw );

    delete treetp_;
    delete datamgr_;

    deepErase( auxdataeditors_ );
    delete viewwin();
    viewwin_ = 0;
}


void uiODViewer2D::setUpView( DataPack::ID packid, bool wva )
{
    DataPack* dp = DPM(DataPackMgr::FlatID()).obtain( packid, true );
    mDynamicCastGet(Attrib::Flat3DDataPack*,dp3d,dp)
    mDynamicCastGet(Attrib::Flat2DDataPack*,dp2d,dp)
    mDynamicCastGet(Attrib::Flat2DDHDataPack*,dp2ddh,dp)
    mDynamicCastGet(Attrib::FlatRdmTrcsDataPack*,dprdm,dp)

    const bool isnew = !viewwin();
    if ( isnew )
    {
	if ( dp3d || dprdm )
	    tifs_ = ODMainWin()->viewer2DMgr().treeItemFactorySet3D();
	else if ( dp2ddh )
	    tifs_ = ODMainWin()->viewer2DMgr().treeItemFactorySet2D();

	createViewWin( (dp3d && dp3d->isVertical()) ||
		       (dp2d && dp2d->isVertical()) );
    }

    if ( slicepos_ )
	slicepos_->getToolBar()->display( dp3d );

    for ( int ivwr=0; ivwr<viewwin()->nrViewers(); ivwr++ )
    {
	DataPack::ID curpackid = viewwin()->viewer(ivwr).packID( wva );
	viewwin()->viewer(ivwr).removePack( curpackid );
	DPM(DataPackMgr::FlatID()).release( curpackid );

	FlatView::DataDispPars& ddp = 
	    		viewwin()->viewer(ivwr).appearance().ddpars_;
	(wva ? ddp.wva_.show_ : ddp.vd_.show_) = true;
	DPM(DataPackMgr::FlatID()).obtain( packid, false );
	viewwin()->viewer(ivwr).setPack( wva, packid, false, isnew );
    }

    if( dp3d )
    {
	cs_ = dp3d->cube().cubeSampling();
	if ( slicepos_ ) slicepos_->setCubeSampling( cs_ );
	adjustOthrDisp( wva, cs_ );
    }
    
    //updating stuff
    if ( treetp_ )
    {
	treetp_->updSelSpec( &wvaselspec_, true );
	treetp_->updSelSpec( &vdselspec_, false );

	if ( dp3d )
	    treetp_->updCubeSamling( dp3d->cube().cubeSampling(), true );
	else if ( dp2ddh )
	    treetp_->updCubeSamling( dp2ddh->getCubeSampling(), true );
    }
    
    viewwin()->start();
}


void uiODViewer2D::adjustOthrDisp( bool wva, const CubeSampling& cs )
{
    const DataPack* othrdp = viewwin()->viewer(0).pack( !wva );
    if ( !othrdp ) return;

    mDynamicCastGet(const Attrib::Flat3DDataPack*,othrdp3d,othrdp);
    if ( !othrdp3d )  return;

    if ( othrdp3d->cube().cubeSampling() == cs ) return;

    uiAttribPartServer* attrserv = appl_.applMgr().attrServer();
    attrserv->setTargetSelSpec( wva ? wvaselspec_ : vdselspec_ );
    const DataPack::ID newid = attrserv->createOutput( cs, DataPack::cNoID() );
    if ( newid == DataPack::cNoID() ) return;

    for ( int ivwr=0; ivwr<viewwin()->nrViewers(); ivwr++ )
    {
	DataPack::ID othrcurpackid = viewwin()->viewer(ivwr).packID( !wva );
	viewwin()->viewer(ivwr).removePack( othrcurpackid );
	DPM(DataPackMgr::FlatID()).release( othrcurpackid );
	viewwin()->viewer(ivwr).setPack( !wva, newid, false, false );
    }
}


void uiODViewer2D::createViewWin( bool isvert )
{    
    bool wantdock = false;
    Settings::common().getYN( "FlatView.Use Dockwin", wantdock );
    uiParent* controlparent = 0;
    if ( !wantdock )
    {
	uiFlatViewMainWin* fvmw = new uiFlatViewMainWin( 0,
		uiFlatViewMainWin::Setup(basetxt_).deleteonclose(true)
	       					  .withhanddrag(true) );
	fvmw->windowClosed.notify( mCB(this,uiODViewer2D,winCloseCB) );

	slicepos_ = new uiSlicePos2DView( fvmw );
	slicepos_->positionChg.notify( mCB(this,uiODViewer2D,posChg) );
	viewwin_ = fvmw;

	createTree( fvmw );
    }
    else
    {
	uiFlatViewDockWin* dwin = new uiFlatViewDockWin( &appl_,
				   uiFlatViewDockWin::Setup(basetxt_) );
	appl_.addDockWindow( *dwin, uiMainWin::Top );
	dwin->setFloating( true );
	viewwin_ = dwin;
	controlparent = &appl_;
    }

    viewwin_->setInitialSize( 600, 400 );
    for ( int ivwr=0; ivwr<viewwin_->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewwin()->viewer( ivwr);
	vwr.appearance().setDarkBG( wantdock );
	vwr.appearance().setGeoDefaults(isvert);
	vwr.appearance().annot_.setAxesAnnot(true);
    }

    uiFlatViewer& mainvwr = viewwin()->viewer();
    viewstdcontrol_ = new uiFlatViewStdControl( mainvwr,
	    uiFlatViewStdControl::Setup(controlparent).helpid("51.0.0")
						      .withedit(true) );
    createPolygonSelBut( viewstdcontrol_->toolBar() );
    viewwin_->addControl( viewstdcontrol_ );
    createViewWinEditors();
}


void uiODViewer2D::createTree( uiMainWin* mw )
{
    if ( !mw || !tifs_ ) return;

    uiDockWin* treedoc = new uiDockWin( mw, "Tree items" );
    treedoc->setMinimumWidth( 200 );
    uiListView* lv = new uiListView( treedoc, "Tree items" );
    treedoc->setObject( lv );
    BufferStringSet labels;
    labels.add( "Elements" );
    labels.add( "Color" );
    lv->addColumns( labels );
    lv->setFixedColumnWidth( uiODViewer2DMgr::cColorColumn(), 40 );

    treetp_ = new uiODVw2DTreeTop( lv, &appl_.applMgr(), this, tifs_ );
    
    TypeSet<int> idxs;
    TypeSet<int> placeidxs;
    
    for ( int idx=0; idx < tifs_->nrFactories(); idx++ )
    {
	SurveyInfo::Pol2D pol2d = (SurveyInfo::Pol2D)tifs_->getPol2D( idx );
	if ( SI().survDataType() == SurveyInfo::Both2DAnd3D
	     || pol2d == SurveyInfo::Both2DAnd3D
	     || pol2d == SI().survDataType() )
	{
	    idxs += idx;
	    placeidxs += tifs_->getPlacementIdx(idx);
	}
    }

    sort_coupled( placeidxs.arr(), idxs.arr(), idxs.size() );
    
    for ( int iidx=0; iidx<idxs.size(); iidx++ )
    {
	const int fidx = idxs[iidx];
	treetp_->addChild( tifs_->getFactory(iidx)->create(), true );
    }

    lv->display( true );
    mw->addDockWindow( *treedoc, uiMainWin::Left );
    treedoc->display( true );
}


void uiODViewer2D::createPolygonSelBut( uiToolBar* tb )
{
    if ( !tb ) return;

    polyseltbid_ = tb->addButton( "polygonselect.png", "Polygon Selection mode",
	    			  mCB(this,uiODViewer2D,selectionMode), true );
    uiPopupMenu* polymnu = new uiPopupMenu( tb, "PoluMenu" );

    uiMenuItem* polyitm = new uiMenuItem( "Polygon",
	    			      mCB(this,uiODViewer2D,handleToolClick) );
    polymnu->insertItem( polyitm, 0 );
    polyitm->setPixmap( ioPixmap("polygonselect.png") );

    uiMenuItem* rectitm = new uiMenuItem( "Rectangle",
	    			      mCB(this,uiODViewer2D,handleToolClick) );
    polymnu->insertItem( rectitm, 1 );
    rectitm->setPixmap( ioPixmap("rectangleselect.png") );

    tb->setButtonMenu( polyseltbid_, polymnu );

    tb->addButton( "trashcan.png", "Remove PolySelection",
			mCB(this,uiODViewer2D,removeSelected), false );
}


void uiODViewer2D::createViewWinEditors()
{   
    for ( int ivwr=0; ivwr<viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewwin()->viewer( ivwr);
	uiFlatViewAuxDataEditor* adeditor = new uiFlatViewAuxDataEditor( vwr );
	adeditor->setSelActive( false );
	auxdataeditors_ += adeditor;
    }
} 


void uiODViewer2D::winCloseCB( CallBacker* cb )
{
    for ( int ivwr=0; ivwr<viewwin()->nrViewers(); ivwr++ )
    {
	DataPack::ID packid = viewwin()->viewer( ivwr ).packID( true );
	DPM(DataPackMgr::FlatID()).release( packid );
	packid = viewwin()->viewer( ivwr ).packID( false );
	DPM(DataPackMgr::FlatID()).release( packid );
    }
    
    delete treetp_; treetp_ = 0;
    datamgr_->removeAll();

    deepErase( auxdataeditors_ );

    mDynamicCastGet(uiMainWin*,mw,cb)
    if ( mw ) mw->windowClosed.remove( mCB(this,uiODViewer2D,winCloseCB) );
    if ( slicepos_ )
	slicepos_->positionChg.remove( mCB(this,uiODViewer2D,posChg) );

    viewstdcontrol_ = 0;
}


void uiODViewer2D::setSelSpec( const Attrib::SelSpec* as, bool wva )
{
    if ( as )
	(wva ? wvaselspec_ : vdselspec_) = *as;
    else
	initSelSpec( wva ? wvaselspec_ : vdselspec_ );
}


void uiODViewer2D::posChg( CallBacker* )
{
    if ( !slicepos_ ) return;

    cs_ = slicepos_->getCubeSampling();
    setPos( cs_ );
}


void uiODViewer2D::setPos( const CubeSampling& cs )
{
    uiAttribPartServer* attrserv = appl_.applMgr().attrServer();
    if ( vdselspec_.id().asInt() > -1 )
    {
	attrserv->setTargetSelSpec( vdselspec_ );
	DataPack::ID dpid = attrserv->createOutput( cs, DataPack::cNoID() );
	setUpView( dpid, false );
    }
    else if ( wvaselspec_.id().asInt() > -1 )
    {
	attrserv->setTargetSelSpec( wvaselspec_ );
	DataPack::ID dpid = attrserv->createOutput( cs, DataPack::cNoID() );
	setUpView( dpid, true );
    }
}


void uiODViewer2D::selectionMode( CallBacker* cb )
{
    if ( !viewstdcontrol_ || !viewstdcontrol_->toolBar() )
	return;

    viewstdcontrol_->toolBar()->setPixmap( polyseltbid_, isPolySelect_ ?
	    			"polygonselect.png" : "rectangleselect.png" );
    viewstdcontrol_->toolBar()->setToolTip( polyseltbid_, isPolySelect_ ?
	    		"Polygon Selection mode" : "Rectangle Selection mode" );

    if ( !auxdataeditors_.size() )
	return;

    for ( int edidx=0; edidx<auxdataeditors_.size(); edidx++ )
    {
	auxdataeditors_[edidx]->setSelectionPolygonRectangle( !isPolySelect_ );
	auxdataeditors_[edidx]->setSelActive(
		viewstdcontrol_->toolBar()->isOn(polyseltbid_) );
    }
}


void uiODViewer2D::handleToolClick( CallBacker* cb )
{
    mDynamicCastGet(uiMenuItem*,itm,cb)
    if ( !itm ) return;

    isPolySelect_ = itm->id()==0;
    selectionMode( cb );
}


void uiODViewer2D::removeSelected( CallBacker* cb )
{
    if ( !viewstdcontrol_->toolBar()->isOn(polyseltbid_) )
	return;

    if ( !auxdataeditors_.size() )
	return;

    for ( int edidx=0; edidx<auxdataeditors_.size(); edidx++ )
    {
	auxdataeditors_[edidx]->removePolygonSelected( -1 );
    }
}


void uiODViewer2D::usePar( const IOPar& iop )
{
    if ( !viewwin() ) 
	return;

    IOPar* vdselspecpar = iop.subselect( sKeyVDSelSpec() );
    if ( vdselspecpar ) vdselspec_.usePar( *vdselspecpar );
    IOPar* wvaselspecpar = iop.subselect( sKeyWVASelSpec() );
    if ( wvaselspecpar ) wvaselspec_.usePar( *wvaselspecpar );
    delete vdselspecpar; delete wvaselspecpar;
    IOPar* cspar = iop.subselect( sKeyPos() );
    if ( cspar ) cs_.usePar( *cspar );
    if ( viewwin()->nrViewers() > 0 )
    {
	const bool iswva = wvaselspec_.id().isValid();
	const DataPack* dp = viewwin()->viewer(0).pack( iswva );
	mDynamicCastGet(const Attrib::Flat3DDataPack*,dp3d,dp)
	if ( dp3d )
	    setPos( cs_ );
    }

    datamgr_->usePar( iop, viewwin(), dataEditor() );
    rebuildTree();
}


void uiODViewer2D::fillPar( IOPar& iop ) const
{
    IOPar vdselspecpar, wvaselspecpar;
    vdselspec_.fillPar( vdselspecpar );
    wvaselspec_.fillPar( wvaselspecpar );
    iop.mergeComp( vdselspecpar, sKeyVDSelSpec() );
    iop.mergeComp( wvaselspecpar, sKeyWVASelSpec() );
    IOPar pospar; cs_.fillPar( pospar );
    iop.mergeComp( pospar, sKeyPos() );

    datamgr_->fillPar( iop );
}


void uiODViewer2D::rebuildTree()
{
    ObjectSet<Vw2DDataObject> objs;
    dataMgr()->getObjects( objs );
    for ( int iobj=0; iobj<objs.size(); iobj++ )
	uiODVw2DTreeItem::create( treeTop(), *this, objs[iobj]->id() );
}
