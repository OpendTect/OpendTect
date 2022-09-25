/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivispolygonsurfbezierdlg.h"

#include "empolygonbody.h"
#include "uigeninput.h"
#include "uibutton.h"
#include "uimsg.h"
#include "vispolygonbodydisplay.h"


uiVisPolygonSurfBezierDlg::uiVisPolygonSurfBezierDlg( uiParent* p,
	visSurvey::PolygonBodyDisplay* plg )
    : uiDlgGroup( p, tr("Shape smoothness") )
    , plg_( plg )
    , surf_( 0 )
{
    if ( !plg ) return;

    bezierfld_ = new uiGenInput( this, tr("Number of inserts") );
    if ( plg->getEMPolygonBody() )
	surf_ = plg->getEMPolygonBody()->geometry().geometryElement();
    bezierfld_->setValue( surf_ ? surf_->getBezierCurveSmoothness() : 0 );

    applybut_ = new uiPushButton( this, tr("Update Now"), true );
    applybut_->attach( centeredBelow, bezierfld_ );
    applybut_->activated.notify( mCB(this,uiVisPolygonSurfBezierDlg,applyCB) );
}


uiVisPolygonSurfBezierDlg::~uiVisPolygonSurfBezierDlg()
{}


bool uiVisPolygonSurfBezierDlg::acceptOK()
{ return apply(); }


void uiVisPolygonSurfBezierDlg::applyCB( CallBacker* )
{
    apply();
}


bool uiVisPolygonSurfBezierDlg::apply()
{
    if ( !surf_ ) return false;

    if ( bezierfld_->getIntValue()<0 )
    {
	uiMSG().error(tr("Nr of points between two"
                         " picks should be at least 0"));
	return false;
    }

    if ( bezierfld_->getIntValue()==surf_->getBezierCurveSmoothness() )
	return true;

    surf_->setBezierCurveSmoothness( bezierfld_->getIntValue() );
    plg_->touchAll( false, true );

    return true;
}
