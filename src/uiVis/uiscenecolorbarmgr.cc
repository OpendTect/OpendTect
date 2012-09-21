/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Oct 2009
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uiscenecolorbarmgr.h"

#include "uigeninput.h"
#include "uispinbox.h"
#include "visscenecoltab.h"


uiSceneColorbarMgr::uiSceneColorbarMgr( uiParent* p, 
				        visBase::SceneColTab* coltab )				        
    : uiDialog(p,uiDialog::Setup("Color Bar Properties",
				 "Change color bar position and size",mNoHelpID))
    , scenecoltab_(coltab)
{
    setCtrlStyle( LeaveOnly );

    uiLabeledSpinBox* wfld = new uiLabeledSpinBox( this, "Width" );
    widthfld_ = wfld->box();
    widthfld_->setMaxValue( 500 );
    widthfld_->setValue( scenecoltab_->getSize().width() );
    widthfld_->valueChanging.notify(
			mCB(this,uiSceneColorbarMgr,sizeChangedCB) );

    uiLabeledSpinBox* hfld = new uiLabeledSpinBox( this, "Height" );
    heightfld_ = hfld->box();
    heightfld_->setMaxValue( 1000 );
    heightfld_->setValue( scenecoltab_->getSize().height() );
    heightfld_->valueChanging.notify(
			mCB(this,uiSceneColorbarMgr,sizeChangedCB) );
    hfld->attach( rightOf, wfld );

    BufferStringSet positms;
    positms.add( "Bottom Left" ).add( "Bottom Right" )
	   .add( "Top Left" ).add( "Top Right" );
    posfld_ = new uiGenInput( this, "Position", StringListInpSpec(positms) );
    posfld_->attach( alignedBelow, wfld );
    posfld_->setValue( scenecoltab_->getPos() );
    posfld_->valuechanged.notify(
			mCB(this,uiSceneColorbarMgr,posChangedCB) );
}
	

uiSceneColorbarMgr::~uiSceneColorbarMgr()
{
}


void uiSceneColorbarMgr::sizeChangedCB( CallBacker* )
{
    scenecoltab_->setSize( widthfld_->getValue(), heightfld_->getValue() ); 
}


void uiSceneColorbarMgr::posChangedCB( CallBacker* )
{
    scenecoltab_->setPos( (visBase::SceneColTab::Pos)posfld_->getIntValue() );
}

