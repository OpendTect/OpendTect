/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Oct 2009
________________________________________________________________________

-*/

#include "uiscenecolorbarmgr.h"

#include "uigeninput.h"
#include "uispinbox.h"
#include "visscenecoltab.h"


uiSceneColorbarMgr::uiSceneColorbarMgr( uiParent* p,
					visBase::SceneColTab* coltab )
    : uiDialog(p,uiDialog::Setup(tr("Color Bar Settings"),
                                 mNoDlgTitle,mNoHelpKey))
    , scenecoltab_(coltab)
{
    setCtrlStyle( CloseOnly );

    uiLabeledSpinBox* wfld = new uiLabeledSpinBox( this, tr("Width") );
    widthfld_ = wfld->box();
    widthfld_->setMaxValue( 500 );
    widthfld_->setValue( scenecoltab_->getSize().width() );
    widthfld_->valueChanging.notify(
			mCB(this,uiSceneColorbarMgr,sizeChangedCB) );

    uiLabeledSpinBox* hfld = new uiLabeledSpinBox( this, tr("Height") );
    heightfld_ = hfld->box();
    heightfld_->setMaxValue( 1000 );
    heightfld_->setValue( scenecoltab_->getSize().height() );
    heightfld_->valueChanging.notify(
			mCB(this,uiSceneColorbarMgr,sizeChangedCB) );
    hfld->attach( rightOf, wfld );

    BufferStringSet positms;
    positms.add( "Left" ).add( "Right" )
	   .add( "Top" ).add( "Bottom" );
    posfld_ = new uiGenInput( this, tr("Position"), StringListInpSpec(positms));
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
    scenecoltab_->setSize( widthfld_->getIntValue(),
			   heightfld_->getIntValue() );
}


void uiSceneColorbarMgr::posChangedCB( CallBacker* )
{
    scenecoltab_->setPos( (visBase::SceneColTab::Pos)posfld_->getIntValue() );
    NotifyStopper ns1( widthfld_->valueChanging );
    NotifyStopper ns2( heightfld_->valueChanging );
    widthfld_->setValue( scenecoltab_->getSize().width() );
    heightfld_->setValue( scenecoltab_->getSize().height() );
}
