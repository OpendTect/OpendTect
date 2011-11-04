/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          November 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uifltdispoptgrp.cc,v 1.1 2011-11-04 15:41:58 cvsyuancheng Exp $";

#include "uifltdispoptgrp.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "visfaultdisplay.h"


uiFaultDisplayOptGrp::uiFaultDisplayOptGrp( uiParent* p, 
	visSurvey::FaultDisplay* fd )
    : uiDlgGroup( p, "Construction algorithms" )
    , fltdisp_( fd )
{
    if ( !fltdisp_ )
	return;

    const char* dispopt[] = {"None(default)","Inline","Crossline","Z-slice",0};
    algfld_ = new uiGenInput(this,"Project along", StringListInpSpec(dispopt));
    algfld_->setValue( fltdisp_->triangulateAlg() );
    algfld_->valuechanged.notify( mCB(this,uiFaultDisplayOptGrp,algChg) );

    applybut_ = new uiPushButton( this, "&Update display now", true );
    applybut_->attach( centeredBelow, algfld_ );
    applybut_->activated.notify( mCB(this,uiFaultDisplayOptGrp,applyCB) );
    applybut_->setSensitive( false ); 
}


void uiFaultDisplayOptGrp::algChg( CallBacker* )
{
    const int tri = algfld_->getIntValue();
    applybut_->setSensitive( tri!=fltdisp_->triangulateAlg() ); 
}


bool uiFaultDisplayOptGrp::applyCB( CallBacker* )
{
    if ( !fltdisp_ )
	return false;

    const int tri = algfld_->getIntValue(); 
    fltdisp_->triangulateAlg( tri );
    return true;
}


bool uiFaultDisplayOptGrp::acceptOK()
{ return applyCB(0); }
