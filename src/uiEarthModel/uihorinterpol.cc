/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uihorinterpol.h"

#include "arrayndimpl.h"
#include "array1dinterpol.h"
#include "array2dinterpol.h"
#include "arraynd.h"
#include "ctxtioobj.h"
#include "datainpspec.h"
#include "executor.h"
#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "gridder2d.h"
#include "horizongridder.h"
#include "pickset.h"
#include "picksettr.h"
#include "survinfo.h"

#include "uiarray1dinterpol.h"
#include "uiarray2dinterpol.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uihorsavefieldgrp.h"
#include "uiioobjsel.h"
#include "uiiosurface.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitaskrunner.h"
#include "od_helpids.h"

#define mScopeSurvey	0
#define mScopeBBX	1
#define mScopeConvex	2
#define mScopeHoles	3
#define mScopePolygon	4

static float defaultRadius()
{
    return 2 * mMAX( SI().inlDistance()*SI().inlStep(),
		     SI().crlDistance()*SI().crlStep() );
}

uiHorizonInterpolDlg::uiHorizonInterpolDlg( uiParent* p, EM::Horizon* hor,
					    bool is2d )
    : uiDialog(p,uiDialog::Setup(tr("Horizon Gridding"),mNoDlgTitle,
		mODHelpKey(mInverseDistanceArray2DInterpolHelpID)).modal(true))
    , horizon_(hor)
    , is2d_(is2d)
    , inputhorsel_(nullptr)
    , interpolhor3dsel_(nullptr)
    , interpol1dsel_(nullptr)
    , savefldgrp_(nullptr)
    , finished(this)
    , horReadyForDisplay(this)
{
    if ( !hor )
	setCtrlStyle( RunAndClose );

    if ( horizon_ )
	horizon_->ref();
    else
    {
	IOObjContext ctxt = is2d ? EMHorizon2DTranslatorGroup::ioContext()
				 : EMHorizon3DTranslatorGroup::ioContext();
	ctxt.forread_ = true;
	inputhorsel_ = new uiIOObjSel( this, ctxt );
	mAttachCB( inputhorsel_->selectionDone,
		   uiHorizonInterpolDlg::selChangeCB );
    }

    if ( !is2d )
    {
	interpolhor3dsel_ = new uiHor3DInterpolSel( this, false );
	if ( inputhorsel_ )
	    interpolhor3dsel_->attach( alignedBelow, inputhorsel_ );

	mDynamicCastGet(EM::Horizon3D*,hor3d,hor);
	if ( hor3d )
	{
	    RowCol rc = hor3d->geometry().step();
	    BinID step( rc.row(), rc.col() );
	    interpolhor3dsel_->setStep( step );
	}
    }
    else
    {
	interpol1dsel_ = new uiArray1DInterpolSel( this, false, true );
	interpol1dsel_->setDistanceUnit( SI().xyInFeet() ?
			uiStrings::sFeet() : uiStrings::sMeter() );
	if ( inputhorsel_ )
	    interpol1dsel_->attach( alignedBelow, inputhorsel_ );
    }

    uiSeparator* sep = new uiSeparator( this, "Hor sep" );

    savefldgrp_ = new uiHorSaveFieldGrp( this, horizon_, is2d, true );
    savefldgrp_->setSaveFieldName( "Save gridded horizon" );
    if ( is2d )
    {
	sep->attach( stretchedBelow, interpol1dsel_ );
	savefldgrp_->attach( alignedBelow, interpol1dsel_ );
    }
    else
    {
	sep->attach( stretchedBelow, interpolhor3dsel_ );
	savefldgrp_->attach( alignedBelow, interpolhor3dsel_ );
    }
    savefldgrp_->attach( ensureBelow, sep );
}


uiHorizonInterpolDlg::~uiHorizonInterpolDlg()
{
    detachAllNotifiers();
    if ( horizon_ ) horizon_->unRef();
}


void uiHorizonInterpolDlg::selChangeCB( CallBacker* )
{
    if ( !inputhorsel_ || !inputhorsel_->ioobj(true) || !savefldgrp_ )
	return;

    const bool canoverwrt = EM::canOverwrite( inputhorsel_->ioobj()->key() );
    savefldgrp_->allowOverWrite( canoverwrt );
}


#define mErrRet(msg) { uiMSG().error( msg ); return false;}

bool uiHorizonInterpolDlg::interpolate3D( const IOPar& par )
{
    const BufferString method = par.find( HorizonGridder::sKeyMethod() );
    if ( method.isEmpty() )
	mErrRet( toUiString("Huh? No methods found in the paramaters") )

    PtrMan<HorizonGridder> interpolator =
				HorizonGridder::factory().create( method );
    if ( !interpolator )
	mErrRet( toUiString("Selected method not found") )

    if ( !interpolator->usePar(par) )
	mErrRet( toUiString("Incomplete parameters") )

    if ( interpolhor3dsel_->isFullSurvey() )
	savefldgrp_->setFullSurveyArray( true );

    if ( !savefldgrp_->acceptOK(nullptr) )
	return false;

    EM::Horizon* usedhor = savefldgrp_->getNewHorizon() ?
	savefldgrp_->getNewHorizon() : horizon_;

    mDynamicCastGet(EM::Horizon3D*,hor3d,usedhor)
    if ( !hor3d )
	return false;

    uiTaskRunner taskrunner( this );

    if ( !savefldgrp_->getNewHorizon() )
	hor3d->setBurstAlert( true );

    MouseCursorManager::setOverride( MouseCursor::Wait );
    uiStringSet errors;
    for ( int idx=0; idx<hor3d->geometry().nrSections(); idx++ )
    {
	Interval<int> polyinlrg( Interval<int>::udf() );
	Interval<int> polycrlrg( Interval<int>::udf() );

	bool usepolygon = false;
	if ( interpolhor3dsel_ )
	    usepolygon = interpolhor3dsel_->getPolygonRange( polyinlrg,
							     polycrlrg );

	uiRetVal rv = HorizonGridder::executeGridding(
		interpolator.ptr(), hor3d, interpolhor3dsel_->getStep(),
		usepolygon ? &polyinlrg : nullptr,
		usepolygon ? &polycrlrg : nullptr, &taskrunner );
	if ( rv.isError() )
	    errors += rv;
    }

    MouseCursorManager::restoreOverride();
    const bool success = errors.isEmpty();
    if ( !success )
    {
	const uiString firstmsg = tr("Horizon gridding failed.");
	uiMSG().errorWithDetails( errors, firstmsg );
    }

    if ( !savefldgrp_->getNewHorizon() )
	hor3d->setBurstAlert( false );

    if ( success &&
	 (saveFldGrp()->displayNewHorizon() || !saveFldGrp()->getNewHorizon()) )
	horReadyForDisplay.trigger();

    return success;
}


bool uiHorizonInterpolDlg::interpolate2D()
{
    if ( !savefldgrp_->acceptOK(nullptr) )
	return false;

    EM::Horizon* usedhor = !savefldgrp_->overwriteHorizon() ?
	savefldgrp_->getNewHorizon() : horizon_;

    mDynamicCastGet(EM::Horizon2D*,usedhor2d,usedhor)
    if ( !usedhor2d )
	return false;

    mDynamicCastGet(EM::Horizon2D*,hor2d,horizon_)
    if ( !hor2d )
	return false;

    const EM::Horizon2DGeometry& geom = hor2d->geometry();

    uiTaskRunner taskrunner( this );

    ObjectSet< Array1D<float> > arr1d;
    for ( int lineidx=0; lineidx<geom.nrLines(); lineidx++ )
	arr1d += hor2d->createArray1D( geom.geomID(lineidx) );

    interpol1dsel_->setInterpolators( geom.nrLines() );
    interpol1dsel_->setArraySet( arr1d );

    ExecutorGroup execgrp( "Interpolator", true );
    for ( int idx=0; idx<arr1d.size(); idx++ )
	execgrp.add( interpol1dsel_->getResult(idx) );

    if ( !TaskRunner::execute( &taskrunner, execgrp ) )
    {
	uiString msg = tr("Cannot interpolate horizon");
	ErrMsg( msg.getFullString() );
	return false;
    }

    for ( int idx=0; idx<arr1d.size(); idx++ )
	usedhor2d->setArray1D( *arr1d[idx], geom.geomID(idx),false);

    if ( saveFldGrp()->displayNewHorizon() || !saveFldGrp()->getNewHorizon() )
	horReadyForDisplay.trigger();

    return true;
}


bool uiHorizonInterpolDlg::acceptOK( CallBacker* )
{
    IOPar par;
    const bool isok = is2d_ ? interpol1dsel_->acceptOK()
			    : interpolhor3dsel_->fillPar( par );
    if ( !isok )
	return false;

    if ( inputhorsel_ )
    {
	const IOObj* ioobj = inputhorsel_->ioobj();
	if ( !ioobj )
	    return false;

	EM::Horizon* hor = savefldgrp_->readHorizon( ioobj->key() );

	if ( horizon_ ) horizon_->unRef();

	horizon_ = hor;
	horizon_->ref();
    }

    if ( !horizon_ )
	mErrRet( uiStrings::phrCannotFind(uiStrings::sHorizon(1)) )

    MouseCursorChanger mcc( MouseCursor::Wait );

    if ( !is2d_ )
    {
	if ( !interpolate3D(par) )
	    return false;
    }
    else
    {
	if ( !interpolate2D() )
	    return false;
    }

    const bool res = savefldgrp_->saveHorizon();
    if ( res )
    {
	finished.trigger();
	uiMSG().message( tr("Horizon successfully gridded/interpolated") );
    }

    return !inputhorsel_;
}


mImplFactory1Param(uiHor3DInterpol,uiParent*,uiHor3DInterpol::factory)

uiHor3DInterpolSel::uiHor3DInterpolSel( uiParent* p, bool musthandlefaults )
    : uiGroup(p,"Horizon3D Interpolation")
{
    methodgrps_.allowNull( true );

    uiStringSet scopes;
    scopes += tr("Full survey");
    scopes += tr("Bounding box");
    scopes += tr("Convex hull");
    scopes += tr("Only holes");
    scopes += tr("Polygon");
    filltypefld_ = new uiGenInput(this, tr("Scope"), StringListInpSpec(scopes));
    filltypefld_->setValue( 2 );
    filltypefld_->valuechanged.notify( mCB(this,uiHor3DInterpolSel,scopeChgCB));

    IOObjContext ctxt = mIOObjContext( PickSet );
    ctxt.toselect_.require_.set( sKey::Type(), sKey::Polygon() );
    polyfld_ = new uiIOObjSel( this,ctxt,
			       uiIOObjSel::Setup(uiStrings::sEmptyString()) );
    polyfld_->setCaption( uiStrings::sEmptyString() );
    polyfld_->attach( rightOf, filltypefld_ );
    polyfld_->setHSzPol( uiObject::SmallVar );

    PositionInpSpec::Setup setup;
    PositionInpSpec spec( setup );
    stepfld_ = new uiGenInput( this, tr("Inl/Crl Step"), spec );
    stepfld_->setValue( BinID(SI().inlStep(),SI().crlStep()) );
    stepfld_->attach( alignedBelow, filltypefld_ );

    uiString titletext( tr("Keep holes larger than %1")
				    .arg(SI().getUiXYUnitString()) );
    maxholeszfld_ = new uiGenInput( this, titletext, FloatInpSpec() );
    maxholeszfld_->setWithCheck( true );
    maxholeszfld_->attach( alignedBelow, stepfld_ );

    const BufferStringSet& methods = uiHor3DInterpol::factory().getNames();
    methodsel_ = new uiGenInput( this, tr("Algorithm"),
		StringListInpSpec(uiHor3DInterpol::factory().getUserNames() ) );
    methodsel_->attach( alignedBelow, maxholeszfld_ );
    methodsel_->valuechanged.notify( mCB(this,uiHor3DInterpolSel,methodSelCB) );

    for ( int idx=0; idx<methods.size(); idx++ )
    {
	uiHor3DInterpol* methodgrp = uiHor3DInterpol::factory().create(
					methods[idx]->buf(), this, true );
	if ( methodgrp )
	    methodgrp->attach( alignedBelow, methodsel_ );

	methodgrps_ += methodgrp;
    }

    setHAlignObj( methodsel_ );
    methodSelCB( nullptr );
    scopeChgCB( nullptr );
}


void uiHor3DInterpolSel::scopeChgCB( CallBacker* )
{
    const bool showpolyfld = isPolygon();
    polyfld_->display( showpolyfld );
}


void uiHor3DInterpolSel::methodSelCB( CallBacker* )
{
    const int selidx = methodsel_ ? methodsel_->getIntValue( 0 ) : 0;
    for ( int idx=0; idx<methodgrps_.size(); idx++ )
    {
	if ( methodgrps_[idx] )
	    methodgrps_[idx]->display( idx==selidx );
    }
}


BinID uiHor3DInterpolSel::getStep() const
{
    return stepfld_->getBinID();
}


void uiHor3DInterpolSel::setStep( const BinID& steps )
{
    stepfld_->setValue( steps );
}


bool uiHor3DInterpolSel::isFullSurvey() const
{
    return filltypefld_->getIntValue() == mScopeSurvey;
}


bool uiHor3DInterpolSel::isPolygon() const
{
    return filltypefld_->getIntValue() == mScopePolygon;
}


bool uiHor3DInterpolSel::cropPolygon() const
{
    return false;
}


bool uiHor3DInterpolSel::getPolygonRange( Interval<int>& inlrg,
					  Interval<int>& crlrg )
{
    if ( !isPolygon() )
	return false;

    ODPolygon<float> poly;
    if ( !readPolygon(poly) )
	return false;

    const Interval<float> xrg = poly.getRange( true );
    const Interval<float> yrg = poly.getRange( false );
    inlrg.start = mNINT32(xrg.start); inlrg.stop = mNINT32(xrg.stop);
    crlrg.start = mNINT32(yrg.start); crlrg.stop = mNINT32(yrg.stop);
    return true;
}


bool uiHor3DInterpolSel::readPolygon( ODPolygon<float>& poly ) const
{
    if ( !polyfld_ || !polyfld_->ioobj() )
	return false;

    BufferString errmsg;
    RefMan<Pick::Set> ps = new Pick::Set;
    const bool res = PickSetTranslator::retrieve(
				*ps, polyfld_->ioobj(), true, errmsg );
    if ( !res )
	mErrRet( mToUiStringTodo(errmsg) );

    for ( int idx=0; idx<ps->size(); idx++ )
    {
	const Pick::Location& pl = ps->get( idx );
	const Coord bid = SI().binID2Coord().transformBackNoSnap( pl.pos() );
	Geom::Point2D<float> pt; pt.setXY( bid.x, bid.y );
	poly.add( pt );
    }

    return true;
}



bool uiHor3DInterpolSel::fillPar( IOPar& par ) const
{
    Array2DInterpol::FillType filltype;
    const int selfilltype = filltypefld_->getIntValue();
    if ( selfilltype == mScopeSurvey || selfilltype == mScopeBBX )
	filltype = Array2DInterpol::Full;
    else if ( selfilltype == mScopeConvex )
	filltype = Array2DInterpol::ConvexHull;
    else if ( selfilltype ==mScopeHoles )
	filltype = Array2DInterpol::HolesOnly;
    else
	filltype = Array2DInterpol::Polygon;

    par.set( Array2DInterpol::sKeyFillType(), filltype );
    if ( filltype == Array2DInterpol::Polygon )
    {
	ODPolygon<float> poly;
	if ( !readPolygon(poly) )
	    return false;

	par.set( Array2DInterpol::sKeyPolyNrofNodes(), poly.size() );
	for ( int idx=0; idx<poly.size(); idx++ )
	{
	    const Geom::Point2D<float>& node = poly.getVertex(idx);
	    par.set( IOPar::compKey(Array2DInterpol::sKeyPolyNode(),idx),
		    Coord(node.x,node.y) );
	}
    }

    if ( maxholeszfld_->isChecked() )
	par.set( Array2DInterpol::sKeyMaxHoleSz(),
		 maxholeszfld_->getIntValue() );

    const BinID step = stepfld_->getBinID();
    if ( step.inl() <= 0 || step.crl() <= 0 )
    {
	uiMSG().error(
	    uiStrings::phrEnter(tr("positive integer value for steps")) );
	return false;
    }
    par.set( Array2DInterpol::sKeyRowStep(), step.inl() );
    par.set( Array2DInterpol::sKeyColStep(), step.crl() );

    const int methodidx = methodsel_->getIntValue( 0 );
    const uiHor3DInterpol* methodgrp = methodgrps_[methodidx];
    par.set( HorizonGridder::sKeyMethod(), methodgrp->factoryKeyword() );
    return methodgrp->fillPar( par );
}


bool uiHor3DInterpolSel::usePar( const IOPar& )
{
    return true;
}


uiHor3DInterpol::uiHor3DInterpol( uiParent* p )
    : uiGroup(p,"Horizon Interpolation")
{
}


const char* uiInvDistHor3DInterpol::factoryKeyword() const
{
    return InverseDistanceArray2DInterpol::sFactoryKeyword();
}


void uiInvDistHor3DInterpol::initClass()
{
    uiHor3DInterpol::factory().addCreator( create,
		InverseDistanceArray2DInterpol::sFactoryKeyword(),
		InverseDistanceArray2DInterpol::sFactoryDisplayName() );
}


uiHor3DInterpol* uiInvDistHor3DInterpol::create( uiParent* p )
{
    return new uiInvDistHor3DInterpol( p );
}


uiInvDistHor3DInterpol::uiInvDistHor3DInterpol( uiParent* p )
    : uiHor3DInterpol(p)
    , cornersfirst_(false)
    , stepsz_(1)
    , nrsteps_(mUdf(int))
{
    fltselfld_ = new uiFaultParSel( this, false );

    uiString lbltxt( tr("Search radius %1").arg( SI().getUiXYUnitString()) );
    radiusfld_ = new uiGenInput( this, lbltxt, FloatInpSpec(defaultRadius()) );
    radiusfld_->setWithCheck( true );
    radiusfld_->setChecked( true );
    radiusfld_->attach( alignedBelow, fltselfld_ );

    parbut_ = new uiPushButton( this, tr("Parameters"),
			mCB(this,uiInvDistHor3DInterpol,doParamDlg),
			false );
    parbut_->attach( rightOf, radiusfld_ );

    setHAlignObj( radiusfld_ );
}


void uiInvDistHor3DInterpol::doParamDlg( CallBacker* )
{
    uiInvDistInterpolPars dlg( this, cornersfirst_, stepsz_, nrsteps_ );
    if ( !dlg.go() ) return;

    cornersfirst_ = dlg.isCornersFirst();
    stepsz_ = dlg.stepSize();
    nrsteps_ = dlg.nrSteps();
}


bool uiInvDistHor3DInterpol::fillPar( IOPar& par ) const
{
    const float radius = radiusfld_->isChecked() ?
				radiusfld_->getFValue(0) : mUdf(float);
    if ( radius<=0 )
    {
	uiMSG().error( InverseDistanceGridder2D::searchRadiusErrMsg() );
	return false;
    }

    const TypeSet<MultiID>& selfaultids = fltselfld_->selFaultIDs();
    par.set( HorizonGridder::sKeyNrFaults(), selfaultids.size() );
    for ( int idx=0; idx<selfaultids.size(); idx++ )
	par.set( IOPar::compKey(HorizonGridder::sKeyFaultID(),idx),
		 selfaultids[idx] );

    par.set( InverseDistanceArray2DInterpol::sKeySearchRadius(), radius );
    par.set( InverseDistanceArray2DInterpol::sKeyStepSize(), stepsz_ );
    par.set( InverseDistanceArray2DInterpol::sKeyNrSteps(), nrsteps_ );

    return true;
}


bool uiInvDistHor3DInterpol::usePar( const IOPar& )
{
    return true;
}


const char* uiTriangulationHor3DInterpol::factoryKeyword() const
{
    return TriangulationArray2DInterpol::sFactoryKeyword();
}


void uiTriangulationHor3DInterpol::initClass()
{
    uiHor3DInterpol::factory().addCreator( create,
	    TriangulationArray2DInterpol::sFactoryKeyword(),
	    TriangulationArray2DInterpol::sFactoryDisplayName() );
}


uiHor3DInterpol* uiTriangulationHor3DInterpol::create( uiParent* p )
{ return new uiTriangulationHor3DInterpol( p ); }


uiTriangulationHor3DInterpol::uiTriangulationHor3DInterpol( uiParent* p )
    : uiHor3DInterpol(p)
{
    fltselfld_ = new uiFaultParSel( this, false );

    useneighborfld_ = new uiCheckBox( this, tr("Use nearest neighbor") );
    useneighborfld_->setChecked( false );
    useneighborfld_->activated.notify(
		mCB(this,uiTriangulationHor3DInterpol,useNeighborCB) );
    useneighborfld_->attach( alignedBelow, fltselfld_ );

    uiString titletext( tr("Max interpolate distance %1")
			.arg(SI().getUiXYUnitString()) );
    maxdistfld_ = new uiGenInput( this, titletext, FloatInpSpec() );
    maxdistfld_->setWithCheck( true );
    maxdistfld_->attach( alignedBelow, useneighborfld_ );

    setHAlignObj( useneighborfld_ );
    useNeighborCB( nullptr );
}


void uiTriangulationHor3DInterpol::useNeighborCB( CallBacker* )
{
    maxdistfld_->display( !useneighborfld_->isChecked() );
}


bool uiTriangulationHor3DInterpol::fillPar( IOPar& par ) const
{
    bool usemax = !useneighborfld_->isChecked() && maxdistfld_->isChecked();
    const float maxdist = maxdistfld_->getFValue();
    if ( usemax && !mIsUdf(maxdist) && maxdist<0 )
    {
	uiMSG().error( tr("Maximum distance must be > 0. ") );
	return false;
    }

    const TypeSet<MultiID>& selfaultids = fltselfld_->selFaultIDs();
    par.set( HorizonGridder::sKeyNrFaults(), selfaultids.size() );
    for ( int idx=0; idx<selfaultids.size(); idx++ )
	par.set( IOPar::compKey(HorizonGridder::sKeyFaultID(),idx),
		 selfaultids[idx] );

    par.setYN( TriangulationArray2DInterpol::sKeyDoInterpol(),
	       !useneighborfld_->isChecked() );
    if ( usemax )
	par.set( TriangulationArray2DInterpol::sKeyMaxDistance(), maxdist );

    return true;
}


bool uiTriangulationHor3DInterpol::usePar( const IOPar& )
{
    return true;
}


const char* uiExtensionHor3DInterpol::factoryKeyword() const
{
    return ExtensionArray2DInterpol::sFactoryKeyword();
}


void uiExtensionHor3DInterpol::initClass()
{
    uiHor3DInterpol::factory().addCreator( create,
	    ExtensionArray2DInterpol::sFactoryKeyword(),
	    ExtensionArray2DInterpol::sFactoryDisplayName() );
}


uiHor3DInterpol* uiExtensionHor3DInterpol::create( uiParent* p )
{ return new uiExtensionHor3DInterpol( p ); }


uiExtensionHor3DInterpol::uiExtensionHor3DInterpol( uiParent* p )
    : uiHor3DInterpol(p)
{
    nrstepsfld_ = new uiGenInput(this, tr("Number of steps"), IntInpSpec(20));
    setHAlignObj( nrstepsfld_ );
}


bool uiExtensionHor3DInterpol::fillPar( IOPar& par ) const
{
    if ( nrstepsfld_->getIntValue()<1 )
    {
	uiMSG().error( tr("Nr steps must be > 0.") );
	return false;
    }

    par.set( ExtensionArray2DInterpol::sKeyNrSteps(),
	     nrstepsfld_->getIntValue() );
    return true;
}


bool uiExtensionHor3DInterpol::usePar( const IOPar& )
{
    return true;
}


uiContinuousCurvatureHor3DInterpol::uiContinuousCurvatureHor3DInterpol(
								uiParent* p )
    : uiHor3DInterpol(p)
{
    tensionfld_ = new uiGenInput( this, tr("Tension"), FloatInpSpec(0.95f) );

    uiString lbltxt( tr("Search radius %1") .arg(SI().getUiXYUnitString()) );
    radiusfld_ = new uiGenInput( this, lbltxt, FloatInpSpec(defaultRadius()) );
    radiusfld_->attach( alignedBelow, tensionfld_ );

    setHAlignObj( radiusfld_ );
}


const char* uiContinuousCurvatureHor3DInterpol::factoryKeyword() const
{
    return ContinuousCurvatureArray2DInterpol::sFactoryKeyword();
}


void uiContinuousCurvatureHor3DInterpol::initClass()
{
    uiHor3DInterpol::factory().addCreator( create,
	ContinuousCurvatureArray2DInterpol::sFactoryKeyword(),
	ContinuousCurvatureArray2DInterpol::sFactoryDisplayName() );

}


uiHor3DInterpol* uiContinuousCurvatureHor3DInterpol::create( uiParent* p )
{
    return new uiContinuousCurvatureHor3DInterpol(p);
}


bool uiContinuousCurvatureHor3DInterpol::fillPar( IOPar& par) const
{
    if ( tensionfld_ )
	par.set( "Tension", tensionfld_->getFValue() );
    if ( radiusfld_ )
	par.set("Search Radius",radiusfld_->getFValue() );

    return true;
}


bool uiContinuousCurvatureHor3DInterpol::usePar( const IOPar& )
{
    return true;
}
