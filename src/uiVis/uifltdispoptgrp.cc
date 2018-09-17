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
#include "uilabel.h"
#include "visfaultdisplay.h"

#include "envvars.h"

uiFaultDisplayOptGrp::uiFaultDisplayOptGrp( uiParent* p,
					    visSurvey::FaultDisplay* fd )
    : uiDlgGroup(p,tr("Display"))
    , algfld_(0)
    , applybut_(0)
    , fltdisp_(0)
{
    CallBack cb = mCB(this,uiFaultDisplayOptGrp,dispChg);
    uiLabel* lbl1 = new uiLabel( this, tr("Display only at") );
    sectionsfld_ = new uiCheckBox( this, tr("Sections"), cb );
    horizonsfld_ = new uiCheckBox( this, uiStrings::sHorizon(1), cb );
    lbl1->attach( leftOf, sectionsfld_ );
    horizonsfld_->attach( rightTo, sectionsfld_ );

    uiLabel* lbl2 = new uiLabel( this, tr("Display Fault") );
    planesfld_ = new uiCheckBox( this, tr("Planes"), cb );
    sticksfld_ = new uiCheckBox( this, tr("Sticks"), cb );
    nodesfld_ = new uiCheckBox( this, tr("Nodes"), cb );
    lbl2->attach( leftOf, planesfld_ );
    planesfld_->attach( alignedBelow, sectionsfld_ );
    sticksfld_->attach( rightTo, planesfld_ );
    nodesfld_->attach( rightTo, sticksfld_ );

    setFaultDisplay( fd );

    if ( GetEnvVarYN("USE_FAULT_RETRIANGULATION") )
    {
	const char* dispopt[] =
		{ "None(default)", "In-line", "Cross-line", "Z-slice" ,0 };
	algfld_ = new uiGenInput( this, tr("Project along"),
				  StringListInpSpec(dispopt) );
	if ( fd )
	    algfld_->setValue( fd->triangulateAlg() );
	algfld_->valuechanged.notify( mCB(this,uiFaultDisplayOptGrp,algChg) );
	algfld_->attach( alignedBelow, planesfld_ );

	applybut_ = new uiPushButton( this, tr("Update display now"), true );
	applybut_->attach( centeredBelow, algfld_ );
	applybut_->activated.notify( mCB(this,uiFaultDisplayOptGrp,applyCB) );
	applybut_->setSensitive( false );
    }
}


void uiFaultDisplayOptGrp::setFaultDisplay( visSurvey::FaultDisplay* fd )
{
    fltdisp_ = fd;
    if ( !fltdisp_ )
	return;

    NotifyStopper ns1( sectionsfld_->activated );
    NotifyStopper ns2( horizonsfld_->activated );
    NotifyStopper ns3( planesfld_->activated );
    NotifyStopper ns4( sticksfld_->activated );

    sectionsfld_->setChecked( fltdisp_->areIntersectionsDisplayed() );
    horizonsfld_->setChecked( fltdisp_->areHorizonIntersectionsDisplayed() );
    planesfld_->setChecked( fltdisp_->arePanelsDisplayed() );
    sticksfld_->setChecked( fltdisp_->areSticksDisplayed() );
}


void uiFaultDisplayOptGrp::algChg( CallBacker* )
{
    if ( !fltdisp_ || !algfld_ )
	return;

    const int tri = algfld_->getIntValue();
    applybut_->setSensitive( tri!=fltdisp_->triangulateAlg() );
}


void uiFaultDisplayOptGrp::applyCB( CallBacker* )
{
    apply();
}


bool uiFaultDisplayOptGrp::apply()
{
    if ( !fltdisp_ || !algfld_ )
	return true;

    const int tri = algfld_->getIntValue();
    fltdisp_->triangulateAlg( (mFltTriProj)tri );
    return true;
}


void uiFaultDisplayOptGrp::dispChg( CallBacker* )
{
    if ( !fltdisp_ )
	return;

    fltdisp_->displayIntersections( sectionsfld_->isChecked() );
    fltdisp_->displayHorizonIntersections( horizonsfld_->isChecked() );
    fltdisp_->display( sticksfld_->isChecked(), planesfld_->isChecked() );
}


bool uiFaultDisplayOptGrp::acceptOK()
{
    return apply();
}
