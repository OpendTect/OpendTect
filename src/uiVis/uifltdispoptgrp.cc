/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          November 2011
________________________________________________________________________

-*/

#include "uifltdispoptgrp.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "visfaultdisplay.h"


uiFaultDisplayOptGrp::uiFaultDisplayOptGrp( uiParent* p,
					    visSurvey::FaultDisplay* fd )
    : uiDlgGroup( p, tr("Construction algorithms") )
    , fltdisp_( fd )
{
    if ( !fltdisp_ )
	return;

    const char* dispopt[] =
	{ "None(default)", "In-line", "Cross-line", "Z-slice" ,0 };
    algfld_ = new uiGenInput( this, tr("Project along"),
			      StringListInpSpec(dispopt) );
    algfld_->setValue( fltdisp_->triangulateAlg() );
    algfld_->valuechanged.notify( mCB(this,uiFaultDisplayOptGrp,algChg) );

    applybut_ = new uiPushButton( this, tr("Update display now"), true );
    applybut_->attach( centeredBelow, algfld_ );
    applybut_->activated.notify( mCB(this,uiFaultDisplayOptGrp,applyCB) );
    applybut_->setSensitive( false );
}


void uiFaultDisplayOptGrp::algChg( CallBacker* )
{
    const int tri = algfld_->getIntValue();
    applybut_->setSensitive( tri!=fltdisp_->triangulateAlg() );
}


void uiFaultDisplayOptGrp::applyCB( CallBacker* )
{
    apply();
}


bool uiFaultDisplayOptGrp::apply()
{
    if ( !fltdisp_ )
	return false;

    const int tri = algfld_->getIntValue();
    fltdisp_->triangulateAlg( (mFltTriProj)tri );
    return true;
}


bool uiFaultDisplayOptGrp::acceptOK()
{ return apply(); }
