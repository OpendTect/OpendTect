/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2009
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

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
#include "survinfo.h"

#include "uiarray1dinterpol.h"
#include "uiarray2dinterpol.h"
#include "uigeninput.h"
#include "uihorsavefieldgrp.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitaskrunner.h"

uiHorizonInterpolDlg::uiHorizonInterpolDlg( uiParent* p, EM::Horizon* hor,
					    bool is2d )
    : uiDialog( p, uiDialog::Setup("Horizon Gridding","Gridding parameters",
				   "104.0.16").modal(true) )
    , horizon_( hor )
    , is2d_( is2d )
    , inputhorsel_( 0 )
    , interpol2dsel_( 0 )
    , interpol1dsel_( 0 )
    , savefldgrp_( 0 )
    , finished(this)
{
    setCtrlStyle( DoAndStay );

    if ( horizon_ )
	horizon_->ref();
    else
    {
	IOObjContext ctxt = is2d ? EMHorizon2DTranslatorGroup::ioContext()
	    			 : EMHorizon3DTranslatorGroup::ioContext();
	ctxt.forread = true;
	inputhorsel_ = new uiIOObjSel( this, ctxt );
    }

    if ( !is2d )
    {
	const char* geometries[] = { "Full survey", "Bounding box",
				     "Convex hull", "Only holes", 0 };
	geometrysel_ = new uiGenInput( this, "Geometry",
				       StringListInpSpec( geometries ) );
	geometrysel_->setText( geometries[2] );

	if ( inputhorsel_ ) geometrysel_->attach( alignedBelow, inputhorsel_ );
	interpol2dsel_ =
	    new uiArray2DInterpolSel( this, false, true, false, 0, true);
	interpol2dsel_->setDistanceUnit( SI().xyInFeet() ? "[ft]" : "[m]" );
	interpol2dsel_->attach( alignedBelow, geometrysel_ );
	mDynamicCastGet(EM::Horizon3D*,hor3d,hor);
	if ( hor3d )
	{
	    RowCol rc = hor3d->geometry().step();
	    BinID step( rc.row, rc.col );
	    interpol2dsel_->setStep( step );
	}
    }
    else
    {
	interpol1dsel_ = new uiArray1DInterpolSel( this, false, true );
	interpol1dsel_->setDistanceUnit( SI().xyInFeet() ? "[ft]" : "[m]" );
	if ( inputhorsel_ )
	    interpol1dsel_->attach( alignedBelow, inputhorsel_ );
    }

    uiSeparator* sep = new uiSeparator( this, "Hor sep" );

    savefldgrp_ = new uiHorSaveFieldGrp( this, horizon_, is2d );
    savefldgrp_->setSaveFieldName( "Save gridded horizon" );
    if ( is2d )
    {
	sep->attach( stretchedBelow, interpol1dsel_ );
	savefldgrp_->attach( alignedBelow, interpol1dsel_ );
    }
    else
    {
	sep->attach( stretchedBelow, interpol2dsel_ );
	savefldgrp_->attach( alignedBelow, interpol2dsel_ );
    }
    savefldgrp_->attach( ensureBelow, sep );
}


uiHorizonInterpolDlg::~uiHorizonInterpolDlg()
{
    if ( horizon_ ) horizon_->unRef();
}


const char* uiHorizonInterpolDlg::helpID() const
{
    return uiDialog::helpID();
}


#define mErrRet(msg) { if ( msg ) uiMSG().error( msg ); return false; }

bool uiHorizonInterpolDlg::interpolate3D()
{
    PtrMan<Array2DInterpol> interpolator = interpol2dsel_->getResult();
    if ( !interpolator )
	return false;

    Array2DInterpol::FillType filltype;
    switch ( geometrysel_->getIntValue() )
    {
	case 0:
	    filltype = Array2DInterpol::Full;
	    savefldgrp_->setFullSurveyArray( true );
	    break;
	case 1:
	    filltype = Array2DInterpol::Full;
	    break;
	case 2:
	    filltype = Array2DInterpol::ConvexHull;
	    break;
	default:
	    filltype = Array2DInterpol::HolesOnly;
    }

    interpolator->setFillType( filltype );

    if ( !savefldgrp_->acceptOK(0) ) 
	return false;

    EM::Horizon* usedhor = savefldgrp_->getNewHorizon() ?
	savefldgrp_->getNewHorizon() : horizon_;

    mDynamicCastGet(EM::Horizon3D*,usedhor3d,usedhor)
    if ( !usedhor3d )
	return false;
    
    mDynamicCastGet(EM::Horizon3D*,hor3d,horizon_)
    if ( !hor3d )
	return false;

    uiTaskRunner tr( this );

    bool success = false;
    for ( int idx=0; idx<hor3d->geometry().nrSections(); idx++ )
    {
	const EM::SectionID sid = hor3d->geometry().sectionID( idx );

	BinID steps = interpol2dsel_->getStep();
	StepInterval<int> rowrg = hor3d->geometry().rowRange( sid );
	rowrg.step = steps.inl;
	StepInterval<int> colrg = hor3d->geometry().colRange();
	colrg.step = steps.crl;
	
	interpolator->setRowStep( SI().inlDistance()*steps.inl );
	interpolator->setColStep( SI().crlDistance()*steps.crl );
	
	HorSampling hs( false );
	hs.set( rowrg, colrg );

	Array2DImpl<float>* arr =
	    new Array2DImpl<float>( hs.nrInl(), hs.nrCrl() );
	if ( !arr->isOK() )
	{
	    BufferString msg( "Not enough horizon data for section " );
	    msg += sid;
	    ErrMsg( msg ); continue;
	}

        arr->setAll( mUdf(float) );	

	PtrMan<EM::EMObjectIterator> iterator = hor3d->createIterator( sid );
	if ( !iterator )
	    continue;

	while( true )
	{
	    EM::PosID posid = iterator->next();
	    if ( posid.objectID() == -1 )
		break;
	    BinID bid = posid.getRowCol();
	    if ( hs.includes(bid) )
	    {
		Coord3 pos = hor3d->getPos( posid );
		arr->set( hs.inlIdx(bid.inl), hs.crlIdx(bid.crl), (float) pos.z );
	    }
	}

	if ( !interpolator->setArray(*arr,&tr) )
	{
	    BufferString msg( "Cannot setup interpolation on section " );
	    msg += sid;
	    ErrMsg( msg ); continue;
	}

	if ( !tr.execute(*interpolator) )
	{
	    BufferString msg( "Cannot interpolate section " );
	    msg += sid;
	    ErrMsg( msg ); continue;
	}

	success = true;
	usedhor3d->geometry().sectionGeometry(sid)->setArray(
					    hs.start, hs.step, arr, true );
    }

    return success;
}


bool uiHorizonInterpolDlg::interpolate2D()
{
    if ( !savefldgrp_->acceptOK(0) )
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
	
    uiTaskRunner tr( this );

    for ( int isect=0; isect<geom.nrSections(); isect++ )
    {
	ObjectSet< Array1D<float> > arr1d;
	const EM::SectionID sid = geom.sectionID( isect );
	for ( int lineidx=0; lineidx<geom.nrLines(); lineidx++ )
	    arr1d += hor2d->createArray1D( sid, geom.lineGeomID(lineidx) );

	interpol1dsel_->setInterpolators( geom.nrLines() );
	interpol1dsel_->setArraySet( arr1d );
	
	ExecutorGroup execgrp( "Interpolator", true );
	for ( int idx=0; idx<arr1d.size(); idx++ )
	    execgrp.add( interpol1dsel_->getResult(idx) );

	if ( !tr.execute( execgrp ) )
	{
	    BufferString msg( "Cannot interpolate section " );
	    msg += sid;
	    ErrMsg( msg ); continue;
	}

	for ( int idx=0; idx<arr1d.size(); idx++ )
	    usedhor2d->setArray1D( *arr1d[idx], sid,geom.lineGeomID(idx),false);
    }

    return true;
}


bool uiHorizonInterpolDlg::acceptOK( CallBacker* cb )
{
    const bool isok = is2d_ ? interpol1dsel_->acceptOK()
			    : interpol2dsel_->acceptOK();
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
	mErrRet( "Missing horizon!" );

    MouseCursorChanger mcc( MouseCursor::Wait );
    
    if ( !is2d_ )
    {
	if ( !interpolate3D() )
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
	uiMSG().message( "Horizon successfully gridded/interpolated" );
    }

    return false;
}
