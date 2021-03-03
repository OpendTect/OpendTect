/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
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
#include "uiodvw2dvariabledensity.h"
#include "uiodvw2dwigglevararea.h"
#include "uipixmap.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uiusershowwait.h"
#include "uitoolbar.h"
#include "uitreeview.h"
#include "uivispartserv.h"

#include "attribprobelayer.h"
#include "emmanager.h"
#include "emobject.h"
#include "flatposdata.h"
#include "ioobj.h"
#include "mouseevent.h"
#include "probeimpl.h"
#include "randomlineprobe.h"
#include "probemanager.h"
#include "scaler.h"
#include "seisdatapack.h"
#include "volumedatapackzaxistransformer.h"
#include "seisioobjinfo.h"
#include "settings.h"
#include "sorting.h"
#include "survinfo.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "posvecdataset.h"
#include "pickset.h"
#include "randomlinegeom.h"
#include "keystrs.h"

#include "zaxistransform.h"
#include "zaxistransformutils.h"
#include "view2ddataman.h"
#include "view2ddata.h"
#include "od_helpids.h"

static void initSelSpec( Attrib::SelSpec& as )
{ as.set( 0, Attrib::SelSpec::cNoAttribID(), false, 0 ); }

mDefineInstanceCreatedNotifierAccess( uiODViewer2D )

uiODViewer2D::uiODViewer2D( uiODMain& appl, Probe& probe,
			    uiODViewer2D::DispSetup su )
    : appl_(appl)
    , vdselspec_(*new Attrib::SelSpec)
    , wvaselspec_(*new Attrib::SelSpec)
    , viewwin_(nullptr)
    , slicepos_(nullptr)
    , viewstdcontrol_(nullptr)
    , datamgr_(new Vw2DDataManager)
    , tifs_(0)
    , treetp_(0)
    , polyseltbid_(-1)
    , voiidx_(-1)
    , basetxt_(tr("2D Viewer - "))
    , viewWinAvailable(this)
    , viewWinClosed(this)
    , dataChanged(this)
    , posChanged(this)
    , mousecursorexchange_(appl.applMgr().mouseCursorExchange())
    , marker_(0)
    , probe_(probe)
    , dispsetup_(su)
{
    probe_.ref();
    mAttachCB( probe_.objectChanged(), uiODViewer2D::probeChangedCB );
    mAttachCB( mousecursorexchange_.notifier,uiODViewer2D::mouseCursorCB );
    mDefineStaticLocalObject( Threads::Atomic<int>, vwrid, (0) );
    viewerobjid_ = ViewerObjID::get( vwrid++ );

    setWinTitle();

    initSelSpec( vdselspec_ );
    initSelSpec( wvaselspec_ );

    mTriggerInstanceCreatedNotifier();
}


uiODViewer2D::~uiODViewer2D()
{
    detachAllNotifiers();

    mDynamicCastGet(uiFlatViewDockWin*,fvdw,viewwin_)
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
    }

    if ( viewwin_ )
    {
	removeAvailablePacks();
	delete viewwin_->viewer(0).removeAuxData( marker_ );
    }

    probe_.unRef();
    delete viewwin_;
}


bool uiODViewer2D::isVertical() const
{ return probe_.isVertical(); }


Pos::GeomID uiODViewer2D::geomID() const
{
    if ( probe_.position().hsamp_.is2D() )
	return probe_.position().hsamp_.trcKeyAt(0).geomID();

    return mUdfGeomID;
}


uiParent* uiODViewer2D::viewerParent()
{
    return viewwin_->viewerParent();
}


void uiODViewer2D::setUpAux()
{
    if ( !viewwin_ )
	return;

    const bool is2d = !mIsUdfGeomID( geomID() );
    FlatView::Annotation& vwrannot = viewwin_->viewer().appearance().annot_;
    if ( !is2d && !probe_.position().isFlat() )
	vwrannot.x1_.showauxannot_ = vwrannot.x2_.showauxannot_ = false;
    else
    {
	vwrannot.x1_.showauxannot_ = true;
	uiString intersection = tr("%1 intersection");
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

	    if ( probe_.position().defaultDir()==OD::InlineSlice )
	    {
		x1auxnm.arg( uiStrings::sCrossline() );
		x2auxnm.arg( uiStrings::sZSlice() );
	    }
	    else if ( probe_.position().defaultDir()==OD::CrosslineSlice )
	    {
		x1auxnm.arg( uiStrings::sInline() );
		x2auxnm.arg( uiStrings::sZSlice() );
	    }
	    else
	    {
		x1auxnm.arg( uiStrings::sInline() );
		x2auxnm.arg( uiStrings::sCrossline() );;
	    }
	}
    }
}


void uiODViewer2D::setUpView( ProbeLayer::ID curlayid )
{
    const bool isnew = !viewwin_;
    if ( isnew )
    {
	if ( probe_.is2D() )
	    tifs_ = ODMainWin()->viewer2DMgr().treeItemFactorySet2D();
	else
	    tifs_ = ODMainWin()->viewer2DMgr().treeItemFactorySet3D();

	createViewWin();
    }

    updateTransformData();
    updateSlicePos();
    bool vddone = false;
    bool wvadone = false;

    removeAvailablePacks();
    for ( int idx=0; idx<probe_.nrLayers(); idx++ )
    {
	ProbeLayer* prblay = probe_.getLayerByIdx( idx );
	if ( !curlayid.isInvalid() && curlayid!=prblay->getID() )
	    continue;

	mDynamicCastGet(AttribProbeLayer*,attriblayer,prblay)
	if ( !attriblayer || attriblayer->dispType()==AttribProbeLayer::RGB )
	    continue;

	const bool hasdatapack = attriblayer->hasData();
	DataPack::ID attrdpid = attriblayer->dataPackID();
	const bool iswiggle =
	    attriblayer->dispType()==AttribProbeLayer::Wiggle;
	Attrib::SelSpec& selspec = iswiggle ? wvaselspec_ : vdselspec_;
	selspec = attriblayer->selSpec();
	bool& typedone = iswiggle ? wvadone : vddone;
	if ( typedone )
	    continue;

	typedone = true;
	if ( !hasdatapack )
	{
	    attrdpid = createDataPack( iswiggle );
	    NotifyStopper ns( attriblayer->objectChanged(), this );
	    attriblayer->setDataPackID( attrdpid );
	}

	setDataPack( createFlatDataPack(attrdpid,0), iswiggle, isnew );
	for ( int ivwr=0; ivwr<viewwin_->nrViewers(); ivwr++ )
	{
	    uiFlatViewer& vwr = viewwin_->viewer(ivwr);
	    vwr.setMapper( iswiggle, attriblayer->mapper() );
	    if ( !iswiggle )
		vwr.appearance().ddpars_.vd_.colseqname_
			    = attriblayer->sequence().name();
	    vwr.handleChange( FlatView::Viewer::DisplayPars );
	}
    }

    setWinTitle();
    if ( isnew )
	viewwin_->start();
}


void uiODViewer2D::emitPrRequest( Presentation::RequestType req )
{
    Presentation::ObjInfo* prinfo = getObjPrInfo();
    IOPar objprinfopar;
    if ( prinfo )
	prinfo->fillPar( objprinfopar );

    const ViewerID vwrid( uiODViewer2DMgr::theViewerTypeID(), viewerObjID());
    uiUserShowWait usw( &appl_, uiStrings::sUpdatingDisplay() );
    OD::PrMan().handleRequest( vwrid, req, objprinfopar );
}


void uiODViewer2D::setDataPack( DataPack::ID packid, bool wva, bool isnew )
{
    if ( !viewwin_ || packid == DataPack::cNoID() )
	return;

    DataPackMgr& dpm = DPM(DataPackMgr::FlatID());
    for ( int ivwr=0; ivwr<viewwin_->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewwin_->viewer(ivwr);
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


void uiODViewer2D::updateTransformData()
{
    const TrcKeyZSampling& probetkzs = probe_.position();
    if ( datatransform_ && datatransform_->needsVolumeOfInterest() )
    {
	if ( voiidx_ < 0 )
	    voiidx_ = datatransform_->addVolumeOfInterest( probetkzs, true );
	else
	    datatransform_->setVolumeOfInterest( voiidx_, probetkzs, true );

	uiTaskRunnerProvider trprov( &appl_ );
	datatransform_->loadDataIfMissing( voiidx_, trprov );
    }
}


void uiODViewer2D::updateSlicePos()
{
    if ( !slicepos_ )
	return;

    const TrcKeyZSampling& probetkzs = probe_.position();
    if ( probetkzs==slicepos_->getTrcKeyZSampling() )
	return;

    NotifyStopper( slicepos_->positionChg );
    if ( slicepos_ )
    {
	slicepos_->setTrcKeyZSampling( probetkzs );
	if ( datatransform_ )
	{
	    TrcKeyZSampling limittkzs;
	    limittkzs.zsamp_.setFrom( datatransform_->getZInterval(false) );
	    limittkzs.zsamp_.step = datatransform_->getGoodZStep();
	    slicepos_->setLimitSampling( limittkzs );
	}
    }

    if ( probetkzs.isFlat() ) setWinTitle();
}


void uiODViewer2D::createViewWin()
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
	if ( probe_.is3DSlice() )
	{
	    slicepos_ = new uiSlicePos2DView( fvmw, ZDomain::Info(zDomain()) );
	    slicepos_->setTrcKeyZSampling( probe_.position() );
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

    for ( int ivwr=0; ivwr<viewwin_->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewwin_->viewer( ivwr);
	vwr.setZAxisTransform( datatransform_.ptr() );
	vwr.appearance().setDarkBG( wantdock );
	vwr.appearance().setGeoDefaults(probe_.isVertical());
	vwr.appearance().annot_.setAxesAnnot(true);
    }

    const float initialx2pospercm =
	probe_.isVertical() ? dispsetup_.initialx2pospercm_
			    : dispsetup_.initialx1pospercm_;
    uiFlatViewer& mainvwr = viewwin_->viewer();
    viewstdcontrol_ = new uiFlatViewStdControl( mainvwr,
	    uiFlatViewStdControl::Setup(controlparent).helpkey(
			mODHelpKey(mODViewer2DHelpID) )
			.withedit(tifs_).isvertical(probe_.isVertical())
			.withfixedaspectratio(true)
			.withhomebutton(true)
			.initialx1pospercm(dispsetup_.initialx1pospercm_)
			.initialx2pospercm(initialx2pospercm)
			.initialcentre(dispsetup_.initialcentre_)
			.withscalebarbut(true)
			.managecoltab(!tifs_) );

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
	const OD::Pol2D3D pol2d3d = (OD::Pol2D3D)tifs_->getPol2D3D( idx );
	if ( SI().survDataType() == OD::Both2DAnd3D
	     || pol2d3d == OD::Both2DAnd3D
	     || pol2d3d == SI().survDataType() )
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

    polyseltbid_ = tb->addButton( "polygonselect", tr("Polygon Selection mode"),
				  mCB(this,uiODViewer2D,selectionMode), true );
    uiMenu* polymnu = tb->addButtonMenu( polyseltbid_ );

    uiAction* polyitm = new uiAction( uiStrings::sPolygon(),
				      mCB(this,uiODViewer2D,handleToolClick) );
    polymnu->insertAction( polyitm, 0 );
    polyitm->setIcon( "polygonselect" );

    uiAction* rectitm = new uiAction( uiStrings::sRectangle(),
				      mCB(this,uiODViewer2D,handleToolClick) );
    polymnu->insertAction( rectitm, 1 );
    rectitm->setIcon( "rectangleselect" );

    tb->addButton( "clearselection", tr("Remove Selection"),
			mCB(this,uiODViewer2D,removeSelected), false );
}


void uiODViewer2D::createViewWinEditors()
{
    if ( !viewwin_ )
	return;

    for ( int ivwr=0; ivwr<viewwin_->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewwin_->viewer( ivwr);
	uiFlatViewAuxDataEditor* adeditor = new uiFlatViewAuxDataEditor( vwr );
	adeditor->setSelActive( false );
	auxdataeditors_ += adeditor;
    }
}


void uiODViewer2D::winCloseCB( CallBacker* cb )
{
    deleteAndZeroPtr( treetp_ );
    datamgr_->removeAll();

    deepErase( auxdataeditors_ );
    removeAvailablePacks();

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
    if ( !viewwin_ )
	{ pErrMsg("No main window"); return; }


    for ( int ivwr=0; ivwr<viewwin_->nrViewers(); ivwr++ )
	viewwin_->viewer(ivwr).clearAllPacks();
}


void uiODViewer2D::setSelSpec( const Attrib::SelSpec* as, bool wva )
{
    if ( as )
	(wva ? wvaselspec_ : vdselspec_) = *as;
    else
	initSelSpec( wva ? wvaselspec_ : vdselspec_ );
}


void uiODViewer2D::probeChangedCB( CallBacker* )
{
    setUpView();
}


void uiODViewer2D::posChg( CallBacker* )
{
    probe_.setPos( slicepos_->getTrcKeyZSampling() );
}


DataPack::ID uiODViewer2D::createDataPack(const Attrib::SelSpec& selspec)
{
    TrcKeyZSampling tkzs = slicepos_ ? slicepos_->getTrcKeyZSampling()
				     : probe_.position();

    const ZAxisTransform* zat = getZAxisTransform();
    if ( zat && !selspec.isZTransformed() )
    {
	if ( tkzs.nrZ() == 1 )
	    return createDataPackForTransformedZSlice( selspec );
	tkzs.zsamp_.setFrom( zat->getZInterval(true) );
	tkzs.zsamp_.step = SI().zStep();
    }

    uiAttribPartServer* attrserv = appl_.applMgr().attrServer();
    attrserv->setTargetSelSpec( selspec );

    mDynamicCastGet(const RandomLineProbe*,rdlprobe,&probe_);
    if ( rdlprobe )
	return attrserv->createRdmTrcsOutput( tkzs.zsamp_,
					      rdlprobe->randomeLineID() );

    return attrserv->createOutput( tkzs, DataPack::cNoID() );
}


DataPack::ID uiODViewer2D::createFlatDataPack( DataPack::ID dpid, int comp )
{
    const DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    auto seisdp = dpm.get<VolumeDataPack>( dpid );
    if ( !seisdp || !(comp<seisdp->nrComponents()) )
	return dpid;

    const FixedString zdomainkey( seisdp->zDomain().key() );
    const bool alreadytransformed =
	!zdomainkey.isEmpty() && zdomainkey!=ZDomain::SI().key();
    if ( datatransform_ && !alreadytransformed )
    {
	VolumeDataPackZAxisTransformer transformer( *datatransform_.ptr() );
	transformer.setInput( seisdp.ptr() );
	transformer.setInterpolate( true );
	transformer.execute();
	if ( transformer.getOutput() )
            seisdp = transformer.getOutput();
    }

    mDynamicCastGet(const RegularSeisDataPack*,regsdp,seisdp.ptr());
    mDynamicCastGet(const RandomSeisDataPack*,randsdp,seisdp.ptr());
    SeisFlatDataPack* seisfdp = 0;
    if ( regsdp )
	seisfdp = new RegularSeisFlatDataPack( *regsdp, comp );
    else if ( randsdp )
	seisfdp = new RandomSeisFlatDataPack( *randsdp, comp );
    DPM(DataPackMgr::FlatID()).add( seisfdp );
    return seisfdp ? seisfdp->id() : DataPack::cNoID();
}


DataPack::ID uiODViewer2D::createDataPackForTransformedZSlice(
					const Attrib::SelSpec& selspec )
{
    if ( !hasZAxisTransform() || selspec.isZTransformed() )
	return DataPack::cNoID();

    const TrcKeyZSampling& tkzs = slicepos_ ? slicepos_->getTrcKeyZSampling()
					    : probe_.position();
    if ( tkzs.nrZ() != 1 ) return DataPack::cNoID();

    uiAttribPartServer* attrserv = appl_.applMgr().attrServer();
    attrserv->setTargetSelSpec( selspec );

    RefMan<DataPointSet> dps = new DataPointSet( false, true );
    DPM(DataPackMgr::PointID()).add( dps );

    ZAxisTransformPointGenerator generator( *datatransform_.ptr() );
    generator.setInput( tkzs, SilentTaskRunnerProvider() );
    generator.setOutputDPS( *dps );
    generator.execute();

    const int firstcol = dps->nrCols();
    BufferStringSet userrefs; userrefs.add( selspec.userRef() );
    dps->dataSet().add( new DataColDef(userrefs.get(0)) );
    if ( !attrserv->createOutput(*dps,firstcol) )
	return DataPack::cNoID();

    return RegularSeisDataPack::createDataPackForZSlice(
	    &dps->bivSet(), tkzs, datatransform_->toZDomainInfo(), &userrefs );
}


bool uiODViewer2D::useStoredDispPars( bool wva )
{
    if ( !viewwin_ )
	return false;

    PtrMan<IOObj> ioobj = appl_.applMgr().attrServer()->getIOObj(selSpec(wva));
    if ( !ioobj )
	return false;

    SeisIOObjInfo seisobj( ioobj );
    IOPar iop;
    if ( !seisobj.getDisplayPars(iop) )
	return false;

    RefMan<ColTab::MapperSetup> mappersetup = new ColTab::MapperSetup;
    mappersetup->usePar( iop );

    for ( int ivwr=0; ivwr<viewwin_->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewwin_->viewer( ivwr );
	FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;
	(wva ? ddp.wva_.mapper_ : ddp.vd_.mapper_)->setup() = *mappersetup;
	if ( !wva )
	    ddp.vd_.colseqname_ = iop.find( sKey::Name() );
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
    const DBKey emobjid = hor2dtreeitm ? hor2dtreeitm->emObjectID()
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

    const DBKey emid = hortreeitm ? hortreeitm->emObjectID()
					 : hor2dtreeitm->emObjectID();
    EM::Object* emobj = EM::MGR().getObject( emid );
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


void uiODViewer2D::removeSelected( CallBacker* cb )
{
    if ( !viewstdcontrol_->editToolBar() ||
	 !viewstdcontrol_->editToolBar()->isOn(polyseltbid_) )
	return;

    for ( int edidx=0; edidx<auxdataeditors_.size(); edidx++ )
	auxdataeditors_[edidx]->removePolygonSelected( -1 );
}


void uiODViewer2D::setWinTitle()
{
    uiString info = toUiString("%1: %2");

    mDynamicCastGet(const RandomLineProbe*,rdlprobe,&probe_);
    if ( rdlprobe )
    {
	const Geometry::RandomLine* rdmline =
		    Geometry::RLM().get( rdlprobe->randomeLineID() );
	if ( rdmline )
	    info = toUiString( rdmline->name() );
    }
    else if ( probe_.position().hsamp_.is2D() )
    {
	info.arg( uiStrings::sLine() ).arg( geomID().name() );
    }
    else if ( probe_.position().defaultDir() == OD::InlineSlice )
    {
	info.arg( uiStrings::sInline() )
	    .arg( probe_.position().hsamp_.start_.inl() );
    }
    else if ( probe_.position().defaultDir() == OD::CrosslineSlice )
    {
	info.arg( uiStrings::sCrossline() )
	    .arg( probe_.position().hsamp_.start_.crl() );
    }
    else
    {
	info.arg( zDomain().userName() )
	    .arg( mNINT32(probe_.position().zsamp_.start *
			  zDomain().userFactor()) );
    }

    uiString title = toUiString("%1%2").arg(basetxt_).arg(info);
    if ( viewwin_ )
	viewwin_->setWinTitle( title );
}

//TODO PrIMPL re-implement via Probe
void uiODViewer2D::usePar( const IOPar& iop )
{
    if ( !viewwin_ )
	return;

    IOPar* vdselspecpar = iop.subselect( sKeyVDSelSpec() );
    if ( vdselspecpar ) vdselspec_.usePar( *vdselspecpar );
    IOPar* wvaselspecpar = iop.subselect( sKeyWVASelSpec() );
    if ( wvaselspecpar ) wvaselspec_.usePar( *wvaselspecpar );
    delete vdselspecpar; delete wvaselspecpar;
    IOPar* tkzspar = iop.subselect( sKeyPos() );
    TrcKeyZSampling tkzs; if ( tkzspar ) tkzs.usePar( *tkzspar );
    if ( viewwin_->nrViewers() > 0 )
    {
	const uiFlatViewer& vwr = viewwin_->viewer(0);
	const bool iswva = wvaselspec_.id().isValid();
	ConstRefMan<RegularSeisDataPack> regsdp = vwr.getPack( iswva );
	//TODO PrIMPL remove later if ( regsdp ) setPos( tkzs );
    }

    datamgr_->usePar( iop, viewwin_, dataEditor() );
    rebuildTree();
}


void uiODViewer2D::fillPar( IOPar& iop ) const
{
    IOPar vdselspecpar, wvaselspecpar;
    vdselspec_.fillPar( vdselspecpar );
    wvaselspec_.fillPar( wvaselspecpar );
    iop.mergeComp( vdselspecpar, sKeyVDSelSpec() );
    iop.mergeComp( wvaselspecpar, sKeyWVASelSpec() );
    IOPar pospar; probe_.position().fillPar( pospar );
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


void uiODViewer2D::mouseCursorCB( CallBacker* cb )
{
    if ( !viewwin_ || viewwin_->nrViewers() < 1 )
	return;

    mCBCapsuleUnpackWithCaller(const MouseCursorExchange::Info&,info,
			       caller,cb);
    uiFlatViewer& vwr = viewwin_->viewer( 0 );
    if ( caller==this )
    {
	if ( marker_ )
	{
	    marker_->poly_[0] = FlatView::Point(0,0);
	    vwr.handleChange( FlatView::Viewer::Auxdata );
	}

	return;
    }

    if ( !marker_ )
    {
	marker_ = vwr.createAuxData( "XYZ Marker" );
	vwr.addAuxData( marker_ );
	marker_->poly_ += FlatView::Point(0,0);
	marker_->markerstyles_ += OD::MarkerStyle2D();
    }

    ConstRefMan<FlatDataPack> fdp = vwr.getPack( false, true );
    mDynamicCastGet(const SeisFlatDataPack*,seisfdp,fdp.ptr());
    mDynamicCastGet(const MapDataPack*,mapdp,fdp.ptr());
    if ( !seisfdp && !mapdp )
	return;

    const TrcKeyValue& trkv = info.trkv_;
    FlatView::Point& pt = marker_->poly_[0];
    if ( seisfdp )
    {
	const int gidx = seisfdp->getSourceDataPack().getGlobalIdx( trkv.tk_ );
	const FlatPosData& posdata = fdp->posData();
	const TrcKeyZSampling& probepos = probe_.position();
	if ( seisfdp->isVertical() )
	{
	    pt.x_ = posdata.range(true).atIndex( gidx );
	    pt.y_ = datatransform_ ?
		   datatransform_->transformTrc( trkv.tk_, trkv.val_ ) :
		   trkv.val_;
	}
	else
	{
	    pt.x_ = posdata.range(true).atIndex( gidx / probepos.nrTrcs() );
	    pt.y_ = posdata.range(false).atIndex( gidx % probepos.nrTrcs() );
	}
    }
    else if ( mapdp )
    {
	const Coord pos = trkv.tk_.getCoord();
	pt = FlatView::Point( pos.x_, pos.y_ );
    }

    vwr.handleChange( FlatView::Viewer::Auxdata );
}


void uiODViewer2D::mouseMoveCB( CallBacker* cb )
{
    Coord3 mousepos( Coord3::udf() );
    mCBCapsuleUnpack(IOPar,pars,cb);

    FixedString valstr = pars.find( sKey::X() );
    if ( valstr.isEmpty() )
	valstr = pars.find( "X-coordinate" );
    if ( !valstr.isEmpty() )
	mousepos.x_ = valstr.toDouble();
    valstr = pars.find( sKey::Y() );
    if ( valstr.isEmpty() )
	valstr = pars.find( "Y-coordinate" );
    if ( !valstr.isEmpty() )
	mousepos.y_ = valstr.toDouble();
    valstr = pars.find( sKey::Z() );
    if ( valstr.isEmpty() )
	valstr = pars.find( "Z-Coord" );
    if ( !valstr.isEmpty() )
    {
	mousepos.z_ = valstr.toDouble() / zDomain().userFactor();
	if ( datatransform_ )
	    mousepos.z_ = datatransform_->transformBack( mousepos );
    }

    const TrcKeyValue trckeyval =
	mousepos.isDefined() ? TrcKeyValue(
		TrcKey(SI().transform(mousepos.getXY())), (float)mousepos.z_)
			     : TrcKeyValue::udf();

    MouseCursorExchange::Info info( trckeyval );
    mousecursorexchange_.notifier.trigger( info, this );
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


void uiODViewer2D::getHor3DVwr2DIDs( const DBKey& emid,
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


void uiODViewer2D::removeHorizon3D( const DBKey& emid )
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


void uiODViewer2D::getLoadedHorizon3Ds( DBKeySet& emids ) const
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


void uiODViewer2D::addHorizon3Ds( const DBKeySet& emids )
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


void uiODViewer2D::setupTrackingHorizon3D( const DBKey& emid )
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


void uiODViewer2D::addNewTrackingHorizon3D( const DBKey& emid )
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


void uiODViewer2D::getHor2DVwr2DIDs( const DBKey& emid,
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


void uiODViewer2D::removeHorizon2D( const DBKey& emid )
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


void uiODViewer2D::getLoadedHorizon2Ds( DBKeySet& emids ) const
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


void uiODViewer2D::addHorizon2Ds( const DBKeySet& emids )
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


void uiODViewer2D::setupTrackingHorizon2D( const DBKey& emid )
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


void uiODViewer2D::addNewTrackingHorizon2D( const DBKey& emid )
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


void uiODViewer2D::getFaultVwr2DIDs( const DBKey& emid,
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


void uiODViewer2D::removeFault( const DBKey& emid )
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


void uiODViewer2D::getLoadedFaults( DBKeySet& emids ) const
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


void uiODViewer2D::addFaults( const DBKeySet& emids )
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


void uiODViewer2D::setupNewTempFault( const DBKey& emid )
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


void uiODViewer2D::addNewTempFault( const DBKey& emid )
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


void uiODViewer2D::getFaultSSVwr2DIDs( const DBKey& emid,
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


void uiODViewer2D::removeFaultSS( const DBKey& emid )
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


void uiODViewer2D::getLoadedFaultSSs( DBKeySet& emids ) const
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


void uiODViewer2D::addFaultSSs( const DBKeySet& emids )
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


void uiODViewer2D::setupNewTempFaultSS( const DBKey& emid )
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



void uiODViewer2D::addNewTempFaultSS( const DBKey& emid )
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


void uiODViewer2D::getFaultSS2DVwr2DIDs( const DBKey& emid,
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


void uiODViewer2D::removeFaultSS2D( const DBKey& emid )
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


void uiODViewer2D::getLoadedFaultSS2Ds( DBKeySet& emids ) const
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


void uiODViewer2D::addFaultSS2Ds( const DBKeySet& emids )
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


void uiODViewer2D::setupNewTempFaultSS2D( const DBKey& emid )
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


void uiODViewer2D::addNewTempFaultSS2D( const DBKey& emid )
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



void uiODViewer2D::getPickSetVwr2DIDs( const DBKey& mid,
				       TypeSet<int>& vw2dobjids ) const
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DPickSetParentTreeItem*,pickpitem,
			treetp_->getChild(idx))
	if ( pickpitem )
	    pickpitem->getVwr2DOjIDs( mid, vw2dobjids );
    }
}


void uiODViewer2D::removePickSet( const DBKey& mid )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DPickSetParentTreeItem*,pickitem,
			treetp_->getChild(idx))
	if ( pickitem )
	{
	    Pick::SetPresentationInfo newpickprinfo( mid );
	    pickitem->removeChildren( newpickprinfo );
	}
    }
}


void uiODViewer2D::getLoadedPickSets( DBKeySet& dbkeys ) const
{
    if ( !treetp_ ) return;

    Presentation::ObjInfoSet loadedobjs;
    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DPickSetParentTreeItem*,pickitem,
			treetp_->getChild(idx))
	if ( pickitem )
	{
	    pickitem->getLoadedChildren( loadedobjs );
	    for ( int lidx=0; lidx<loadedobjs.size(); lidx++ )
	    {
		mDynamicCastGet(const Pick::SetPresentationInfo*,pickprinfo,
				loadedobjs.get(lidx));
		if ( !pickprinfo )
		    continue;

		dbkeys += pickprinfo->storedID();
	    }
	}
    }
}


void uiODViewer2D::addPickSets( const DBKeySet& mids )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DPickSetParentTreeItem*,pickitem,
			treetp_->getChild(idx))
	if ( pickitem )
	{
	    Presentation::ObjInfoSet prinfos;
	    for ( int pidx=0; pidx<mids.size(); pidx++ )
		prinfos.add( new Pick::SetPresentationInfo(mids[pidx]) );
	    pickitem->addChildren( prinfos );
	}
    }
}


void uiODViewer2D::setupNewPickSet( const DBKey& pickid )
{
    if ( !treetp_ ) return;

    for ( int idx=0; idx<treetp_->nrChildren(); idx++ )
    {
	mDynamicCastGet(uiODVw2DPickSetParentTreeItem*,pickpitem,
			treetp_->getChild(idx))
	if ( pickpitem )
	{
	    Presentation::ObjInfoSet pickprinfos;
	    Pick::SetPresentationInfo* newpickprinfo =
		new Pick::SetPresentationInfo( pickid );
	    pickprinfos.add( newpickprinfo );
	    pickpitem->addChildren( pickprinfos );
	    if ( viewstdcontrol_->editToolBar() &&
		 pickpitem->selectChild(*newpickprinfo) )
		viewstdcontrol_->editToolBar()->setSensitive(
			 picksettingstbid_, false );
	}
    }
}


Presentation::ObjInfo* uiODViewer2D::getObjPrInfo() const
{
    ProbePresentationInfo* prinfo =
	new ProbePresentationInfo( ProbeMGR().getID(probe_) );
    return prinfo;
}
