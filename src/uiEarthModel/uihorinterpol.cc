/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uihorinterpol.cc,v 1.2 2009-04-23 18:08:50 cvskris Exp $";

#include "uihorinterpol.h"

#include "array2dinterpol.h"
#include "arraynd.h"
#include "datainpspec.h"
#include "emhorizon3d.h"
#include "survinfo.h"
#include "uiarray2dinterpol.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uitaskrunner.h"

uiHorizon3DInterpolDlg::uiHorizon3DInterpolDlg( uiParent* p,
						EM::Horizon3D& hor )
    : uiDialog( p, uiDialog::Setup::Setup("Horizon Gridding",
	       				  "Gridding parameters",
					  "HelpID" ) )
    , horizon_( hor )
{
    horizon_.ref();
    const char* geometries[] = { "Full survey", "Bounding box",
				 "Convex hull", "Only holes", 0 };
    geometrysel_ = new uiGenInput( this, "Geometry",
	    			   StringListInpSpec( geometries ) );
    interpolsel_ = new uiArray2DInterpolSel( this, false, true, 0 );
    interpolsel_->setDistanceUnit( SI().xyInFeet() ? "[ft]" : "[m]" );
    interpolsel_->attach( alignedBelow, geometrysel_ );
}


uiHorizon3DInterpolDlg::~uiHorizon3DInterpolDlg()
{ horizon_.unRef(); }


bool uiHorizon3DInterpolDlg::acceptOK( CallBacker* )
{
    if ( !interpolsel_->acceptOK() )
	return false;

    PtrMan<Array2DInterpol> interpolator = interpolsel_->getResult();
    if ( !interpolator )
	return false;

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
    uiTaskRunner tr( this );

    for ( int idx=0; idx<horizon_.geometry().nrSections(); idx++ )
    {
	const EM::SectionID sid = horizon_.geometry().sectionID( idx );
	PtrMan<Array2D<float> > arr = horizon_.createArray2D( sid );

	if ( !arr )
	{
	    BufferString msg( "Not enough horizon data for section " );
	    msg += sid;
	    ErrMsg( msg ); continue;
	}

	const int inlstepoutstep =  horizon_.geometry().rowRange( sid ).step;
	const int crlstepoutstep =  horizon_.geometry().colRange( sid ).step;

	interpolator->setRowStep( inlstepoutstep*inldist );
	interpolator->setColStep( crlstepoutstep*crldist );

	if ( !interpolator->setArray( *arr ) )
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


	if ( !horizon_.setArray2D( *arr, sid, true, "Interpolation" ) )
	{
	    BufferString msg( "Cannot set new data to section " );
	    msg += sid;
	    ErrMsg( msg ); continue;
	}
    }

    return true;
}

bool uiHorizon3DInterpolDlg::expandArraysToSurvey()
{
    for ( int idx=0; idx<horizon_.geometry().nrSections(); idx++ ) 
    {
	const EM::SectionID sid = horizon_.geometry().sectionID( idx );
	StepInterval<int> rowrg = horizon_.geometry().rowRange( sid );
	StepInterval<int> colrg = horizon_.geometry().colRange( sid );

	mDynamicCastGet( Geometry::ParametricSurface*, surf,
			 horizon_.sectionGeometry( sid ) );

	const StepInterval<int> survcrlrg = SI().crlRange(true);
	while ( colrg.start-colrg.step>=survcrlrg.start )
	{
	    const int newcol = colrg.start-colrg.step;
	    surf->insertCol( newcol );
											    colrg.start = newcol;
	}

	while ( colrg.stop+colrg.step<=survcrlrg.stop )
	{
	    const int newcol = colrg.stop+colrg.step;
	    surf->insertCol( newcol );
	    colrg.stop = newcol;
	}

	const StepInterval<int> survinlrg = SI().inlRange(true);
	while ( rowrg.start-rowrg.step>=survinlrg.start )
	{
	    const int newrow = rowrg.start-rowrg.step;
	    surf->insertRow( newrow );
	    rowrg.start = newrow;
	}

	while ( rowrg.stop+rowrg.step<=survinlrg.stop )
	{
	    const int newrow = rowrg.stop+rowrg.step;
	    surf->insertRow( newrow );
	    rowrg.stop = newrow;
	}
    }

    return true;
}
