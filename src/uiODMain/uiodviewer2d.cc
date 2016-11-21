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
#include "uiodvw2dvariabledensity.h"
#include "uiodvw2dwigglevararea.h"
#include "uipixmap.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"
#include "uitreeview.h"
#include "uivispartserv.h"

#include "attribprobelayer.h"
#include "emmanager.h"
#include "emobject.h"
#include "filepath.h"
#include "flatposdata.h"
#include "ioobj.h"
#include "mouseevent.h"
#include "probeimpl.h"
#include "randomlineprobe.h"
#include "probemanager.h"
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
#include "pickset.h"
#include "randomlinegeom.h"

#include "zaxistransform.h"
#include "zaxistransformutils.h"
#include "view2ddataman.h"
#include "view2ddata.h"
#include "od_helpids.h"

static void initSelSpec( Attrib::SelSpec& as )
{ as.set( 0, Attrib::SelSpec::cNoAttrib(), false, 0 ); }

mDefineInstanceCreatedNotifierAccess( uiODViewer2D )

uiODViewer2D::uiODViewer2D( uiODMain& appl, Probe& probe,
			    uiODViewer2D::DispSetup su )
    : OD::PresentationManagedViewer()
    , appl_(appl)
    , vdselspec_(*new Attrib::SelSpec)
    , wvaselspec_(*new Attrib::SelSpec)
    , viewwin_(0)
    , slicepos_(0)
    , viewstdcontrol_(0)
    , datamgr_(new Vw2DDataManager)
    , tifs_(0)
    , treetp_(0)
    , polyseltbid_(-1)
    , voiidx_(-1)
    , basetxt_(tr("2D Viewer - "))
    , isvertical_(true)
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
    viewerobjid_ = OD::ViewerObjID::get( vwrid++ );

    setWinTitle();

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
    }

    if ( viewwin() )
    {
	removeAvailablePacks();
	viewwin()->viewer(0).removeAuxData( marker_ );
    }

    probe_.unRef();
    delete marker_;
    delete viewwin();
}


Pos::GeomID uiODViewer2D::geomID() const
{
    if ( probe_.position().hsamp_.survid_ == Survey::GM().get2DSurvID() )
	return probe_.position().hsamp_.trcKeyAt(0).geomID();

    return Survey::GM().cUndefGeomID();
}


uiParent* uiODViewer2D::viewerParent()
{ return viewwin_->viewerParent(); }


void uiODViewer2D::setUpAux()
{
    const bool is2d = geomID() != Survey::GM().cUndefGeomID();
    FlatView::Annotation& vwrannot = viewwin()->viewer().appearance().annot_;
    if ( !is2d && !probe_.position().isFlat() )
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

	    if ( probe_.position().defaultDir()==TrcKeyZSampling::Inl )
	    {
		x1auxnm.arg( uiStrings::sCrossline() );
		x2auxnm.arg( uiStrings::sZSlice() );
	    }
	    else if ( probe_.position().defaultDir()==TrcKeyZSampling::Crl )
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
    const bool isnew = !viewwin();
    if ( isnew )
    {
	if ( probe_.type()==Line2DProbe::sFactoryKey() )
	    tifs_ = ODMainWin()->viewer2DMgr().treeItemFactorySet2D();
	else
	    tifs_ = ODMainWin()->viewer2DMgr().treeItemFactorySet3D();

	isvertical_ = probe_.type()!=ZSliceProbe::sFactoryKey();
	const bool is2d = probe_.type()==Line2DProbe::sFactoryKey();
	const bool isrdl = probe_.type()==RDLProbe::sFactoryKey();
	createViewWin( isvertical_, !is2d || !isrdl );
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
	if ( !attriblayer || attriblayer->getDispType()==AttribProbeLayer::RGB )
	    continue;

	const bool hasdatapack = attriblayer->hasData();
	DataPack::ID attrdpid = attriblayer->getAttribDataPack();
	const bool iswiggle =
	    attriblayer->getDispType()==AttribProbeLayer::Wiggle;
	Attrib::SelSpec& selspec = iswiggle ? wvaselspec_ : vdselspec_;
	selspec = attriblayer->getSelSpec();
	bool& typedone = iswiggle ? wvadone : vddone;
	if ( typedone )
	    continue;

	typedone = true;
	if ( !hasdatapack )
	{
	    attrdpid = createDataPack( iswiggle );
	    ChangeNotifyBlocker notifybocker( *attriblayer );
	    attriblayer->setAttribDataPack( attrdpid );
	    notifybocker.unBlockNow( false );
	}

	setDataPack( createFlatDataPack(attrdpid,0), iswiggle, isnew );
	for ( int ivwr=0; ivwr<viewwin()->nrViewers(); ivwr++ )
	{
	    uiFlatViewer& vwr = viewwin()->viewer(ivwr);
	    if ( !iswiggle )
	    {
		vwr.appearance().ddpars_.vd_.ctab_ =
		    attriblayer->getColTab().name();
		vwr.appearance().ddpars_.vd_.mappersetup_ =
		    attriblayer->getColTabMapper();
	    }
	    else
		vwr.appearance().ddpars_.wva_.mappersetup_ =
		    attriblayer->getColTabMapper();

	    vwr.handleChange( FlatView::Viewer::DisplayPars );
	}
    }

    setWinTitle();
    if ( isnew )
	viewwin()->start();
}


void uiODViewer2D::emitPRRequest( OD::PresentationRequestType req )
{
    OD::ViewerID vwrid( uiODViewer2DMgr::theViewerTypeID(), viewerObjID());
    OD::ObjPresentationInfo* prinfo = getObjPRInfo();
    IOPar objprinfopar;
    prinfo->fillPar( objprinfopar );
    OD::PrMan().request( vwrid, req, objprinfopar );
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


void uiODViewer2D::updateTransformData()
{
    const TrcKeyZSampling& probetkzs = probe_.position();
    uiTaskRunner taskr( &appl_ );
    if ( datatransform_ && datatransform_->needsVolumeOfInterest() )
    {
	if ( voiidx_ < 0 )
	    voiidx_ = datatransform_->addVolumeOfInterest( probetkzs, true );
	else
	    datatransform_->setVolumeOfInterest( voiidx_, probetkzs, true );

	datatransform_->loadDataIfMissing( voiidx_, &taskr );
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


void uiODViewer2D::createViewWin( bool isvert, bool needslicepos )
{
    bool wantdock = false;
    Settings::common().getYN( "FlatView.Use Dockwin", wantdock );
    uiParent* controlparent = 0;
    if ( !wantdock )
    {
	uiFlatViewMainWin* fvmw = new uiFlatViewMainWin( 0,
		uiFlatViewMainWin::Setup(basetxt_).deleteonclose(true) );
	mAttachCB( fvmw->windowClosed, uiODViewer2D::winCloseCB );

	if ( needslicepos )
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
	uiFlatViewDockWin* dwin = new uiFlatViewDockWin( &appl_,
				   uiFlatViewDockWin::Setup(basetxt_) );
	appl_.addDockWindow( *dwin, uiMainWin::Top );
	dwin->setFloating( true );
	viewwin_ = dwin;
	controlparent = &appl_;
    }

    viewwin_->setInitialSize( 700, 400 );

    for ( int ivwr=0; ivwr<viewwin_->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewwin()->viewer( ivwr);
	vwr.setZAxisTransform( datatransform_.ptr() );
	vwr.appearance().setDarkBG( wantdock );
	vwr.appearance().setGeoDefaults(isvert);
	vwr.appearance().annot_.setAxesAnnot(true);
    }

    const float initialx2pospercm = isvert ? dispsetup_.initialx2pospercm_
					   : dispsetup_.initialx1pospercm_;
    uiFlatViewer& mainvwr = viewwin()->viewer();
    viewstdcontrol_ = new uiFlatViewStdControl( mainvwr,
	    uiFlatViewStdControl::Setup(controlparent).helpkey(
			mODHelpKey(mODViewer2DHelpID) )
			.withedit(tifs_).isvertical(isvert)
			.withfixedaspectratio(true)
			.withhomebutton(true)
			.initialx1pospercm(dispsetup_.initialx1pospercm_)
			.initialx2pospercm(initialx2pospercm)
			.initialcentre(dispsetup_.initialcentre_)
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

    polyseltbid_ = tb->addButton( "polygonselect", tr("Polygon Selection mode"),
				  mCB(this,uiODViewer2D,selectionMode), true );
    uiMenu* polymnu = new uiMenu( tb, toUiString("PolyMenu") );

    uiAction* polyitm = new uiAction( uiStrings::sPolygon(),
				      mCB(this,uiODViewer2D,handleToolClick) );
    polymnu->insertItem( polyitm, 0 );
    polyitm->setIcon( "polygonselect" );

    uiAction* rectitm = new uiAction( uiStrings::sRectangle(),
				      mCB(this,uiODViewer2D,handleToolClick) );
    polymnu->insertItem( rectitm, 1 );
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

    mDynamicCastGet(const RDLProbe*,rdlprobe,&probe_);
    if ( rdlprobe )
	return attrserv->createRdmTrcsOutput( tkzs.zsamp_,
					      rdlprobe->randomeLineID() );

    return attrserv->createOutput( tkzs, DataPack::cNoID() );
}


DataPack::ID uiODViewer2D::createFlatDataPack( DataPack::ID dpid, int comp )
{
    DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
    ConstRefMan<SeisDataPack> seisdp = dpm.get( dpid );
    if ( !seisdp || !(comp<seisdp->nrComponents()) ) return dpid;

    const FixedString zdomainkey( seisdp->zDomain().key() );
    const bool alreadytransformed =
	!zdomainkey.isEmpty() && zdomainkey!=ZDomain::SI().key();
    if ( datatransform_ && !alreadytransformed )
    {
	SeisDataPackZAxisTransformer transformer( *datatransform_.ptr() );
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
	seisfdp = new RegularFlatDataPack( *regsdp, comp );
    else if ( randsdp )
	seisfdp = new RandomFlatDataPack( *randsdp, comp );
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

    RefMan<DataPointSet> data =
	DPM(DataPackMgr::PointID()).add(new DataPointSet(false,true));

    ZAxisTransformPointGenerator generator( *datatransform_.ptr() );
    generator.setInput( tkzs );
    generator.setOutputDPS( *data );
    generator.execute();

    const int firstcol = data->nrCols();
    BufferStringSet userrefs; userrefs.add( selspec.userRef() );
    data->dataSet().add( new DataColDef(userrefs.get(0)) );
    if ( !attrserv->createOutput(*data,firstcol) )
	return DataPack::cNoID();

    return RegularSeisDataPack::createDataPackForZSlice(
	    &data->bivSet(), tkzs, datatransform_->toZDomainInfo(), userrefs );
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


void uiODViewer2D::setWinTitle()
{
    uiString info = toUiString("%1: %2");

    mDynamicCastGet(const RDLProbe*,rdlprobe,&probe_);
    if ( rdlprobe )
    {
	const Geometry::RandomLine* rdmline =
		    Geometry::RLM().get( rdlprobe->randomeLineID() );
	if ( rdmline ) info = toUiString( rdmline->name() );
    }
    else if ( probe_.position().hsamp_.survid_ == Survey::GM().get2DSurvID() )
    {
	info.arg( tr("Line") )
	    .arg( toUiString( Survey::GM().getName(geomID()) ) );
    }
    else if ( probe_.position().defaultDir() == TrcKeyZSampling::Inl )
    {
	info.arg( uiStrings::sInline() )
	    .arg( probe_.position().hsamp_.start_.inl() );
    }
    else if ( probe_.position().defaultDir() == TrcKeyZSampling::Crl )
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
    if ( viewwin() )
	viewwin()->setWinTitle( title );
}

//TODO PrIMPL re-implement via Probe
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
	ConstRefMan<RegularSeisDataPack> regsdp = vwr.getPack( iswva );
	//TODO PrIMPL remove later if ( regsdp ) setPos( tkzs );
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
    mCBCapsuleUnpackWithCaller(const MouseCursorExchange::Info&,info,
			       caller,cb);
    uiFlatViewer& vwr = viewwin()->viewer(0);
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
    if ( !seisfdp && !mapdp ) return;

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
	const Coord pos = Survey::GM().toCoord( trkv.tk_ );
	pt = FlatView::Point( pos.x_, pos.y_ );
    }

    vwr.handleChange( FlatView::Viewer::Auxdata );
}


void uiODViewer2D::mouseMoveCB( CallBacker* cb )
{
    Coord3 mousepos( Coord3::udf() );
    mCBCapsuleUnpack(IOPar,pars,cb);

    FixedString valstr = pars.find( "X" );
    if ( valstr.isEmpty() ) valstr = pars.find( "X-coordinate" );
    if ( !valstr.isEmpty() ) mousepos.x_ = valstr.toDouble();
    valstr = pars.find( "Y" );
    if ( valstr.isEmpty() ) valstr = pars.find( "Y-coordinate" );
    if ( !valstr.isEmpty() ) mousepos.y_ = valstr.toDouble();
    valstr = pars.find( "Z" );
    if ( valstr.isEmpty() ) valstr = pars.find( "Z-Coord" );
    if ( !valstr.isEmpty() )
    {
	mousepos.z_ = valstr.toDouble() / zDomain().userFactor();
	if ( datatransform_ )
	    mousepos.z_ = datatransform_->transformBack( mousepos );
    }

    const TrcKeyValue trckeyval =
	mousepos.isDefined() ? TrcKeyValue(SI().transform(mousepos.getXY()),
					    mCast(float,mousepos.z_))
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

    OD::ObjPresentationInfoSet loadedobjs;
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
	    OD::ObjPresentationInfoSet prinfos;
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
	    OD::ObjPresentationInfoSet pickprinfos;
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


OD::ObjPresentationInfo* uiODViewer2D::getObjPRInfo() const
{
    ProbePresentationInfo* prinfo =
	new ProbePresentationInfo( ProbeMGR().getID(probe_) );
    return prinfo;
}
