/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uihorinterpol.cc,v 1.13 2009-11-19 04:04:12 cvssatyaki Exp $";

#include "uihorinterpol.h"

#include "array2dinterpol.h"
#include "arraynd.h"
#include "ctxtioobj.h"
#include "datainpspec.h"
#include "executor.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "survinfo.h"

#include "uiarray2dinterpol.h"
#include "uigeninput.h"
#include "uihorsavefieldgrp.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitaskrunner.h"

uiHorizon3DInterpolDlg::uiHorizon3DInterpolDlg( uiParent* p,
						EM::Horizon3D* hor )
    : uiDialog( p, uiDialog::Setup::Setup("Horizon Gridding",
	       				  "Gridding parameters",
					  "HelpID" ) )
    , horizon_( hor )
    , inputhorsel_( 0 )
    , savefldgrp_( 0 )		       
{
    if ( horizon_ )
	horizon_->ref();
    else
    {
	IOObjContext ctxt = EMHorizon3DTranslatorGroup::ioContext();
	ctxt.forread = true;
	inputhorsel_ = new uiIOObjSel( this, ctxt );
    }

    const char* geometries[] = { "Full survey", "Bounding box",
				 "Convex hull", "Only holes", 0 };
    geometrysel_ = new uiGenInput( this, "Geometry",
	    			   StringListInpSpec( geometries ) );
    geometrysel_->setText( geometries[2] );

    if ( inputhorsel_ ) geometrysel_->attach( alignedBelow, inputhorsel_ );
    interpolsel_ = new uiArray2DInterpolSel( this, false, true, false, 0 );
    interpolsel_->setDistanceUnit( SI().xyInFeet() ? "[ft]" : "[m]" );
    interpolsel_->attach( alignedBelow, geometrysel_ );

    uiSeparator* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, interpolsel_ );

    savefldgrp_ = new uiHorSaveFieldGrp( this, horizon_ );
    savefldgrp_->setSaveFieldName( "Save gridded horizon" );
    savefldgrp_->attach( alignedBelow, interpolsel_ );
    savefldgrp_->attach( ensureBelow, sep );
}


uiHorizon3DInterpolDlg::~uiHorizon3DInterpolDlg()
{
    if ( horizon_ ) horizon_->unRef();
}


const char* uiHorizon3DInterpolDlg::helpID() const
{
    return interpolsel_->helpID();
}


#define mErrRet(msg) { if ( msg ) uiMSG().error( msg ); return false; }


bool uiHorizon3DInterpolDlg::acceptOK( CallBacker* cb )
{
    if ( !interpolsel_->acceptOK() )
	return false;

    PtrMan<Array2DInterpol> interpolator = interpolsel_->getResult();
    if ( !interpolator )
	return false;

    uiTaskRunner tr( this );

    if ( inputhorsel_ ) 
    {
	const IOObj* ioobj = inputhorsel_->ioobj();
	if ( !ioobj )
	    return false;

	EM::Horizon* hor = savefldgrp_->readHorizon( ioobj->key() );
	mDynamicCastGet( EM::Horizon3D*, hor3d, hor )
	if ( !hor3d )
	    mErrRet( "Could not load horizon" );

	if ( horizon_ ) horizon_->unRef();

	horizon_ = hor3d;
	horizon_->ref();
    }

    if ( !horizon_ )
	mErrRet( "Missing horizon!" );

    MouseCursorChanger mcc( MouseCursor::Wait );

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

    if ( !savefldgrp_->acceptOK( cb ) ) 
	return false;

    const float inldist = SI().inlDistance();
    const float crldist = SI().crlDistance();

    EM::Horizon* usedhor = savefldgrp_->getNewHorizon() ?
	savefldgrp_->getNewHorizon() : horizon_;

    mDynamicCastGet(EM::Horizon3D*,usedhor3d,usedhor)
    if ( !usedhor3d )
	return false;

    for ( int idx=0; idx<horizon_->geometry().nrSections(); idx++ )
    {
	const EM::SectionID sid = horizon_->geometry().sectionID( idx );
	PtrMan<Array2D<float> > arr = horizon_->createArray2D( sid );

	if ( !arr )
	{
	    BufferString msg( "Not enough horizon data for section " );
	    msg += sid;
	    ErrMsg( msg ); continue;
	}

	const int inlstepoutstep = horizon_->geometry().rowRange( sid ).step;
	const int crlstepoutstep = horizon_->geometry().colRange( sid ).step;

	interpolator->setRowStep( inlstepoutstep*inldist );
	interpolator->setColStep( crlstepoutstep*crldist );

	if ( !interpolator->setArray( *arr, &tr ) )
	{
	    BufferString msg( "Cannot setup interpolation on section " );
	    msg += sid;
	    ErrMsg( msg ); continue;
	}

	if ( !tr.execute( *interpolator ) )
	{
	    BufferString msg( "Cannot interpolate section " );
	    msg += sid;
	    ErrMsg( msg ); continue;
	}

	EM::SectionID usedsid = usedhor3d->geometry().sectionID( idx );;
	if ( !usedhor3d->setArray2D( *arr, usedsid, true, "Interpolation" ) )
	{
	    BufferString msg( "Cannot set new data to section " );
	    msg += usedsid;
	    ErrMsg( msg ); continue;
	}
    }

    return savefldgrp_->saveHorizon();
}


