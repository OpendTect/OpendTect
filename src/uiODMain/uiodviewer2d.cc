/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiodviewer2d.h"

#include "uiattribpartserv.h"
#include "uiflatauxdataeditor.h"
#include "uiflatviewdockwin.h"
#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
#include "uiflatviewslicepos.h"
#include "uiflatviewstdcontrol.h"
#include "uiflatviewpropdlg.h"
#include "uimenu.h"
#include "uiodmain.h"
#include "uiodviewer2dmgr.h"
#include "uiodvw2dtreeitem.h"
#include "uitoolbar.h"
#include "uitreeview.h"
#include "uivispartserv.h"
#include "pixmap.h"

#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "attribsel.h"
#include "mouseevent.h"
#include "settings.h"
#include "sorting.h"
#include "survinfo.h"

#include "visvw2ddataman.h"
#include "visvw2ddata.h"

static void initSelSpec( Attrib::SelSpec& as )
{ as.set( 0, Attrib::SelSpec::cNoAttrib(), false, 0 ); }

mDefineInstanceCreatedNotifierAccess( uiODViewer2D )

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
    , ispolyselect_(true)
    , viewWinAvailable(this)
    , viewWinClosed(this)
    , dataChanged(this)
    , mousecursorexchange_(0)
    , marker_(0)
{
    setWinTitle();
    linesetid_.setEmpty();

    initSelSpec( vdselspec_ );
    initSelSpec( wvaselspec_ );

    mTriggerInstanceCreatedNotifier();
}


uiODViewer2D::~uiODViewer2D()
{
    mDynamicCastGet(uiFlatViewDockWin*,fvdw,viewwin())
    if ( fvdw )
	appl_.removeDockWindow( fvdw );

    delete treetp_;
    delete datamgr_;

    deepErase( auxdataeditors_ );
    setMouseCursorExchange( 0 );
    detachAllNotifiers();

    if ( viewwin() )
    {
	removeAvailablePacks();
	viewwin()->viewer(0).removeAuxData( marker_ );
    }
    delete marker_;
    delete viewwin();
}


void uiODViewer2D::setUpView( DataPack::ID packid, bool wva )
{
    DataPackMgr& dpm = DPM(DataPackMgr::FlatID());
    DataPack* dp = dpm.obtain( packid, true );
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
		       (dp2d && dp2d->isVertical()), dp3d );
	if ( slicepos_ ) slicepos_->getToolBar()->display( dp3d );
    }

    if ( dp3d )
    {
	const CubeSampling cs = dp3d->cube().cubeSampling();
	if ( cs != cs_ ) removeAvailablePacks();
	cs_ = cs; if ( slicepos_ ) slicepos_->setCubeSampling( cs_ );
    }

    setDataPack( packid, wva, isnew ); adjustOthrDisp( wva, isnew );

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


void uiODViewer2D::adjustOthrDisp( bool wva, bool isnew )
{
    const uiFlatViewer& vwr = viewwin()->viewer(0);
    const FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;
    const bool setpack = ( !wva ? ddp.wva_.show_ : ddp.vd_.show_ );
    if ( !slicepos_ || !setpack ) return;

    const CubeSampling cs = slicepos_->getCubeSampling();
    const bool newcs = ( cs != cs_ );
    const DataPack::ID othrdpid = newcs ? createDataPack(!wva)
					: getDataPackID(!wva);
    if ( newcs && (othrdpid != DataPack::cNoID()) )
    { removeAvailablePacks(); cs_ = cs; slicepos_->setCubeSampling( cs_ ); }
    setDataPack( othrdpid, !wva, isnew );
}


void uiODViewer2D::setDataPack( DataPack::ID packid, bool wva, bool isnew )
{
    if ( packid == DataPack::cNoID() ) return;

    DataPackMgr& dpm = DPM(DataPackMgr::FlatID());
    for ( int ivwr=0; ivwr<viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewwin()->viewer(ivwr);
	FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;
	(wva ? ddp.wva_.show_ : ddp.vd_.show_) = true;

	TypeSet<DataPack::ID> ids = vwr.availablePacks();
	if ( ids.isPresent(packid) )
	{ vwr.setPack( wva, packid, true, isnew ); continue; }

	const FixedString newpackname = dpm.nameOf(packid);
	bool setforotherdisp = false;
	for ( int idx=0; idx<ids.size(); idx++ )
	{
	    const FixedString packname = dpm.nameOf(ids[idx]);
	    if ( packname == newpackname )
	    {
		if ( ids[idx] == vwr.packID(!wva) )
		    setforotherdisp = true;
		vwr.removePack( ids[idx] );
		dpm.release( ids[idx] );
		break;
	    }
	}

	dpm.obtain( packid, false );
	vwr.setPack( wva, packid, false, isnew );
	if ( isnew || setforotherdisp )
	    vwr.setPack( !wva, packid, true, isnew );
    }

    dataChanged.trigger( this );
}


void uiODViewer2D::createViewWin( bool isvert, bool needslicepos )
{
    bool wantdock = false;
    Settings::common().getYN( "FlatView.Use Dockwin", wantdock );
    uiParent* controlparent = 0;
    if ( !wantdock )
    {
	uiFlatViewMainWin* fvmw = new uiFlatViewMainWin( &appl_,
		uiFlatViewMainWin::Setup(basetxt_).deleteonclose(true)
						  .withhanddrag(true) );
	mAttachCB( fvmw->windowClosed, uiODViewer2D::winCloseCB );

	if ( needslicepos )
	{
	    slicepos_ = new uiSlicePos2DView( fvmw );
	    mAttachCB( slicepos_->positionChg, uiODViewer2D::posChg );
	}

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
    mAttachCB( viewstdcontrol_->infoChanged, uiODViewer2D::mouseMoveCB );
    createPolygonSelBut( viewstdcontrol_->toolBar() );
    viewwin_->addControl( viewstdcontrol_ );
    createViewWinEditors();

    viewWinAvailable.trigger( this );
}


void uiODViewer2D::createTree( uiMainWin* mw )
{
    if ( !mw || !tifs_ ) return;

    uiDockWin* treedoc = new uiDockWin( mw, "Tree items" );
    treedoc->setMinimumWidth( 200 );
    uiTreeView* lv = new uiTreeView( treedoc, "Tree items" );
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
	treetp_->addChild( tifs_->getFactory(fidx)->create(), true );
    }

    lv->display( true );
    mw->addDockWindow( *treedoc, uiMainWin::Left );
    treedoc->display( true );
}


void uiODViewer2D::createPolygonSelBut( uiToolBar* tb )
{
    if ( !tb ) return;

    polyseltbid_ = tb->addButton( "polygonselect", "Polygon Selection mode",
				  mCB(this,uiODViewer2D,selectionMode), true );
    uiMenu* polymnu = new uiMenu( tb, "PoluMenu" );

    uiAction* polyitm = new uiAction( "Polygon",
				      mCB(this,uiODViewer2D,handleToolClick) );
    polymnu->insertItem( polyitm, 0 );
    polyitm->setIcon( ioPixmap("polygonselect") );

    uiAction* rectitm = new uiAction( "Rectangle",
				      mCB(this,uiODViewer2D,handleToolClick) );
    polymnu->insertItem( rectitm, 1 );
    rectitm->setIcon( ioPixmap("rectangleselect") );

    tb->setButtonMenu( polyseltbid_, polymnu );

    tb->addButton( "trashcan", "Remove PolySelection",
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
    delete treetp_; treetp_ = 0;
    datamgr_->removeAll();

    deepErase( auxdataeditors_ );
    removeAvailablePacks();

    viewWinClosed.trigger();
}


void uiODViewer2D::removeAvailablePacks()
{
    if ( !viewwin() ) { pErrMsg("No main window"); return; }

    DataPackMgr& dpm = DPM(DataPackMgr::FlatID());
    for ( int ivwr=0; ivwr<viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewwin()->viewer(ivwr);
	TypeSet<DataPack::ID> ids = vwr.availablePacks();
	for ( int idx=0; idx<ids.size(); idx++ )
	{
	    vwr.removePack( ids[idx] );
	    dpm.release( ids[idx] );
	}
    }
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
    const CubeSampling cs = slicepos_->getCubeSampling();
    setPos( cs ); setWinTitle( true );
    slicepos_->setCubeSampling( cs_ );
}


void uiODViewer2D::setPos( const CubeSampling& cs )
{
    const uiFlatViewer& vwr = viewwin()->viewer(0);
    const bool shwvd = vwr.isVisible(false);
    const bool shwwva = vwr.isVisible(true);
    if ( cs == cs_ ) return;

    if ( shwvd && vdselspec_.id().isValid() )
    {
	const DataPack::ID dpid = createDataPack(false);
	setUpView( dpid, false );
    }
    else if ( shwwva && wvaselspec_.id().isValid() )
    {
	const DataPack::ID dpid = createDataPack(true);
	setUpView( dpid, true );
    }
}


DataPack::ID uiODViewer2D::getDataPackID( bool wva ) const
{
    const uiFlatViewer& vwr = viewwin()->viewer(0);
    if ( vwr.hasPack(wva) )
	return vwr.packID(wva);
    else if ( wvaselspec_ == vdselspec_ )
    {
	const DataPack::ID dpid = vwr.packID(!wva);
	if ( dpid != DataPack::cNoID() ) return dpid;
    }
    return createDataPack( wva );
}


DataPack::ID uiODViewer2D::createDataPack( bool wva ) const
{
    if ( !slicepos_ ) return DataPack::cNoID();
    uiAttribPartServer* attrserv = appl_.applMgr().attrServer();
    attrserv->setTargetSelSpec( wva ? wvaselspec_ : vdselspec_ );
    const CubeSampling cs = slicepos_->getCubeSampling();
    return attrserv->createOutput( cs, DataPack::cNoID() );
}


void uiODViewer2D::selectionMode( CallBacker* cb )
{
    if ( !viewstdcontrol_ || !viewstdcontrol_->toolBar() )
	return;

    viewstdcontrol_->toolBar()->setIcon( polyseltbid_, ispolyselect_ ?
				"polygonselect" : "rectangleselect" );
    viewstdcontrol_->toolBar()->setToolTip( polyseltbid_, ispolyselect_ ?
			"Polygon Selection mode" : "Rectangle Selection mode" );

    if ( auxdataeditors_.isEmpty() )
	return;

    for ( int edidx=0; edidx<auxdataeditors_.size(); edidx++ )
    {
	auxdataeditors_[edidx]->setSelectionPolygonRectangle( !ispolyselect_ );
	auxdataeditors_[edidx]->setSelActive(
		viewstdcontrol_->toolBar()->isOn(polyseltbid_) );
    }
}


void uiODViewer2D::handleToolClick( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,itm,cb)
    if ( !itm ) return;

    ispolyselect_ = itm->getID()==0;
    selectionMode( cb );
}


void uiODViewer2D::removeSelected( CallBacker* cb )
{
    if ( !viewstdcontrol_->toolBar()->isOn(polyseltbid_) )
	return;

    if ( auxdataeditors_.isEmpty() )
	return;

    for ( int edidx=0; edidx<auxdataeditors_.size(); edidx++ )
    {
	auxdataeditors_[edidx]->removePolygonSelected( -1 );
    }
}


void uiODViewer2D::setWinTitle( bool fromcs )
{
    basetxt_ = "2D Viewer - ";
    BufferString info;
    if ( !fromcs )
    {
	appl_.applMgr().visServer()->getObjectInfo( visid_, info );
	if ( info.isEmpty() )
	    info = appl_.applMgr().visServer()->getObjectName( visid_ );
    }
    else
    {
	if ( cs_.defaultDir() == CubeSampling::Z )
	{
	    const ZDomain::Def& zdef = SI().zDomain();
	    info = zdef.userName(); info += ": ";
	    info += cs_.zrg.start * zdef.userFactor();
	}
	else if ( cs_.defaultDir() == CubeSampling::Crl )
	{ info = "Crossline: "; info += cs_.hrg.start.crl(); }
	else
	{ info = "Inline: "; info += cs_.hrg.start.inl(); }
    }

    basetxt_ += info; if ( viewwin() ) viewwin()->setWinTitle( basetxt_ );
}


void uiODViewer2D::usePar( const IOPar& iop )
{
    if ( !viewwin() ) return;

    IOPar* vdselspecpar = iop.subselect( sKeyVDSelSpec() );
    if ( vdselspecpar ) vdselspec_.usePar( *vdselspecpar );
    IOPar* wvaselspecpar = iop.subselect( sKeyWVASelSpec() );
    if ( wvaselspecpar ) wvaselspec_.usePar( *wvaselspecpar );
    delete vdselspecpar; delete wvaselspecpar;
    IOPar* cspar = iop.subselect( sKeyPos() );
    CubeSampling cs; if ( cspar ) cs.usePar( *cspar );
    if ( viewwin()->nrViewers() > 0 )
    {
	const uiFlatViewer& vwr = viewwin()->viewer(0);
	const bool iswva = wvaselspec_.id().isValid();
	ConstDataPackRef<Attrib::Flat3DDataPack> dp3d = vwr.obtainPack(iswva);
	if ( dp3d ) setPos( cs );
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


void uiODViewer2D::setMouseCursorExchange( MouseCursorExchange* mce )
{
    if ( mousecursorexchange_ )
	mousecursorexchange_->notifier.remove(
		mCB(this,uiODViewer2D,mouseCursorCB) );

    mousecursorexchange_ = mce;

    if ( mousecursorexchange_ )
	mousecursorexchange_->notifier.notify(
		mCB(this,uiODViewer2D,mouseCursorCB) );
}


void uiODViewer2D::mouseCursorCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(const MouseCursorExchange::Info&,info,
			       caller,cb);
    if ( caller==this )
	return;

    uiFlatViewer& vwr = viewwin()->viewer(0);
    if ( !marker_ )
    {
	marker_ = vwr.createAuxData( "XYZ Marker" );
	vwr.addAuxData( marker_ );
	marker_->poly_ += FlatView::Point(0,0);
	marker_->markerstyles_ += MarkerStyle2D();
    }

    const BinID bid = SI().transform( info.surveypos_.coord() );
    FlatView::Point& pt = marker_->poly_[0];
    if ( cs_.defaultDir() == CubeSampling::Inl )
	pt = FlatView::Point( bid.crl(), info.surveypos_.z );
    else if ( cs_.defaultDir() == CubeSampling::Crl )
	pt = FlatView::Point( bid.inl(), info.surveypos_.z );
    else
	pt = FlatView::Point( bid.inl(), bid.crl() );

    vwr.handleChange( FlatView::Viewer::Auxdata );
}


void uiODViewer2D::mouseMoveCB( CallBacker* cb )
{
    Coord3 mousepos( Coord3::udf() );
    mCBCapsuleUnpack(IOPar,pars,cb);

    FixedString valstr = pars.find( "X" );
    if ( valstr.isEmpty() ) valstr = pars.find( "X-coordinate" );
    if ( !valstr.isEmpty() ) mousepos.x = valstr.toDouble();
    valstr = pars.find( "Y" );
    if ( valstr.isEmpty() ) valstr = pars.find( "Y-coordinate" );
    if ( !valstr.isEmpty() ) mousepos.y = valstr.toDouble();
    valstr = pars.find( "Z" );
    if ( !valstr.isEmpty() ) mousepos.z = valstr.toDouble()/1000.;

    if ( mousecursorexchange_ && mousepos.isDefined() )
    {
	MouseCursorExchange::Info info( mousepos );
	mousecursorexchange_->notifier.trigger( info, this );
    }
}
