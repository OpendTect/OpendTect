/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author: 	Yuancheng Liu
 Date: 		Feb 2009
___________________________________________________________________

-*/
static const char* rcsID = "$Id: uivispolygonsurfbezierdlg.cc,v 1.2 2009/07/22 16:01:43 cvsbert Exp $";

#include "uivispolygonsurfbezierdlg.h"

#include "empolygonbody.h"
#include "uigeninput.h"
#include "uibutton.h"
#include "uimsg.h"
#include "vispolygonbodydisplay.h"


uiVisPolygonSurfBezierDlg::uiVisPolygonSurfBezierDlg( uiParent* p,
	visSurvey::PolygonBodyDisplay* plg )
    : uiDlgGroup( p, "Shape smoothness" )
    , plg_( plg )
    , surf_( 0 )		 
{
    if ( !plg ) return;

    bezierfld_ = new uiGenInput( this, "Number of inserts" );
    if ( plg->getEMPolygonBody() )
	surf_ = plg->getEMPolygonBody()->geometry().sectionGeometry(0);
    bezierfld_->setValue( surf_ ? surf_->getBezierCurveSmoothness() : 0 );
    
    applybut_ = new uiPushButton( this, "&Update Now", true );
    applybut_->attach( centeredBelow, bezierfld_ );
    applybut_->activated.notify( mCB(this,uiVisPolygonSurfBezierDlg,applyCB) );
}


bool uiVisPolygonSurfBezierDlg::acceptOK()
{ return applyCB(0); }


bool uiVisPolygonSurfBezierDlg::applyCB( CallBacker* )
{
    if ( !surf_ ) return false;

    if ( bezierfld_->getIntValue()<0 )
    {
	uiMSG().error("Nr of points between two picks should be at least 0");
	return false;
    }

    if ( bezierfld_->getIntValue()==surf_->getBezierCurveSmoothness() )
	return true;

    surf_->setBezierCurveSmoothness( bezierfld_->getIntValue() );
    plg_->touchAll( false, true );

    return true;
}

