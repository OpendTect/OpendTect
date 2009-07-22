/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		April 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uihorinterpol.cc,v 1.8 2009-07-22 16:01:39 cvsbert Exp $";

#include "uihorinterpol.h"

#include "array2dinterpol.h"
#include "arraynd.h"
#include "ctxtioobj.h"
#include "datainpspec.h"
#include "executor.h"
#include "emhorizon3d.h"
#include "emsurfacetr.h"
#include "emmanager.h"
#include "survinfo.h"
#include "uiarray2dinterpol.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uitaskrunner.h"

uiHorizon3DInterpolDlg::uiHorizon3DInterpolDlg( uiParent* p,
						EM::Horizon3D* hor )
    : uiDialog( p, uiDialog::Setup::Setup("Horizon Gridding",
	       				  "Gridding parameters",
					  "HelpID" ) )
    , horizon_( hor )
    , inputhorsel_( 0 )
    , savefld_( 0 )
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
    if ( inputhorsel_ ) geometrysel_->attach( alignedBelow, inputhorsel_ );
    interpolsel_ = new uiArray2DInterpolSel( this, false, true, false, 0 );
    interpolsel_->setDistanceUnit( SI().xyInFeet() ? "[ft]" : "[m]" );
    interpolsel_->attach( alignedBelow, geometrysel_ );

    if ( horizon_ )
    {
	savefld_ = new uiGenInput( this, "Store interpolated horizon",
				    BoolInpSpec(true) );
	savefld_->attach( alignedBelow, interpolsel_ );
	savefld_->valuechanged.notify(
		mCB(this,uiHorizon3DInterpolDlg,saveChangeCB));
    }

    IOObjContext ctxt = EMHorizon3DTranslatorGroup::ioContext();
    ctxt.forread = false;
    outputfld_ = new uiIOObjSel( this, ctxt, "Output Horizon" );
    outputfld_->attach( alignedBelow, savefld_
	    ? (uiParent*) savefld_ : (uiParent*) interpolsel_ );

    saveChangeCB( 0 );
}


uiHorizon3DInterpolDlg::~uiHorizon3DInterpolDlg()
{ if ( horizon_ ) horizon_->unRef(); }


const char* uiHorizon3DInterpolDlg::helpID() const
{
    return interpolsel_->helpID();
}


void uiHorizon3DInterpolDlg::saveChangeCB( CallBacker* )
{
    outputfld_->display( savefld_ ? savefld_->getBoolValue() : true );
}


bool uiHorizon3DInterpolDlg::acceptOK( CallBacker* )
{
    if ( !interpolsel_->acceptOK() )
	return false;

    PtrMan<Array2DInterpol> interpolator = interpolsel_->getResult();
    if ( !interpolator )
	return false;


    const IOObj* outputioobj = 0;
    if ( !savefld_ || savefld_->getBoolValue() )
    {
	outputioobj = outputfld_->ioobj();
	if ( !outputioobj )
	    return false;
    }

    uiTaskRunner tr( this );

    if ( inputhorsel_ )
    {
	const IOObj* ioobj = inputhorsel_->ioobj();
	if ( !ioobj )
	    return false;

	RefMan<EM::EMObject> obj =
	    EM::EMM().loadIfNotFullyLoaded( ioobj->key(), &tr );
	mDynamicCastGet( EM::Horizon3D*, hor, obj.ptr() );
	if ( !hor )
	{
	    uiMSG().error("Could not load horizon");
	    return false;
	}

	if ( horizon_ )
	    horizon_->unRef();

	horizon_ = hor;
	horizon_->ref();
    }

    if ( !horizon_ )
    {
	pErrMsg("Missing horizon!");
	return false;
    }

    MouseCursorChanger mcc( MouseCursor::Wait );

    Array2DInterpol::FillType filltype;
    switch ( geometrysel_->getIntValue() )
    {
	case 0:
	    filltype = Array2DInterpol::Full;
	    if ( !expandArraysToSurvey() )
	    {
		uiMSG().error("Cannot allocate memory for extrapolation");
		return false;
	    }
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


    const float inldist = SI().inlDistance();
    const float crldist = SI().crlDistance();

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

	const int inlstepoutstep =  horizon_->geometry().rowRange( sid ).step;
	const int crlstepoutstep =  horizon_->geometry().colRange( sid ).step;

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


	if ( !horizon_->setArray2D( *arr, sid, true, "Interpolation" ) )
	{
	    BufferString msg( "Cannot set new data to section " );
	    msg += sid;
	    ErrMsg( msg ); continue;
	}
    }

    if ( outputioobj )
    {
	horizon_->setMultiID( outputioobj->key() );
	PtrMan<Executor> exec = horizon_->saver();
	if ( !tr.execute( *exec ) )
	{
	    uiMSG().error("Could not save horizon");
	    return false;
	}
    }

    return true;
}

bool uiHorizon3DInterpolDlg::expandArraysToSurvey()
{
    for ( int idx=0; idx<horizon_->geometry().nrSections(); idx++ ) 
    {
	const EM::SectionID sid = horizon_->geometry().sectionID( idx );
	StepInterval<int> rowrg = horizon_->geometry().rowRange( sid );
	StepInterval<int> colrg = horizon_->geometry().colRange( sid );

	mDynamicCastGet( Geometry::ParametricSurface*, surf,
			 horizon_->sectionGeometry( sid ) );

	const StepInterval<int> survcrlrg = SI().crlRange(true);
	int nrcolstoinsert = -colrg.nearestIndex(survcrlrg.start);
	if ( nrcolstoinsert>0 )
	{
	    surf->insertCol( survcrlrg.start, nrcolstoinsert );
	    colrg.start = survcrlrg.start;
	}

	nrcolstoinsert = (survcrlrg.stop-colrg.stop)/colrg.step;
	if ( nrcolstoinsert>0 )
	{
	    surf->insertCol( colrg.stop+colrg.step, nrcolstoinsert );
	    colrg.stop += nrcolstoinsert*colrg.step;
	}

	const StepInterval<int> survinlrg = SI().inlRange(true);

	int nrrowstoinsert = -rowrg.nearestIndex(survinlrg.start);
	if ( nrrowstoinsert>0 )
	{
	    surf->insertRow( survinlrg.start, nrrowstoinsert );
	    rowrg.start = survinlrg.start;
	}

	nrrowstoinsert = (survinlrg.stop-rowrg.stop)/rowrg.step;
	if ( nrrowstoinsert>0 )
	{
	    surf->insertRow( rowrg.stop+rowrg.step, nrrowstoinsert );
	    rowrg.stop += nrrowstoinsert*rowrg.step;
	}
    }

    return true;
}
