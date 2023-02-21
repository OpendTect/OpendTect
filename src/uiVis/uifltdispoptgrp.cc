/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    algfld_->valueChanged.notify( mCB(this,uiFaultDisplayOptGrp,algChg) );

    applybut_ = new uiPushButton( this, tr("Update display now"), true );
    applybut_->attach( centeredBelow, algfld_ );
    applybut_->activated.notify( mCB(this,uiFaultDisplayOptGrp,applyCB) );
    applybut_->setSensitive( false );
}


uiFaultDisplayOptGrp::~uiFaultDisplayOptGrp()
{}


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
