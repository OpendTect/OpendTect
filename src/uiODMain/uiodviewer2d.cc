/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2002
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

static void initSelSpec( Attrib::SelSpec& as )
{ as.set( 0, Attrib::SelSpec::cNoAttrib(), false, 0 ); }

mDefineInstanceCreatedNotifierAccess( uiODViewer2D )

uiODViewer2D::uiODViewer2D( uiODMain& appl, int visid )
    : appl_(appl)
    , visid_(visid)
    , vdselspec_(*new Attrib::SelSpec)
    , wvaselspec_(*new Attrib::SelSpec)
    , viewwin_(nullptr)
    , slicepos_(nullptr)
    , viewstdcontrol_(nullptr)
    , datamgr_(new Vw2DDataManager)
    , tifs_(0)
    , treetp_(0)
    , polyseltbid_(-1)
    , rdmlineid_(mUdf(int))
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
    id_ = vwrid++;

    setWinTitle( true );

    if ( visid_>=0 )
	syncsceneid_ = appl_.applMgr().visServer()->getSceneID( visid_ );
    else
    {
	TypeSet<int> sceneids;
	appl_.applMgr().visServer()->getSceneIds( sceneids );
	for ( int iscn=0; iscn<sceneids.size(); iscn++ )
	{
	    const int sceneid = sceneids[iscn];
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
}


int uiODViewer2D::getSyncSceneID() const
{
    return syncsceneid_;
}


Pos::GeomID uiODViewer2D::geomID() const
{
    if ( tkzs_.hsamp_.survid_ == Survey::GM().get2DSurvID() )
	return tkzs_.hsamp_.trcKeyAt(0).geomID();

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
	    vwrannot.x1_.auxlabel_.arg( tr("2D Line" ));
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


void uiODViewer2D::setUpView( DataPack::ID packid, bool wva )
{
    DataPackMgr& dpm = DPM(DataPackMgr::FlatID());
    ConstDataPackRef<FlatDataPack> fdp = dpm.obtain( packid );
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
	packid = createMapDataPack( *regfdp );
	viewwin()->viewer().appearance().annot_.x2_.reversed_ = false;
    }

    setDataPack( packid, wva, isnew ); adjustOthrDisp( wva, isnew );

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


void uiODViewer2D::adjustOthrDisp( bool wva, bool isnew )
{
    if ( !slicepos_ ) return;
    const TrcKeyZSampling& cs = slicepos_->getTrcKeyZSampling();
    const bool newcs = ( cs != tkzs_ );
    const DataPack::ID othrdpid = newcs ? createDataPack(!wva)
					: getDataPackID(!wva);
    if ( newcs && (othrdpid != DataPack::cNoID()) )
    { removeAvailablePacks(); setTrcKeyZSampling( cs ); }
    setDataPack( othrdpid, !wva, isnew );
}


void uiODViewer2D::setDataPack( DataPack::ID packid, bool wva, bool isnew )
{
    if ( packid == DataPack::cNoID() ) return;

    DataPackMgr& dpm = DPM(DataPackMgr::FlatID());
    for ( int ivwr=0; ivwr<viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewwin()->viewer(ivwr);
	const TypeSet<DataPack::ID> ids = vwr.availablePacks();
	if ( ids.isPresent(packid) )
	{ vwr.usePack( wva, packid, isnew ); continue; }

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
		break;
	    }
	}

	vwr.setPack( wva, packid, isnew );
	if ( setforotherdisp || (isnew && wvaselspec_==vdselspec_) )
	    vwr.usePack( !wva, packid, isnew );
    }

    dataChanged.trigger( this );
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

    treetp_ = new uiODVw2DTreeTop( lv, &appl_.applMgr(), this, tifs_ );
    mAttachCB( treetp_->getTreeView()->selectionChanged,
	       uiODViewer2D::itmSelectionChangedCB );

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
    deleteAndZeroPtr( treetp_ );
    datamgr_->removeAll();

    deepErase( auxdataeditors_ );
    removeAvailablePacks();

    if ( viewwin_ )
	viewwin_->viewer(0).removeAuxData( marker_ );

    deleteAndZeroPtr( marker_ );
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
    if ( as )
	(wva ? wvaselspec_ : vdselspec_) = *as;
    else
	initSelSpec( wva ? wvaselspec_ : vdselspec_ );
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
    if ( vwr.isVisible(false) && vdselspec_.id().isValid() )
	setUpView( createDataPack(false), false );
    else if ( vwr.isVisible(true) && wvaselspec_.id().isValid() )
	setUpView( createDataPack(true), true );
    posChanged.trigger();
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


DataPack::ID uiODViewer2D::createDataPack( const Attrib::SelSpec& selspec )const
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
    const DataPack::ID dpid = attrserv->createOutput( tkzs, DataPack::cNoID() );
    return createFlatDataPack( dpid, 0 );
}


DataPack::ID uiODViewer2D::createFlatDataPack(
				DataPack::ID dpid, int comp ) const
{
    DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    ConstDataPackRef<SeisDataPack> seisdp = dpm.obtain( dpid );
    if ( !seisdp || !(comp<seisdp->nrComponents()) ) return dpid;

    const FixedString zdomainkey( seisdp->zDomain().key() );
    const bool alreadytransformed =
	!zdomainkey.isEmpty() && zdomainkey!=ZDomain::SI().key();
    if ( datatransform_ && !alreadytransformed )
    {
	DataPack::ID outputid = DataPack::cNoID();
	SeisDataPackZAxisTransformer transformer( *datatransform_ );
	transformer.setInput( seisdp.ptr() );
	transformer.setOutput( outputid );
	transformer.setInterpolate( true );
	transformer.execute();
	if ( outputid != DataPack::cNoID() )
	    seisdp = dpm.obtain( outputid );
    }

    mDynamicCastGet(const RegularSeisDataPack*,regsdp,seisdp.ptr());
    mDynamicCastGet(const RandomSeisDataPack*,randsdp,seisdp.ptr());
    SeisFlatDataPack* seisfdp = 0;
    if ( regsdp )
	seisfdp = new RegularFlatDataPack( *regsdp, comp );
    else if ( randsdp )
	seisfdp = new RandomFlatDataPack( *randsdp, comp );
    DPM(DataPackMgr::FlatID()).add( seisfdp );
    return seisfdp ? seisfdp->id() : DataPack::cNoID();
}


DataPack::ID uiODViewer2D::createDataPackForTransformedZSlice(
					const Attrib::SelSpec& selspec ) const
{
    if ( !hasZAxisTransform() || selspec.isZTransformed() )
	return DataPack::cNoID();

    const TrcKeyZSampling& tkzs = slicepos_ ? slicepos_->getTrcKeyZSampling()
					    : tkzs_;
    if ( tkzs.nrZ() != 1 ) return DataPack::cNoID();

    uiAttribPartServer* attrserv = appl_.applMgr().attrServer();
    attrserv->setTargetSelSpec( selspec );

    DataPackRef<DataPointSet> data =
	DPM(DataPackMgr::PointID()).addAndObtain(new DataPointSet(false,true));

    ZAxisTransformPointGenerator generator( *datatransform_ );
    generator.setInput( tkzs );
    generator.setOutputDPS( *data );
    generator.execute();

    const int firstcol = data->nrCols();
    BufferStringSet userrefs; userrefs.add( selspec.userRef() );
    data->dataSet().add( new DataColDef(userrefs.get(0)) );
    if ( !attrserv->createOutput(*data,firstcol) )
	return DataPack::cNoID();

    const DataPack::ID dpid = RegularSeisDataPack::createDataPackForZSlice(
	    &data->bivSet(), tkzs, datatransform_->toZDomainInfo(), &userrefs );
    return createFlatDataPack( dpid, 0 );
}


DataPack::ID uiODViewer2D::createMapDataPack( const RegularFlatDataPack& rsdp )
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

    MapDataPack* mdp =
	new MapDataPack( "ZSlice", new Array2DImpl<float>( slice2d ) );
    mdp->setProps( inlrg, crlrg, true, &dimnames );
    DPM(DataPackMgr::FlatID()).addAndObtain( mdp );
    DPM(DataPackMgr::FlatID()).add( mdp );
    return mdp->id();
}


bool uiODViewer2D::useStoredDispPars( bool wva )
{
    PtrMan<IOObj> ioobj = appl_.applMgr().attrServer()->getIOObj(selSpec(wva));
    if ( !ioobj ) return false;

    SeisIOObjInfo seisobj( ioobj );
    IOPar iop;
    if ( !seisobj.getDisplayPars(iop) )
	return false;

    ColTab::MapperSetup mapper;
    if ( !mapper.usePar(iop) )
	return false;

    for ( int ivwr=0; ivwr<viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewwin()->viewer( ivwr );
	FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;
	wva ? ddp.wva_.mappersetup_ : ddp.vd_.mappersetup_ = mapper;
	if ( !wva ) ddp.vd_.ctab_ = iop.find( sKey::Name() );
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
    uiODVw2DHor2DTreeItem* hor2dtreeitm = 0;
    uiODVw2DHor3DTreeItem* hor3dtreeitm = 0;
    for ( int idx=0; idx<treeitms.size(); idx++ )
    {
	mDynamicCast(uiODVw2DHor2DTreeItem*,hor2dtreeitm,treeitms[idx])
	mDynamicCast(uiODVw2DHor3DTreeItem*,hor3dtreeitm,treeitms[idx])
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
    uiODVw2DHor3DTreeItem* hortreeitm = 0;
    uiODVw2DHor2DTreeItem* hor2dtreeitm = 0;
    for ( int idx=0; idx<treeitms.size(); idx++ )
    {
	mDynamicCast( uiODVw2DHor3DTreeItem*,hortreeitm,treeitms[idx])
	mDynamicCast( uiODVw2DHor2DTreeItem*,hor2dtreeitm,treeitms[idx])
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

    appl_.applMgr().mpeServer()->showSetupDlg(emobj->id(),emobj->sectionID(0));
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


void uiODViewer2D::removeSelected( CallBacker* cb )
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
    if ( !mIsUdf(rdmlineid_) )
    {
	const Geometry::RandomLine* rdmline =
		    Geometry::RLM().get( rdmlineid_ );
	if ( rdmline ) info = toUiString( rdmline->name() );
    }
    else if ( tkzs_.hsamp_.survid_ == Survey::GM().get2DSurvID() )
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
	ConstDataPackRef<RegularSeisDataPack> regsdp = vwr.obtainPack( iswva );
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

    ObjectSet<Vw2DDataObject> objs;
    dataMgr()->getObjects( objs );
    for ( int iobj=0; iobj<objs.size(); iobj++ )
    {
	const uiODVw2DTreeItem* childitem =
	    treetp_->getVW2DItem( objs[iobj]->id() );
	if ( !childitem )
	    uiODVw2DTreeItem::create( treeTop(), *this, objs[iobj]->id() );
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

    ConstDataPackRef<FlatDataPack> fdp = vwr.obtainPack( false, true );
    mDynamicCastGet(const SeisFlatDataPack*,seisfdp,fdp.ptr());
    mDynamicCastGet(const MapDataPack*,mapdp,fdp.ptr());
    if ( !seisfdp && !mapdp ) return;

    const TrcKeyValue& trkv = info.trkv_;
    FlatView::Point& pt = marker_->poly_[0];
    if ( seisfdp )
    {
	const int gidx = seisfdp->getSourceDataPack().getGlobalIdx( trkv.tk_ );
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

    FixedString valstr = pars.find( "X" );
    if ( valstr.isEmpty() ) valstr = pars.find( "X-coordinate" );
    if ( !valstr.isEmpty() ) mousepos.x = valstr.toDouble();
    valstr = pars.find( "Y" );
    if ( valstr.isEmpty() ) valstr = pars.find( "Y-coordinate" );
    if ( !valstr.isEmpty() ) mousepos.y = valstr.toDouble();
    valstr = pars.find( "Z" );
    if ( valstr.isEmpty() ) valstr = pars.find( "Z-Coord" );
    if ( !valstr.isEmpty() )
    {
	mousepos.z = valstr.toFloat() / zDomain().userFactor();
	if ( datatransform_ )
	    mousepos.z = datatransform_->transformBack( mousepos );
    }

    if ( mousecursorexchange_ )
    {
	const TrcKeyValue trckeyval =
	    mousepos.isDefined() ? TrcKeyValue(SI().transform(mousepos.coord()),
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


void uiODViewer2D::getVwr2DObjIDs( TypeSet<int>& vw2dobjids ) const
{
    TypeSet<int> vw2dids;
    datamgr_->getObjectIDs( vw2dids );
    vw2dobjids.append( vw2dids );
}


void uiODViewer2D::getHor3DVwr2DIDs( EM::ObjectID emid,
				     TypeSet<int>& vw2dobjids ) const
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DHor3DParentTreeItem*,hor3dpitem,
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
	mDynamicCastGet(uiODVw2DHor3DParentTreeItem*,hor3dpitem,
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
	mDynamicCastGet(uiODVw2DHor3DParentTreeItem*,hor3dpitem,
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
	mDynamicCastGet(uiODVw2DHor3DParentTreeItem*,hor3dpitem,
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
	mDynamicCastGet(uiODVw2DHor3DParentTreeItem*,hor3dpitem,
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
	mDynamicCastGet(uiODVw2DHor3DParentTreeItem*,hor3dpitem,
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
				     TypeSet<int>& vw2dobjids ) const
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DHor2DParentTreeItem*,hor3dpitem,
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
	mDynamicCastGet(uiODVw2DHor2DParentTreeItem*,hor2dpitem,
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
	mDynamicCastGet(uiODVw2DHor2DParentTreeItem*,hor2dpitem,
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
	mDynamicCastGet(uiODVw2DHor2DParentTreeItem*,hor2dpitem,
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
	mDynamicCastGet(uiODVw2DHor2DParentTreeItem*,hor2dpitem,
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
	mDynamicCastGet(uiODVw2DHor2DParentTreeItem*,hor2dpitem,
			treetp_->getChild(idx))
	if ( hor2dpitem )
	    hor2dpitem->addNewTrackingHorizon2D( emid );
    }
}


void uiODViewer2D::getFaultVwr2DIDs( EM::ObjectID emid,
				     TypeSet<int>& vw2dobjids ) const
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DFaultParentTreeItem*,faultpitem,
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
	mDynamicCastGet(uiODVw2DFaultParentTreeItem*,faultpitem,
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
	mDynamicCastGet(uiODVw2DFaultParentTreeItem*,faultpitem,
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
	mDynamicCastGet(uiODVw2DFaultParentTreeItem*,faultpitem,
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
	mDynamicCastGet(uiODVw2DFaultParentTreeItem*,faultpitem,
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
	mDynamicCastGet(uiODVw2DFaultParentTreeItem*,faultpitem,
			treetp_->getChild(idx))
	if ( faultpitem )
	    faultpitem->addNewTempFault( emid );
    }
}


void uiODViewer2D::getFaultSSVwr2DIDs( EM::ObjectID emid,
				     TypeSet<int>& vw2dobjids ) const
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DFaultSSParentTreeItem*,faultsspitem,
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
	mDynamicCastGet(uiODVw2DFaultSSParentTreeItem*,faultpitem,
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
	mDynamicCastGet(uiODVw2DFaultSSParentTreeItem*,faultpitem,
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
	mDynamicCastGet(uiODVw2DFaultSSParentTreeItem*,faultpitem,
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
	mDynamicCastGet(uiODVw2DFaultSSParentTreeItem*,fltsspitem,
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
	mDynamicCastGet(uiODVw2DFaultSSParentTreeItem*,faultpitem,
			treetp_->getChild(idx))
	if ( faultpitem )
	    faultpitem->addNewTempFaultSS( emid );
    }
}


void uiODViewer2D::getFaultSS2DVwr2DIDs( EM::ObjectID emid,
				     TypeSet<int>& vw2dobjids ) const
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DFaultSS2DParentTreeItem*,faultsspitem,
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
	mDynamicCastGet(uiODVw2DFaultSS2DParentTreeItem*,faultpitem,
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
	mDynamicCastGet(uiODVw2DFaultSS2DParentTreeItem*,faultpitem,
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
	mDynamicCastGet(uiODVw2DFaultSS2DParentTreeItem*,faultpitem,
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
	mDynamicCastGet(uiODVw2DFaultSS2DParentTreeItem*,fltsspitem,
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
	mDynamicCastGet(uiODVw2DFaultSS2DParentTreeItem*,faultpitem,
			treetp_->getChild(idx))
	if ( faultpitem )
	    faultpitem->addNewTempFaultSS2D( emid );
    }
}



void uiODViewer2D::getPickSetVwr2DIDs( const MultiID& mid,
				       TypeSet<int>& vw2dobjids ) const
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DPickSetParentTreeItem*,pickpitem,
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
	mDynamicCastGet(uiODVw2DPickSetParentTreeItem*,pickitem,
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
	mDynamicCastGet(uiODVw2DPickSetParentTreeItem*,pickitem,
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
	mDynamicCastGet(uiODVw2DPickSetParentTreeItem*,pickitem,
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
	mDynamicCastGet(uiODVw2DPickSetParentTreeItem*,pickpitem,
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
