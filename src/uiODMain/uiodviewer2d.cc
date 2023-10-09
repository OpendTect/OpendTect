/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodviewer2d.h"

#include "uiattribpartserv.h"
#include "uiflatauxdataeditor.h"
#include "uiflatviewdockwin.h"
#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
#include "uiflatviewslicepos.h"
#include "uiflatviewstdcontrol.h"
#include "uimenu.h"
#include "uimpepartserv.h"
#include "uiodmain.h"
#include "uiodviewer2dmgr.h"
#include "uiodscenemgr.h"
#include "uiodvw2dtreeitem.h"
#include "uiodvw2dhor3dtreeitem.h"
#include "uiodvw2dhor2dtreeitem.h"
#include "uiodvw2dfaulttreeitem.h"
#include "uiodvw2dfaultsstreeitem.h"
#include "uiodvw2dfaultss2dtreeitem.h"
#include "uiodvw2dpicksettreeitem.h"
#include "uipixmap.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"
#include "uitreeview.h"
#include "uivispartserv.h"
#include "uimsg.h"

#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "emmanager.h"
#include "emobject.h"
#include "filepath.h"
#include "flatposdata.h"
#include "ioobj.h"
#include "mouseevent.h"
#include "scaler.h"
#include "seisdatapack.h"
#include "seisdatapackzaxistransformer.h"
#include "seisioobjinfo.h"
#include "settings.h"
#include "sorting.h"
#include "survinfo.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "posvecdataset.h"
#include "randomlinegeom.h"

#include "zaxistransform.h"
#include "zaxistransformutils.h"
#include "view2ddataman.h"
#include "view2ddata.h"
#include "od_helpids.h"

#include "hiddenparam.h"

static HiddenParam<uiODViewer2D,RefMan<SeisFlatDataPack>> hp_wvadp_(nullptr);
static HiddenParam<uiODViewer2D,RefMan<SeisFlatDataPack>> hp_vddp_(nullptr);

static void initSelSpec( Attrib::SelSpec& as )
{ as.set( 0, Attrib::SelSpec::cNoAttrib(), false, 0 ); }

mDefineInstanceCreatedNotifierAccess( uiODViewer2D )

uiODViewer2D::uiODViewer2D( uiODMain& appl, VisID visid )
    : appl_(appl)
    , visid_(visid)
    , vdselspec_(*new Attrib::SelSpec)
    , wvaselspec_(*new Attrib::SelSpec)
    , viewwin_(nullptr)
    , slicepos_(nullptr)
    , viewstdcontrol_(nullptr)
    , datamgr_(new View2D::DataManager)
    , tifs_(0)
    , treetp_(0)
    , polyseltbid_(-1)
    , voiidx_(-1)
    , basetxt_(tr("2D Viewer - "))
    , initialcentre_(uiWorldPoint::udf())
    , initialx1pospercm_(mUdf(float))
    , initialx2pospercm_(mUdf(float))
    , isvertical_(true)
    , ispolyselect_(true)
    , viewWinAvailable(this)
    , viewWinClosed(this)
    , dataChanged(this)
    , posChanged(this)
    , mousecursorexchange_(0)
    , marker_(0)
    , datatransform_(0)
{
    mDefineStaticLocalObject( Threads::Atomic<int>, vwrid, (0) );
    id_.set( vwrid++ );

    hp_wvadp_.setParam( this, nullptr );
    hp_vddp_.setParam( this, nullptr );

    setWinTitle( true );

    if ( visid_.isValid() )
	syncsceneid_ = appl_.applMgr().visServer()->getSceneID( visid_ );
    else
    {
	TypeSet<SceneID> sceneids;
	appl_.applMgr().visServer()->getSceneIds( sceneids );
	for ( int iscn=0; iscn<sceneids.size(); iscn++ )
	{
	    const SceneID sceneid = sceneids[iscn];
	    const ZAxisTransform* scntransform =
		appl_.applMgr().visServer()->getZAxisTransform( sceneid );
	    const ZDomain::Info* scnzdomaininfo =
		appl_.applMgr().visServer()->zDomainInfo( sceneid );
	    if ( datatransform_==scntransform ||
		 (scnzdomaininfo && scnzdomaininfo->def_==zDomain()) )
	    {
		syncsceneid_ = sceneid;
		break;
	    }
	}
    }

    initSelSpec( vdselspec_ );
    initSelSpec( wvaselspec_ );

    mTriggerInstanceCreatedNotifier();
}


uiODViewer2D::~uiODViewer2D()
{
    detachAllNotifiers();
    mDynamicCastGet(uiFlatViewDockWin*,fvdw,viewwin())
    if ( fvdw )
	appl_.removeDockWindow( fvdw );

    delete treetp_;
    delete datamgr_;

    deepErase( auxdataeditors_ );

    if ( datatransform_ )
    {
	if ( voiidx_ != -1 )
	    datatransform_->removeVolumeOfInterest( voiidx_ );
	voiidx_ = -1;
	datatransform_->unRef();
    }

    if ( viewwin() )
    {
	removeAvailablePacks();
	viewwin()->viewer(0).removeAuxData( marker_ );
    }
    delete marker_;
    delete viewwin();

    hp_wvadp_.setParam( this, nullptr );
    hp_vddp_.setParam( this, nullptr );
    hp_wvadp_.removeParam( this );
    hp_vddp_.removeParam( this );
}


const View2D::DataObject* uiODViewer2D::getObject( Vis2DID id ) const
{
    return datamgr_ ? datamgr_->getObject( id ) : nullptr;
}


View2D::DataObject* uiODViewer2D::getObject(Vis2DID id)
{
    return datamgr_ ? datamgr_->getObject( id ) : nullptr;
}


void uiODViewer2D::getObjects( ObjectSet<View2D::DataObject>& objs )const
{
    if ( datamgr_ )
	datamgr_->getObjects( objs );
}


SceneID uiODViewer2D::getSyncSceneID() const
{
    return syncsceneid_;
}


Pos::GeomID uiODViewer2D::geomID() const
{
    if ( tkzs_.is2D() )
	return tkzs_.hsamp_.getGeomID();

    return Survey::GM().cUndefGeomID();
}


const ZDomain::Def& uiODViewer2D::zDomain() const
{
    return datatransform_ ? datatransform_->toZDomainInfo().def_
			  : SI().zDomain();
}


uiParent* uiODViewer2D::viewerParent()
{ return viewwin_->viewerParent(); }


void uiODViewer2D::setUpAux()
{
    const bool is2d = geomID() != Survey::GM().cUndefGeomID();
    FlatView::Annotation& vwrannot = viewwin()->viewer().appearance().annot_;
    if ( !is2d && !tkzs_.isFlat() )
	vwrannot.x1_.showauxannot_ = vwrannot.x2_.showauxannot_ = false;
    else
    {
	vwrannot.x1_.showauxannot_ = true;
	uiString intersection = tr( "%1 intersection");
	if ( is2d )
	{
	    vwrannot.x2_.showauxannot_ = false;
	    vwrannot.x1_.auxlabel_ = intersection;
	    vwrannot.x1_.auxlabel_.arg( tr("2D Line") );
	    viewwin()->viewer().setAnnotChoice(
		SeisTrcInfo::toString(SeisTrcInfo::TrcNr) );
	}
	else
	{
	    vwrannot.x2_.showauxannot_ = true;
	    uiString& x1auxnm = vwrannot.x1_.auxlabel_;
	    uiString& x2auxnm = vwrannot.x2_.auxlabel_;

	    x1auxnm = intersection;
	    x2auxnm = intersection;

	    if ( tkzs_.defaultDir()==TrcKeyZSampling::Z )
	    {
		x1auxnm.arg( uiStrings::sInline() );
		x2auxnm.arg( uiStrings::sCrossline() );
	    }
	    else
	    {
		uiString axisnm = tkzs_.defaultDir()==TrcKeyZSampling::Inl ?
		    uiStrings::sCrossline() : uiStrings::sInline();
		x1auxnm.arg( axisnm );
		x2auxnm.arg( uiStrings::sZSlice() );
	    }
	}
    }
}


void uiODViewer2D::setUpView( DataPackID packid, bool wva )
{
    const FlatView::Viewer::VwrDest dest = FlatView::Viewer::getDest( wva,
								      !wva );

    makeUpView( packid, dest );
}


void uiODViewer2D::makeUpView( FlatDataPack* indp,
			       FlatView::Viewer::VwrDest dst )
{
    RefMan<FlatDataPack> fdp = indp;
    mDynamicCastGet(const SeisFlatDataPack*,seisfdp,fdp.ptr());
    mDynamicCastGet(const RegularFlatDataPack*,regfdp,fdp.ptr());
    mDynamicCastGet(const MapDataPack*,mapdp,fdp.ptr());

    const bool isnew = !viewwin();
    if ( isnew )
    {
	if ( regfdp && regfdp->is2D() )
	    tifs_ = ODMainWin()->viewer2DMgr().treeItemFactorySet2D();
	else if ( !mapdp )
	    tifs_ = ODMainWin()->viewer2DMgr().treeItemFactorySet3D();

	isvertical_ = seisfdp && seisfdp->isVertical();
	if ( regfdp )
	    setTrcKeyZSampling( regfdp->sampling() );
	createViewWin( isvertical_, regfdp && !regfdp->is2D() );
    }

    if ( regfdp )
    {
	const TrcKeyZSampling& cs = regfdp->sampling();
	if ( tkzs_.isFlat() || !cs.includes(tkzs_) )
	{
	    int nrtrcs;
	    if ( tkzs_.defaultDir()==TrcKeyZSampling::Inl )
		nrtrcs = cs.hsamp_.nrTrcs();
	    else
		nrtrcs = cs.hsamp_.nrLines();
	    //nrTrcs() or nrLines() return value 1 means start=stop
	    if ( nrtrcs < 2 )
	    {
		uiMSG().error( tr("No data available for current %1 position")
		    .arg(TrcKeyZSampling::toUiString(tkzs_.defaultDir())) );
		return;
	    }
	}
	if ( tkzs_ != cs ) { removeAvailablePacks(); setTrcKeyZSampling( cs ); }
    }

    if ( !isVertical() && !mapdp && regfdp )
    {
	viewwin()->viewer().appearance().annot_.x2_.reversed_ = false;
	setDataPack( createMapDataPackRM( *regfdp ), dst, isnew );
    }
    else
	setDataPack( fdp, dst, isnew );

    adjustOthrDisp( dst, isnew );

    //updating stuff
    if ( treetp_ )
    {
	treetp_->updSelSpec( &wvaselspec_, true );
	treetp_->updSelSpec( &vdselspec_, false );
	treetp_->updSampling( tkzs_, true );
    }

    if ( isnew )
	viewwin()->start();

    if ( viewwin()->dockParent() )
	viewwin()->dockParent()->raise();

}


void uiODViewer2D::makeUpView( DataPackID packid,
			       FlatView::Viewer::VwrDest dst )
{
    DataPackMgr& dpm = DPM(DataPackMgr::FlatID());
    RefMan<FlatDataPack> fdp = dpm.get<FlatDataPack>( packid );
    makeUpView( fdp, dst );
}


void uiODViewer2D::adjustOthrDisp( bool wva, bool isnew )
{
    const FlatView::Viewer::VwrDest dest = FlatView::Viewer::getDest( wva,
								      !wva );
    adjustOthrDisp( dest, isnew );
}


void uiODViewer2D::adjustOthrDisp( FlatView::Viewer::VwrDest dest, bool isnew )
{
    if ( !slicepos_ ) return;
    const TrcKeyZSampling& cs = slicepos_->getTrcKeyZSampling();
    const bool newcs = ( cs != tkzs_ );
    const bool wva = dest == FlatView::Viewer::WVA;

    RefMan<FlatDataPack> othrdp;
    othrdp = newcs ? createDataPackRM(!wva) : getDataPack(!wva);
    if ( newcs && othrdp )
	setTrcKeyZSampling( cs );

    setDataPack( othrdp, !wva, isnew );
}


void uiODViewer2D::setDataPack( FlatDataPack* indp,
				FlatView::Viewer::VwrDest dest, bool isnew )
{
    if ( !indp ) return;

    RefMan<FlatDataPack> fdp = indp;
    if ( dest == FlatView::Viewer::WVA || dest == FlatView::Viewer::Both )
	hp_wvadp_.setParam( this, fdp );

    if ( dest == FlatView::Viewer::VD || dest == FlatView::Viewer::Both )
	hp_vddp_.setParam( this, fdp );

    for ( int ivwr=0; ivwr<viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewwin()->viewer(ivwr);
	vwr.setPack( dest, fdp, isnew );
    }

    dataChanged.trigger( this );
}


void uiODViewer2D::setDataPack( FlatDataPack* indp, bool wva,
				bool isnew )
{
    const FlatView::Viewer::VwrDest dest = FlatView::Viewer::getDest( wva,
				  !wva || (isnew && wvaselspec_==vdselspec_) );
    setDataPack( indp, dest, isnew );
}


void uiODViewer2D::setDataPack( DataPackID packid, bool wva, bool isnew )
{
	const FlatView::Viewer::VwrDest dest = FlatView::Viewer::getDest( wva,
				   !wva || (isnew && wvaselspec_==vdselspec_) );

	setDataPack( packid, dest, isnew );
}


void uiODViewer2D::setDataPack( DataPackID packid,
				FlatView::Viewer::VwrDest dest,
				bool isnew )
{
    if ( packid == DataPack::cNoID() ) return;
    auto& dpm = DPM(DataPackMgr::FlatID());
    RefMan<FlatDataPack> fdp = dpm.get<FlatDataPack>( packid );
    setDataPack( fdp, dest, isnew );
}


bool uiODViewer2D::setZAxisTransform( ZAxisTransform* zat )
{
    if ( datatransform_ )
	datatransform_->unRef();

    datatransform_ = zat;
    if ( datatransform_ )
	datatransform_->ref();

    return true;
}


void uiODViewer2D::setTrcKeyZSampling( const TrcKeyZSampling& tkzs,
				       TaskRunner* taskr )
{
    if ( datatransform_ && datatransform_->needsVolumeOfInterest() )
    {
	if ( voiidx_!=-1 && tkzs!=tkzs_ )
	{
	    datatransform_->removeVolumeOfInterest( voiidx_ );
	    voiidx_ = -1;
	}

	if ( voiidx_ < 0 )
	    voiidx_ = datatransform_->addVolumeOfInterest( tkzs, true );
	else
	    datatransform_->setVolumeOfInterest( voiidx_, tkzs, true );

	datatransform_->loadDataIfMissing( voiidx_, taskr );
    }

    tkzs_ = tkzs;
    if ( slicepos_ )
    {
	slicepos_->setTrcKeyZSampling( tkzs );
	if ( datatransform_ )
	{
	    TrcKeyZSampling limitcs;
	    limitcs.zsamp_.setFrom( datatransform_->getZInterval(false) );
	    limitcs.zsamp_.step = datatransform_->getGoodZStep();
	    slicepos_->setLimitSampling( limitcs );
	}
    }

    if ( tkzs.isFlat() ) setWinTitle( false );
}


void uiODViewer2D::createViewWin( bool isvert, bool needslicepos )
{
    bool wantdock = false;
    Settings::common().getYN( "FlatView.Use Dockwin", wantdock );
    uiParent* controlparent = nullptr;
    if ( !wantdock )
    {
	auto* fvmw = new uiFlatViewMainWin( nullptr,
					uiFlatViewMainWin::Setup(basetxt_) );
	mAttachCB( fvmw->windowClosed, uiODViewer2D::winCloseCB );
	mAttachCB( appl_.windowClosed, uiODViewer2D::applClosed );
	if ( needslicepos )
	{
	    slicepos_ = new uiSlicePos2DView( fvmw, ZDomain::Info(zDomain()) );
	    slicepos_->setTrcKeyZSampling( tkzs_ );
	    mAttachCB( slicepos_->positionChg, uiODViewer2D::posChg );
	}

	viewwin_ = fvmw;
	createTree( fvmw );
    }
    else
    {
	auto* dwin = new uiFlatViewDockWin( &appl_,
				   uiFlatViewDockWin::Setup(basetxt_) );
	appl_.addDockWindow( *dwin, uiMainWin::Top );
	dwin->setFloating( true );
	viewwin_ = dwin;
	controlparent = &appl_;
    }

    viewwin_->setInitialSize( 700, 400 );
    if ( tkzs_.isFlat() ) setWinTitle( false );

    for ( int ivwr=0; ivwr<viewwin_->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewwin()->viewer( ivwr);
	vwr.setZAxisTransform( datatransform_ );
	vwr.appearance().setDarkBG( wantdock );
	vwr.appearance().setGeoDefaults(isvert);
	vwr.appearance().annot_.setAxesAnnot(true);
    }

    const float initialx2pospercm = isvert ? initialx2pospercm_
					   : initialx1pospercm_;
    uiFlatViewer& mainvwr = viewwin()->viewer();
    viewstdcontrol_ = new uiFlatViewStdControl( mainvwr,
	    uiFlatViewStdControl::Setup(controlparent).helpkey(
					mODHelpKey(mODViewer2DHelpID) )
					.withedit(tifs_).isvertical(isvert)
					.withfixedaspectratio(true)
					.withhomebutton(true)
					.initialx1pospercm(initialx1pospercm_)
					.initialx2pospercm(initialx2pospercm)
					.initialcentre(initialcentre_)
					.withscalebarbut(true)
					.managecoltab(!tifs_) );
    mAttachCB( mainvwr.dispPropChanged, uiODViewer2D::dispPropChangedCB );
    mAttachCB( viewstdcontrol_->infoChanged, uiODViewer2D::mouseMoveCB );
    if ( viewstdcontrol_->editPushed() )
	mAttachCB( viewstdcontrol_->editPushed(),
		   uiODViewer2D::itmSelectionChangedCB );
    if ( tifs_ && viewstdcontrol_->editToolBar() )
    {
	picksettingstbid_ = viewstdcontrol_->editToolBar()->addButton(
		    "seedpicksettings", tr("Tracking setup"),
		    mCB(this,uiODViewer2D,trackSetupCB), false );
	createPolygonSelBut( viewstdcontrol_->editToolBar() );
	createViewWinEditors();
    }

    viewwin_->addControl( viewstdcontrol_ );
    viewWinAvailable.trigger( this );
}


void uiODViewer2D::createTree( uiMainWin* mw )
{
    if ( !mw || !tifs_ ) return;

    uiDockWin* treedoc = new uiDockWin( mw, tr("Tree items") );
    treedoc->setMinimumWidth( 200 );
    uiTreeView* lv = new uiTreeView( treedoc, "Tree items" );
    treedoc->setObject( lv );
    uiStringSet labels;
    labels.add( uiODSceneMgr::sElements() );
    labels.add( uiStrings::sColor() );
    lv->addColumns( labels );
    lv->setFixedColumnWidth( uiODViewer2DMgr::cColorColumn(), 40 );

    treetp_ = new uiODView2DTreeTop( lv, &appl_.applMgr(), this, tifs_ );
    mAttachCB( treetp_->getTreeView()->selectionChanged,
	       uiODViewer2D::itmSelectionChangedCB );

    TypeSet<int> idxs;
    TypeSet<int> placeidxs;

    for ( int idx=0; idx<tifs_->nrFactories(); idx++ )
    {
	OD::Pol2D3D pol2d = tifs_->getPol2D3D( idx );
	if ( SI().survDataType() == OD::Both2DAnd3D
	     || pol2d == OD::Both2DAnd3D
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

    treetp_->setZAxisTransform( datatransform_ );
    lv->display( true );
    mw->addDockWindow( *treedoc, uiMainWin::Left );
    treedoc->display( true );
}


void uiODViewer2D::createPolygonSelBut( uiToolBar* tb )
{
    if ( !tb ) return;

    polyseltbid_ = tb->addButton( "polygonselect", tr("Polygon Selection mode"),
				  mCB(this,uiODViewer2D,selectionMode), true );
    uiMenu* polymnu = new uiMenu( tb, toUiString("PolyMenu") );

    uiAction* polyitm = new uiAction( uiStrings::sPolygon(),
				      mCB(this,uiODViewer2D,handleToolClick) );
    polymnu->insertAction( polyitm, 0 );
    polyitm->setIcon( "polygonselect" );

    uiAction* rectitm = new uiAction( uiStrings::sRectangle(),
				      mCB(this,uiODViewer2D,handleToolClick) );
    polymnu->insertAction( rectitm, 1 );
    rectitm->setIcon( "rectangleselect" );

    tb->setButtonMenu( polyseltbid_, polymnu );

    tb->addButton( "clearselection", tr("Remove Selection"),
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


void uiODViewer2D::dispPropChangedCB( CallBacker* )
{
    FlatView::Annotation& vwrannot = viewwin()->viewer().appearance().annot_;
    if ( vwrannot.dynamictitle_ )
	vwrannot.title_ = getInfoTitle().getFullString();
    viewwin()->viewer().handleChange( FlatView::Viewer::Annot );
}


void uiODViewer2D::winCloseCB( CallBacker* )
{
    deleteAndNullPtr( treetp_ );
    datamgr_->removeAll();

    deepErase( auxdataeditors_ );
    removeAvailablePacks();

    if ( viewwin_ )
	viewwin_->viewer(0).removeAuxData( marker_ );

    deleteAndNullPtr( marker_ );
    viewwin_ = nullptr;
    viewWinClosed.trigger();
}


void uiODViewer2D::applClosed( CallBacker* )
{
    mDynamicCastGet(uiFlatViewMainWin*,uimainviewwin,viewwin_);
    if ( uimainviewwin )
	uimainviewwin->close();
}


void uiODViewer2D::removeAvailablePacks()
{
    if ( !viewwin() ) { pErrMsg("No main window"); return; }

    for ( int ivwr=0; ivwr<viewwin()->nrViewers(); ivwr++ )
	viewwin()->viewer(ivwr).clearAllPacks();
}


void uiODViewer2D::setSelSpec( const Attrib::SelSpec* as, bool wva )
{
    const FlatView::Viewer::VwrDest dest = FlatView::Viewer::getDest( wva,
								      !wva );
    setSelSpec( as, dest );
}


void uiODViewer2D::setSelSpec( const Attrib::SelSpec* as,
			       FlatView::Viewer::VwrDest dest )
{
    const bool wva =	dest == FlatView::Viewer::WVA ||
			dest == FlatView::Viewer::Both;
    const bool vd =	dest == FlatView::Viewer::VD ||
			dest == FlatView::Viewer::Both;

    if ( as )
    {
	if ( wva )
	    wvaselspec_ = *as;
	if ( vd )
	    vdselspec_ = *as;
    }
    else
    {
	if ( wva )
	    initSelSpec( wvaselspec_ );
	if ( vd )
	    initSelSpec( vdselspec_ );
    }
}


void uiODViewer2D::posChg( CallBacker* )
{
    setPos( slicepos_->getTrcKeyZSampling() );
}


void uiODViewer2D::setPos( const TrcKeyZSampling& tkzs )
{
    if ( tkzs == tkzs_ ) return;
    uiTaskRunner taskr( viewerParent() );
    setTrcKeyZSampling( tkzs, &taskr );
    const uiFlatViewer& vwr = viewwin()->viewer(0);
    RefMan<FlatDataPack> fdp = createDataPackRM( false );
    FlatView::Viewer::VwrDest dest = FlatView::Viewer::VD;
    if ( vdselspec_==wvaselspec_ )
	dest = FlatView::Viewer::Both;
    else if ( vwr.isVisible(true) && wvaselspec_.id().isValid() )
    {
	fdp = createDataPackRM( true );
	dest = FlatView::Viewer::WVA;
    }

    makeUpView( fdp, dest );
    posChanged.trigger();
}


RefMan<SeisFlatDataPack> uiODViewer2D::getDataPack( bool wva ) const
{
    if ( wva && hp_wvadp_.getParam(this) )
	return hp_wvadp_.getParam( this );
    else if ( !wva && hp_vddp_.getParam(this) )
	return hp_vddp_.getParam( this );

    return createDataPackRM( wva );
}


RefMan<SeisFlatDataPack> uiODViewer2D::createDataPackRM( bool wva ) const
{
    return createDataPackRM( selSpec(wva) );
}


RefMan<SeisFlatDataPack> uiODViewer2D::createDataPackRM(
					const Attrib::SelSpec& selspec ) const
{
   TrcKeyZSampling tkzs = slicepos_ ? slicepos_->getTrcKeyZSampling() : tkzs_;
    if ( !tkzs.isFlat() ) return nullptr;

    RefMan<ZAxisTransform> zat = getZAxisTransform();
    if ( zat && !selspec.isZTransformed() )
    {
	if ( tkzs.nrZ() == 1 )
	    return createDataPackForTransformedZSliceRM( selspec );
	tkzs.zsamp_.setFrom( zat->getZInterval(true) );
	tkzs.zsamp_.step = SI().zStep();
    }

    uiAttribPartServer* attrserv = appl_.applMgr().attrServer();
    attrserv->setTargetSelSpec( selspec );
    auto dp = attrserv->createOutput( tkzs, nullptr );
    if ( !dp )
	return nullptr;

    return createFlatDataPackRM( *dp, 0 );
}


RefMan<SeisFlatDataPack> uiODViewer2D::createFlatDataPackRM( DataPackID dpid,
								int comp ) const
{
    const DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    ConstRefMan<SeisDataPack> seisdp = dpm.get<SeisDataPack>( dpid );
    if ( !seisdp )
	return nullptr;

    return createFlatDataPackRM( *seisdp, comp );
}


RefMan<SeisFlatDataPack> uiODViewer2D::createFlatDataPackRM(
					const SeisDataPack& dp, int comp ) const
{
    ConstRefMan<SeisDataPack> seisdp( &dp );
    if ( !seisdp->validComp(comp) )
	return nullptr;

    const StringView zdomainkey( seisdp->zDomain().key() );
    const bool alreadytransformed =
	!zdomainkey.isEmpty() && zdomainkey!=ZDomain::SI().key();
    if ( datatransform_ && !alreadytransformed )
    {
	SeisDataPackZAxisTransformer transformer( *datatransform_ );
	transformer.setInput( seisdp.ptr() );
	transformer.setInterpolate( true );
	transformer.execute();
	if ( transformer.getOutput() )
	    seisdp = transformer.getOutput();
    }

    mDynamicCastGet(const RegularSeisDataPack*,regsdp,seisdp.ptr());
    mDynamicCastGet(const RandomSeisDataPack*,randsdp,seisdp.ptr());
    RefMan<SeisFlatDataPack> seisfdp;
    if ( regsdp )
	seisfdp = new RegularFlatDataPack( *regsdp, comp );
    else if ( randsdp )
	seisfdp = new RandomFlatDataPack( *randsdp, comp );

    if ( !seisfdp )
	return nullptr;

    return seisfdp;
}


RefMan<SeisFlatDataPack> uiODViewer2D::createDataPackForTransformedZSliceRM(
					const Attrib::SelSpec& selspec ) const
{
    if ( !hasZAxisTransform() || selspec.isZTransformed() )
	return nullptr;

    const TrcKeyZSampling& tkzs = slicepos_ ? slicepos_->getTrcKeyZSampling()
					    : tkzs_;
    if ( tkzs.nrZ() != 1 ) return nullptr;

    uiAttribPartServer* attrserv = appl_.applMgr().attrServer();
    attrserv->setTargetSelSpec( selspec );

    RefMan<DataPointSet> data = new DataPointSet(false,true);
    ZAxisTransformPointGenerator generator( *datatransform_ );
    generator.setInput( tkzs );
    generator.setOutputDPS( *data );
    generator.execute();

    const int firstcol = data->nrCols();
    BufferStringSet userrefs; userrefs.add( selspec.userRef() );
    data->dataSet().add( new DataColDef(userrefs.get(0)) );
    if ( !attrserv->createOutput(*data,firstcol) )
	return nullptr;

    auto dp = RegularSeisDataPack::createDataPackForZSliceRM(
	    &data->bivSet(), tkzs, datatransform_->toZDomainInfo(), &userrefs );
    return createFlatDataPackRM( *dp, 0 );
}


RefMan<MapDataPack> uiODViewer2D::createMapDataPackRM(
					    const RegularFlatDataPack& rsdp )
{
    const TrcKeyZSampling& tkzs = rsdp.sampling();
    StepInterval<double> inlrg, crlrg;
    inlrg.setFrom( tkzs.hsamp_.inlRange() );
    crlrg.setFrom( tkzs.hsamp_.crlRange() );

    BufferStringSet dimnames;
    dimnames.add("X").add("Y").add(sKey::Inline()).add(sKey::Crossline());

    Array2DSlice<float> slice2d( rsdp.data() );
    slice2d.setDimMap( 0, 0 );
    slice2d.setDimMap( 1, 1 );
    slice2d.setPos( 2, 0 );
    slice2d.init();

    RefMan<MapDataPack> mdp =
	new MapDataPack( "ZSlice", new Array2DImpl<float>( slice2d ) );
    if ( !mdp )
	return nullptr;

    mdp->setName( rsdp.name() );
    mdp->setProps( inlrg, crlrg, true, &dimnames );
    return mdp;
}


DataPackID uiODViewer2D::getDataPackID( bool wva ) const
{
    const uiFlatViewer& vwr = viewwin()->viewer(0);
    if ( vwr.hasPack(wva) )
	return vwr.packID(wva);
    else if ( wvaselspec_ == vdselspec_ )
    {
	const DataPackID dpid = vwr.packID(!wva);
	if ( dpid != DataPack::cNoID() ) return dpid;
    }
    return createDataPack( wva );
}


DataPackID uiODViewer2D::createDataPack( const Attrib::SelSpec& selspec )const
{
    TrcKeyZSampling tkzs = slicepos_ ? slicepos_->getTrcKeyZSampling() : tkzs_;
    if ( !tkzs.isFlat() ) return DataPack::cNoID();

    RefMan<ZAxisTransform> zat = getZAxisTransform();
    if ( zat && !selspec.isZTransformed() )
    {
	if ( tkzs.nrZ() == 1 )
	    return createDataPackForTransformedZSlice( selspec );
	tkzs.zsamp_.setFrom( zat->getZInterval(true) );
	tkzs.zsamp_.step = SI().zStep();
    }

    uiAttribPartServer* attrserv = appl_.applMgr().attrServer();
    attrserv->setTargetSelSpec( selspec );
    const DataPackID dpid = attrserv->createOutput( tkzs, DataPack::cNoID() );

    auto dp = DPM(DataPackMgr::SeisID()).get<RegularSeisDataPack>( dpid );
    if ( !dp )
	return DataPack::cNoID();

    DataPackID fdpid = createFlatDataPack( *dp, 0 );
    DPM(DataPackMgr::SeisID()).unRef( dpid );
    return fdpid;
}


DataPackID uiODViewer2D::createFlatDataPack(
				DataPackID dpid, int comp ) const
{
    const DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    ConstRefMan<SeisDataPack> seisdp = dpm.get<SeisDataPack>( dpid );
    if ( !seisdp )
	return DataPack::cNoID();

    return createFlatDataPack( *seisdp, comp );
}


DataPackID uiODViewer2D::createFlatDataPack( const SeisDataPack& dp,
					     int comp ) const
{
    ConstRefMan<SeisDataPack> seisdp( &dp );
    if ( !seisdp->validComp(comp) )
	return DataPack::cNoID();

    const StringView zdomainkey( seisdp->zDomain().key() );
    const bool alreadytransformed =
	!zdomainkey.isEmpty() && zdomainkey!=ZDomain::SI().key();
    if ( datatransform_ && !alreadytransformed )
    {
	SeisDataPackZAxisTransformer transformer( *datatransform_ );
	transformer.setInput( seisdp.ptr() );
	transformer.setInterpolate( true );
	transformer.execute();
	if ( transformer.getOutput() )
            seisdp = transformer.getOutput();
    }

    mDynamicCastGet(const RegularSeisDataPack*,regsdp,seisdp.ptr());
    mDynamicCastGet(const RandomSeisDataPack*,randsdp,seisdp.ptr());
    RefMan<SeisFlatDataPack> seisfdp;
    if ( regsdp )
	seisfdp = new RegularFlatDataPack( *regsdp, comp );
    else if ( randsdp )
	seisfdp = new RandomFlatDataPack( *randsdp, comp );

    if ( !seisfdp || !DPM(DataPackMgr::FlatID()).add(seisfdp) )
	return DataPack::cNoID();

    seisfdp->ref();
    return seisfdp->id();
}


DataPackID uiODViewer2D::createDataPackForTransformedZSlice(
					const Attrib::SelSpec& selspec ) const
{
    if ( !hasZAxisTransform() || selspec.isZTransformed() )
	return DataPack::cNoID();

    const TrcKeyZSampling& tkzs = slicepos_ ? slicepos_->getTrcKeyZSampling()
					    : tkzs_;
    if ( tkzs.nrZ() != 1 ) return DataPack::cNoID();

    uiAttribPartServer* attrserv = appl_.applMgr().attrServer();
    attrserv->setTargetSelSpec( selspec );

    RefMan<DataPointSet> data = new DataPointSet(false,true);
    DPM(DataPackMgr::PointID()).add( data );

    ZAxisTransformPointGenerator generator( *datatransform_ );
    generator.setInput( tkzs );
    generator.setOutputDPS( *data );
    generator.execute();

    const int firstcol = data->nrCols();
    BufferStringSet userrefs; userrefs.add( selspec.userRef() );
    data->dataSet().add( new DataColDef(userrefs.get(0)) );
    if ( !attrserv->createOutput(*data,firstcol) )
	return DataPack::cNoID();

    const DataPackID dpid = RegularSeisDataPack::createDataPackForZSlice(
	  &data->bivSet(), tkzs, datatransform_->toZDomainInfo(), &userrefs );
    return createFlatDataPack( dpid, 0 );
}


DataPackID uiODViewer2D::createMapDataPack( const RegularFlatDataPack& rsdp )
{
    const TrcKeyZSampling& tkzs = rsdp.sampling();
    StepInterval<double> inlrg, crlrg;
    inlrg.setFrom( tkzs.hsamp_.inlRange() );
    crlrg.setFrom( tkzs.hsamp_.crlRange() );

    BufferStringSet dimnames;
    dimnames.add("X").add("Y").add(sKey::Inline()).add(sKey::Crossline());

    Array2DSlice<float> slice2d( rsdp.data() );
    slice2d.setDimMap( 0, 0 );
    slice2d.setDimMap( 1, 1 );
    slice2d.setPos( 2, 0 );
    slice2d.init();

    RefMan<MapDataPack> mdp =
	new MapDataPack( "ZSlice", new Array2DImpl<float>( slice2d ) );
    if ( !mdp || !DPM(DataPackMgr::FlatID()).add(mdp) )
	return DataPack::cNoID();

    mdp->setName( rsdp.name() );
    mdp->setProps( inlrg, crlrg, true, &dimnames );
    mdp->ref();
    return mdp->id();
}


bool uiODViewer2D::useStoredDispPars( bool wva )
{
    const FlatView::Viewer::VwrDest dest = FlatView::Viewer::getDest( wva,
								      !wva );
    return useStoredDispPars( dest );
}


bool getMapperSetup( uiODMain& appl, const Attrib::SelSpec& selspec,
		     ColTab::MapperSetup& mapper, BufferString& ctab_name )
{
    PtrMan<IOObj> ioobj = appl.applMgr().attrServer()->getIOObj(selspec);
    if ( !ioobj ) return false;

    SeisIOObjInfo seisobj( ioobj );
    IOPar iop;
    if ( !seisobj.getDisplayPars(iop) )
	return false;

    if ( !mapper.usePar(iop) )
	return false;

    ctab_name = iop.find( sKey::Name() );

    return true;
}


bool uiODViewer2D::useStoredDispPars( FlatView::Viewer::VwrDest dest )
{
    const bool wva =	dest == FlatView::Viewer::WVA ||
			dest == FlatView::Viewer::Both;
    const bool vd =	dest == FlatView::Viewer::VD ||
			dest == FlatView::Viewer::Both;

    ColTab::MapperSetup mapper_wva;
    ColTab::MapperSetup mapper_vd;
    BufferString ctabnm_wva, ctabnm_vd;
    if ( (wva && !getMapperSetup(appl_, selSpec(true), mapper_wva, ctabnm_wva))
     || (vd && !getMapperSetup(appl_, selSpec(false), mapper_vd, ctabnm_vd)) )
	return false;

    for ( int ivwr=0; ivwr<viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewwin()->viewer( ivwr );
	FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;
	if ( wva )
	    ddp.wva_.mappersetup_ = mapper_wva;

	if ( vd )
	{
	    ddp.vd_.mappersetup_ = mapper_vd;
	    ddp.vd_.ctab_ = ctabnm_vd;
	}
    }

    return true;
}


void uiODViewer2D::itmSelectionChangedCB( CallBacker* )
{
    const uiTreeViewItem* curitem =
	treetp_ ? treetp_->getTreeView()->selectedItem() : 0;
    if ( !curitem )
    {
	if ( viewstdcontrol_->editToolBar() )
	    viewstdcontrol_->editToolBar()->setSensitive( picksettingstbid_,
							  false );
	return;
    }

    BufferString seltxt( curitem->text() );
    ObjectSet<uiTreeItem> treeitms;
    treetp_->findChildren( seltxt, treeitms );
    uiODView2DHor2DTreeItem* hor2dtreeitm = 0;
    uiODView2DHor3DTreeItem* hor3dtreeitm = 0;
    for ( int idx=0; idx<treeitms.size(); idx++ )
    {
	mDynamicCast(uiODView2DHor2DTreeItem*,hor2dtreeitm,treeitms[idx])
	mDynamicCast(uiODView2DHor3DTreeItem*,hor3dtreeitm,treeitms[idx])
	if ( hor2dtreeitm || hor3dtreeitm )
	    break;
    }

    if ( !hor2dtreeitm && !hor3dtreeitm )
    {
	if ( viewstdcontrol_->editToolBar() )
	    viewstdcontrol_->editToolBar()->setSensitive(
		    picksettingstbid_, false );
	return;
    }

    uiMPEPartServer* mpserv = appl_.applMgr().mpeServer();
    const EM::ObjectID emobjid = hor2dtreeitm ? hor2dtreeitm->emObjectID()
					      : hor3dtreeitm->emObjectID();
    const int trackerid = mpserv->getTrackerID( emobjid );
    if ( viewstdcontrol_->editToolBar() )
	viewstdcontrol_->editToolBar()->setSensitive(
		picksettingstbid_, trackerid==mpserv->activeTrackerID() );
}


void uiODViewer2D::trackSetupCB( CallBacker* )
{
    const uiTreeViewItem* curitem =
	treetp_ ? treetp_->getTreeView()->selectedItem() : 0;
    if ( !curitem )
	return;

    BufferString seltxt( curitem->text() );
    ObjectSet<uiTreeItem> treeitms;
    treetp_->findChildren( seltxt, treeitms );
    uiODView2DHor3DTreeItem* hortreeitm = 0;
    uiODView2DHor2DTreeItem* hor2dtreeitm = 0;
    for ( int idx=0; idx<treeitms.size(); idx++ )
    {
	mDynamicCast( uiODView2DHor3DTreeItem*,hortreeitm,treeitms[idx])
	mDynamicCast( uiODView2DHor2DTreeItem*,hor2dtreeitm,treeitms[idx])
	if ( hortreeitm || hor2dtreeitm )
	    break;
    }

    if ( !hortreeitm && !hor2dtreeitm )
	return;

    const EM::ObjectID emid = hortreeitm ? hortreeitm->emObjectID()
					 : hor2dtreeitm->emObjectID();
    EM::EMObject* emobj = EM::EMM().getObject( emid );
    if ( !emobj )
	return;

    appl_.applMgr().mpeServer()->showSetupDlg( emobj->id() );
}


void uiODViewer2D::selectionMode( CallBacker* cb )
{
    if ( !viewstdcontrol_ || !viewstdcontrol_->editToolBar() )
	return;

    viewstdcontrol_->editToolBar()->setIcon( polyseltbid_, ispolyselect_ ?
				"polygonselect" : "rectangleselect" );
    viewstdcontrol_->editToolBar()->setToolTip( polyseltbid_,
				ispolyselect_ ? tr("Polygon Selection mode")
					      : tr("Rectangle Selection mode"));
    const bool ispolyseltbon =
	viewstdcontrol_->editToolBar()->isOn( polyseltbid_ );
    if ( ispolyseltbon )
	viewstdcontrol_->setEditMode( true );

    for ( int edidx=0; edidx<auxdataeditors_.size(); edidx++ )
    {
	auxdataeditors_[edidx]->setSelectionPolygonRectangle( !ispolyselect_ );
	auxdataeditors_[edidx]->setSelActive( ispolyseltbon );
    }
}


void uiODViewer2D::handleToolClick( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,itm,cb)
    if ( !itm ) return;

    ispolyselect_ = itm->getID()==0;
    selectionMode( cb );
}


void uiODViewer2D::removeSelected( CallBacker* )
{
    if ( !viewstdcontrol_->editToolBar() ||
	 !viewstdcontrol_->editToolBar()->isOn(polyseltbid_) )
	return;

    for ( int edidx=0; edidx<auxdataeditors_.size(); edidx++ )
	auxdataeditors_[edidx]->removePolygonSelected( -1 );
}


uiString uiODViewer2D::getInfoTitle() const
{
    uiString info = toUiString("%1: %2");
    if ( rdmlineid_.isValid() )
    {
	const Geometry::RandomLine* rdmline =
			Geometry::RLM().get( rdmlineid_ );
	if ( rdmline )
	    info = toUiString( rdmline->name() );
    }
    else if ( tkzs_.is2D() )
    {
	info.arg( tr("Line") )
	    .arg( toUiString( Survey::GM().getName(geomID()) ) );
    }
    else if ( tkzs_.defaultDir() == TrcKeyZSampling::Inl )
    {
	info.arg( uiStrings::sInline() )
	    .arg( tkzs_.hsamp_.start_.inl() );
    }
    else if ( tkzs_.defaultDir() == TrcKeyZSampling::Crl )
    {
	info.arg( uiStrings::sCrossline() )
	    .arg( tkzs_.hsamp_.start_.crl() );
    }
    else
    {
	info.arg( zDomain().userName() )
	    .arg( mNINT32(tkzs_.zsamp_.start * zDomain().userFactor()) );
    }

    return info;
}


void uiODViewer2D::setWinTitle( bool fromvisobjinfo )
{
    uiString info;
    if ( !fromvisobjinfo )
	info = getInfoTitle();
    else
    {
	BufferString objectinfo;
	appl_.applMgr().visServer()->getObjectInfo( visid_, objectinfo );
	if ( objectinfo.isEmpty() )
	    info = appl_.applMgr().visServer()->getUiObjectName( visid_ );
	else
	    info = toUiString( objectinfo );
    }

    uiString title = toUiString("%1%2").arg( mToUiStringTodo(basetxt_) )
				       .arg( info );
    if ( !viewwin() )
	return;

    viewwin()->setWinTitle( title );

    FlatView::Annotation& vwrannot = viewwin()->viewer().appearance().annot_;
    if ( vwrannot.dynamictitle_ )
	vwrannot.title_ = info.getFullString();
}


void uiODViewer2D::usePar( const IOPar& iop )
{
    if ( !viewwin() ) return;

    IOPar* vdselspecpar = iop.subselect( sKeyVDSelSpec() );
    if ( vdselspecpar ) vdselspec_.usePar( *vdselspecpar );
    IOPar* wvaselspecpar = iop.subselect( sKeyWVASelSpec() );
    if ( wvaselspecpar ) wvaselspec_.usePar( *wvaselspecpar );
    delete vdselspecpar; delete wvaselspecpar;
    IOPar* tkzspar = iop.subselect( sKeyPos() );
    TrcKeyZSampling tkzs; if ( tkzspar ) tkzs.usePar( *tkzspar );
    if ( viewwin()->nrViewers() > 0 )
    {
	const uiFlatViewer& vwr = viewwin()->viewer(0);
	const bool iswva = wvaselspec_.id().isValid();
	ConstRefMan<RegularSeisDataPack> regsdp = vwr.getPack( iswva ).get();
	if ( regsdp ) setPos( tkzs );
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
    IOPar pospar; tkzs_.fillPar( pospar );
    iop.mergeComp( pospar, sKeyPos() );

    datamgr_->fillPar( iop );
}


void uiODViewer2D::rebuildTree()
{
    if ( !treetp_ )
	return;

    ObjectSet<View2D::DataObject> objs;
    getObjects( objs );
    for ( int iobj=0; iobj<objs.size(); iobj++ )
    {
	const uiODView2DTreeItem* childitem =
	    treetp_->getView2DItem( objs[iobj]->id() );
	if ( !childitem )
	    uiODView2DTreeItem::create( treeTop(), *this, objs[iobj]->id() );
    }
}


void uiODViewer2D::setMouseCursorExchange( MouseCursorExchange* mce )
{
    if ( mousecursorexchange_ )
	mDetachCB( mousecursorexchange_->notifier,uiODViewer2D::mouseCursorCB );

    mousecursorexchange_ = mce;
    if ( mousecursorexchange_ )
	mAttachCB( mousecursorexchange_->notifier,uiODViewer2D::mouseCursorCB );
}


void uiODViewer2D::mouseCursorCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(const MouseCursorExchange::Info&,info,
			       caller,cb);
    if ( caller==this )
	return;

    if ( !viewwin() ) return;

    uiFlatViewer& vwr = viewwin()->viewer(0);
    if ( !marker_ )
    {
	marker_ = vwr.createAuxData( "XYZ Marker" );
	vwr.addAuxData( marker_ );
	marker_->poly_ += FlatView::Point(0,0);
	marker_->markerstyles_ += MarkerStyle2D();
    }

    ConstRefMan<FlatDataPack> fdp = vwr.getPack( false, true ).get();
    mDynamicCastGet(const SeisFlatDataPack*,seisfdp,fdp.ptr());
    mDynamicCastGet(const MapDataPack*,mapdp,fdp.ptr());
    if ( !seisfdp && !mapdp ) return;

    const TrcKeyValue& trkv = info.trkv_;
    FlatView::Point& pt = marker_->poly_[0];
    if ( seisfdp )
    {
	const int gidx = seisfdp->getSourceGlobalIdx( trkv.tk_ );
	if ( seisfdp->isVertical() )
	{
	    pt.x = fdp->posData().range(true).atIndex( gidx );
	    pt.y = datatransform_ ?
		   datatransform_->transformTrc( trkv.tk_, trkv.val_ ) :
		   trkv.val_;
	}
	else
	{
	    pt.x = fdp->posData().range(true).atIndex( gidx / tkzs_.nrTrcs() );
	    pt.y = fdp->posData().range(false).atIndex( gidx % tkzs_.nrTrcs() );
	}
    }
    else if ( mapdp )
    {
	const Coord pos = Survey::GM().toCoord( trkv.tk_ );
	pt = FlatView::Point( pos.x, pos.y );
    }

    vwr.handleChange( FlatView::Viewer::Auxdata );
}


void uiODViewer2D::mouseMoveCB( CallBacker* cb )
{
    Coord3 mousepos( Coord3::udf() );
    mCBCapsuleUnpack(IOPar,pars,cb);

    BufferString valstr = pars.find( "X" );
    if ( valstr.isEmpty() )
	valstr = pars.find( "X-coordinate" );

    if ( !valstr.isEmpty() )
	mousepos.x = valstr.toDouble();

    valstr = pars.find( "Y" );
    if ( valstr.isEmpty() )
	valstr = pars.find( "Y-coordinate" );

    if ( !valstr.isEmpty() )
	mousepos.y = valstr.toDouble();

    valstr = pars.find( "Z" );
    if ( valstr.isEmpty() )
	valstr = pars.find( "Z-Coord" );

    if ( !valstr.isEmpty() )
    {
	mousepos.z = valstr.toFloat() / zDomain().userFactor();
	if ( datatransform_ )
	    mousepos.z = datatransform_->transformBack( mousepos );
    }

    if ( mousecursorexchange_ )
    {
	const TrcKeyValue trckeyval =
	    mousepos.isDefined() ? TrcKeyValue(
		    TrcKey( SI().transform(mousepos.coord()) ),
						mCast(float,mousepos.z))
				 : TrcKeyValue::udf();

	MouseCursorExchange::Info info( trckeyval );
	mousecursorexchange_->notifier.trigger( info, this );
    }
}

bool uiODViewer2D::isItemPresent( const uiTreeItem* item ) const
{
    for ( int ip=0; ip<treetp_->nrChildren(); ip++ )
    {
	const uiTreeItem* parentitm = treetp_->getChild( ip );
	if ( parentitm == item )
	    return true;
	for ( int ich=0; ich<parentitm->nrChildren(); ich++ )
	{
	    const uiTreeItem* childitm = parentitm->getChild( ich );
	    if ( childitm == item )
		return true;
	}
    }

    return false;
}


void uiODViewer2D::getVwr2DObjIDs( TypeSet<Vis2DID>& vw2dobjids ) const
{
    TypeSet<Vis2DID> vw2dids;
    datamgr_->getObjectIDs( vw2dids );
    vw2dobjids.append( vw2dids );
}


void uiODViewer2D::getHor3DVwr2DIDs( EM::ObjectID emid,
				     TypeSet<Vis2DID>& vw2dobjids ) const
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DHor3DParentTreeItem*,hor3dpitem,
			treetp_->getChild(idx))
	if ( hor3dpitem )
	    hor3dpitem->getHor3DVwr2DIDs( emid, vw2dobjids );
    }
}


void uiODViewer2D::removeHorizon3D( EM::ObjectID emid )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DHor3DParentTreeItem*,hor3dpitem,
			treetp_->getChild(idx))
	if ( hor3dpitem )
	    hor3dpitem->removeHorizon3D( emid );
    }
}


void uiODViewer2D::getLoadedHorizon3Ds( TypeSet<EM::ObjectID>& emids ) const
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DHor3DParentTreeItem*,hor3dpitem,
			treetp_->getChild(idx))
	if ( hor3dpitem )
	    hor3dpitem->getLoadedHorizon3Ds( emids );
    }
}


void uiODViewer2D::addHorizon3Ds( const TypeSet<EM::ObjectID>& emids )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DHor3DParentTreeItem*,hor3dpitem,
			treetp_->getChild(idx))
	if ( hor3dpitem )
	    hor3dpitem->addHorizon3Ds( emids );
    }
}


void uiODViewer2D::setupTrackingHorizon3D( EM::ObjectID emid )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DHor3DParentTreeItem*,hor3dpitem,
			treetp_->getChild(idx))
	if ( hor3dpitem )
	{
	    hor3dpitem->setupTrackingHorizon3D( emid );
	    if ( viewstdcontrol_->editToolBar() )
		viewstdcontrol_->editToolBar()->setSensitive(
			picksettingstbid_, true );
	}
    }
}


void uiODViewer2D::addNewTrackingHorizon3D( EM::ObjectID emid )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DHor3DParentTreeItem*,hor3dpitem,
			treetp_->getChild(idx))
	if ( hor3dpitem )
	{
	    hor3dpitem->addNewTrackingHorizon3D( emid );
	    if ( viewstdcontrol_->editToolBar() )
		viewstdcontrol_->editToolBar()->setSensitive(
			picksettingstbid_, true );
	}
    }
}


void uiODViewer2D::getHor2DVwr2DIDs( EM::ObjectID emid,
				     TypeSet<Vis2DID>& vw2dobjids ) const
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DHor2DParentTreeItem*,hor3dpitem,
			treetp_->getChild(idx))
	if ( hor3dpitem )
	    hor3dpitem->getHor2DVwr2DIDs( emid, vw2dobjids );
    }
}


void uiODViewer2D::removeHorizon2D( EM::ObjectID emid )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DHor2DParentTreeItem*,hor2dpitem,
			treetp_->getChild(idx))
	if ( hor2dpitem )
	    hor2dpitem->removeHorizon2D( emid );
    }
}


void uiODViewer2D::getLoadedHorizon2Ds( TypeSet<EM::ObjectID>& emids ) const
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DHor2DParentTreeItem*,hor2dpitem,
			treetp_->getChild(idx))
	if ( hor2dpitem )
	    hor2dpitem->getLoadedHorizon2Ds( emids );
    }
}


void uiODViewer2D::addHorizon2Ds( const TypeSet<EM::ObjectID>& emids )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DHor2DParentTreeItem*,hor2dpitem,
			treetp_->getChild(idx))
	if ( hor2dpitem )
	    hor2dpitem->addHorizon2Ds( emids );
    }
}


void uiODViewer2D::setupTrackingHorizon2D( EM::ObjectID emid )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DHor2DParentTreeItem*,hor2dpitem,
			treetp_->getChild(idx))
	if ( hor2dpitem )
	{
	    hor2dpitem->setupTrackingHorizon2D( emid );
	    if ( viewstdcontrol_->editToolBar() )
		viewstdcontrol_->editToolBar()->setSensitive(
			 picksettingstbid_, true );
	}
    }
}


void uiODViewer2D::addNewTrackingHorizon2D( EM::ObjectID emid )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DHor2DParentTreeItem*,hor2dpitem,
			treetp_->getChild(idx))
	if ( hor2dpitem )
	    hor2dpitem->addNewTrackingHorizon2D( emid );
    }
}


void uiODViewer2D::getFaultVwr2DIDs( EM::ObjectID emid,
				     TypeSet<Vis2DID>& vw2dobjids ) const
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DFaultParentTreeItem*,faultpitem,
			treetp_->getChild(idx))
	if ( faultpitem )
	    faultpitem->getFaultVwr2DIDs( emid, vw2dobjids );
    }
}


void uiODViewer2D::removeFault( EM::ObjectID emid )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DFaultParentTreeItem*,faultpitem,
			treetp_->getChild(idx))
	if ( faultpitem )
	    faultpitem->removeFault( emid );
    }
}


void uiODViewer2D::getLoadedFaults( TypeSet<EM::ObjectID>& emids ) const
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DFaultParentTreeItem*,faultpitem,
			treetp_->getChild(idx))
	if ( faultpitem )
	    faultpitem->getLoadedFaults( emids );
    }
}


void uiODViewer2D::addFaults( const TypeSet<EM::ObjectID>& emids )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DFaultParentTreeItem*,faultpitem,
			treetp_->getChild(idx))
	if ( faultpitem )
	    faultpitem->addFaults( emids );
    }
}


void uiODViewer2D::setupNewTempFault( EM::ObjectID emid )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DFaultParentTreeItem*,faultpitem,
			treetp_->getChild(idx))
	if ( faultpitem )
	{
	    faultpitem->setupNewTempFault( emid );
	    if ( viewstdcontrol_->editToolBar() )
		viewstdcontrol_->editToolBar()->setSensitive(
			 picksettingstbid_, false );
	}
    }
}


void uiODViewer2D::addNewTempFault( EM::ObjectID emid )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DFaultParentTreeItem*,faultpitem,
			treetp_->getChild(idx))
	if ( faultpitem )
	    faultpitem->addNewTempFault( emid );
    }
}


void uiODViewer2D::getFaultSSVwr2DIDs( EM::ObjectID emid,
				     TypeSet<Vis2DID>& vw2dobjids ) const
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DFaultSSParentTreeItem*,faultsspitem,
			treetp_->getChild(idx))
	if ( faultsspitem )
	    faultsspitem->getFaultSSVwr2DIDs( emid, vw2dobjids );
    }
}


void uiODViewer2D::removeFaultSS( EM::ObjectID emid )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DFaultSSParentTreeItem*,faultpitem,
			treetp_->getChild(idx))
	if ( faultpitem )
	    faultpitem->removeFaultSS( emid );
    }
}


void uiODViewer2D::getLoadedFaultSSs( TypeSet<EM::ObjectID>& emids ) const
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DFaultSSParentTreeItem*,faultpitem,
			treetp_->getChild(idx))
	if ( faultpitem )
	    faultpitem->getLoadedFaultSSs( emids );
    }
}


void uiODViewer2D::addFaultSSs( const TypeSet<EM::ObjectID>& emids )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DFaultSSParentTreeItem*,faultpitem,
			treetp_->getChild(idx))
	if ( faultpitem )
	    faultpitem->addFaultSSs( emids );
    }
}


void uiODViewer2D::setupNewTempFaultSS( EM::ObjectID emid )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DFaultSSParentTreeItem*,fltsspitem,
			treetp_->getChild(idx))
	if ( fltsspitem )
	{
	    fltsspitem->setupNewTempFaultSS( emid );
	    if ( viewstdcontrol_->editToolBar() )
		viewstdcontrol_->editToolBar()->setSensitive(
			 picksettingstbid_, false );
	}
    }
}



void uiODViewer2D::addNewTempFaultSS( EM::ObjectID emid )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DFaultSSParentTreeItem*,faultpitem,
			treetp_->getChild(idx))
	if ( faultpitem )
	    faultpitem->addNewTempFaultSS( emid );
    }
}


void uiODViewer2D::getFaultSS2DVwr2DIDs( EM::ObjectID emid,
				     TypeSet<Vis2DID>& vw2dobjids ) const
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DFaultSS2DParentTreeItem*,faultsspitem,
			treetp_->getChild(idx))
	if ( faultsspitem )
	    faultsspitem->getFaultSS2DVwr2DIDs( emid, vw2dobjids );
    }
}


void uiODViewer2D::removeFaultSS2D( EM::ObjectID emid )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DFaultSS2DParentTreeItem*,faultpitem,
			treetp_->getChild(idx))
	if ( faultpitem )
	    faultpitem->removeFaultSS2D( emid );
    }
}


void uiODViewer2D::getLoadedFaultSS2Ds( TypeSet<EM::ObjectID>& emids ) const
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DFaultSS2DParentTreeItem*,faultpitem,
			treetp_->getChild(idx))
	if ( faultpitem )
	    faultpitem->getLoadedFaultSS2Ds( emids );
    }
}


void uiODViewer2D::addFaultSS2Ds( const TypeSet<EM::ObjectID>& emids )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DFaultSS2DParentTreeItem*,faultpitem,
			treetp_->getChild(idx))
	if ( faultpitem )
	    faultpitem->addFaultSS2Ds( emids );
    }
}


void uiODViewer2D::setupNewTempFaultSS2D( EM::ObjectID emid )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DFaultSS2DParentTreeItem*,fltsspitem,
			treetp_->getChild(idx))
	if ( fltsspitem )
	{
	    fltsspitem->setupNewTempFaultSS2D( emid );
	    if ( viewstdcontrol_->editToolBar() )
		viewstdcontrol_->editToolBar()->setSensitive(
			 picksettingstbid_, false );
	}
    }
}


void uiODViewer2D::addNewTempFaultSS2D( EM::ObjectID emid )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DFaultSS2DParentTreeItem*,faultpitem,
			treetp_->getChild(idx))
	if ( faultpitem )
	    faultpitem->addNewTempFaultSS2D( emid );
    }
}



void uiODViewer2D::getPickSetVwr2DIDs( const MultiID& mid,
				       TypeSet<Vis2DID>& vw2dobjids ) const
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DPickSetParentTreeItem*,pickpitem,
			treetp_->getChild(idx))
	if ( pickpitem )
	    pickpitem->getPickSetVwr2DIDs( mid, vw2dobjids );
    }
}


void uiODViewer2D::removePickSet( const MultiID& mid )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DPickSetParentTreeItem*,pickitem,
			treetp_->getChild(idx))
	if ( pickitem )
	    pickitem->removePickSet( mid );
    }
}


void uiODViewer2D::getLoadedPickSets( TypeSet<MultiID>& mids ) const
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DPickSetParentTreeItem*,pickitem,
			treetp_->getChild(idx))
	if ( pickitem )
	    pickitem->getLoadedPickSets( mids );
    }
}


void uiODViewer2D::addPickSets( const TypeSet<MultiID>& mids )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DPickSetParentTreeItem*,pickitem,
			treetp_->getChild(idx))
	if ( pickitem )
	    pickitem->addPickSets( mids );
    }
}


void uiODViewer2D::setupNewPickSet( const MultiID& pickid )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODView2DPickSetParentTreeItem*,pickpitem,
			treetp_->getChild(idx))
	if ( pickpitem )
	{
	    pickpitem->setupNewPickSet( pickid );
	    if ( viewstdcontrol_->editToolBar() )
		viewstdcontrol_->editToolBar()->setSensitive(
			 picksettingstbid_, false );
	}
    }
}
