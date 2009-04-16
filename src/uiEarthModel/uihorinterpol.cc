/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		April 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uihorinterpol.cc,v 1.1 2009-04-16 20:22:32 cvskris Exp $";

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
    };


    interpolator->setFillType( filltype );

    const float inldist = SI().inlRange(true).step*SI().inlDistance();
    const float crldist = SI().crlRange(true).step*SI().crlDistance();
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

	if ( !interpolator->setArray( *arr ) )
	{
	    BufferString msg( "Cannot setup interpolation on section " );
	    msg += sid;
	    ErrMsg( msg ); continue;
	}

	const int inlstepoutstep =  horizon_.geometry().rowRange( sid ).step;
	const int crlstepoutstep =  horizon_.geometry().colRange( sid ).step;

	interpolator->setRowStep( inlstepoutstep*inldist );
	interpolator->setColStep( crlstepoutstep*crldist );
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
    MouseCursorChanger chgr( MouseCursor::Wait );
    //TODO
    return true;
}
