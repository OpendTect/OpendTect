/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiscenecolorbarmgr.h"

#include "notify.h"
#include "survinfo.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "visscenecoltab.h"


uiSceneColorbarMgr::uiSceneColorbarMgr( uiParent* p,
					visBase::SceneColTab* coltab )
    : uiDialog(p,Setup(tr("Color Bar Settings"),mNoHelpKey))
    , scenecoltab_(coltab)
{
    setCtrlStyle( CloseOnly );
    const bool horizontal = scenecoltab_->getOrientation();

    auto* wfld = new uiLabeledSpinBox( this, tr("Width") );
    widthfld_ = wfld->box();
    widthfld_->setMaxValue( horizontal ? 700 : 35 );
    widthfld_->setMinValue( horizontal ? 350 : 20 );
    widthfld_->setValue( scenecoltab_->getSize().width() );
    mAttachCB( widthfld_->valueChanging, uiSceneColorbarMgr::sizeChangedCB );

    auto* hfld = new uiLabeledSpinBox( this, tr("Height") );
    heightfld_ = hfld->box();
    heightfld_->setMaxValue( horizontal ? 35 : 700 );
    heightfld_->setMinValue( horizontal ? 20 : 350 );
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
    posfld_->valueChanged.notify(
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

    const bool horizontal = scenecoltab_->getOrientation();
    const int numlbls = scenecoltab_->getNumLabels();

    widthfld_->setMaxValue( horizontal ? numlbls*100 : 35 );
    widthfld_->setMinValue( horizontal ? numlbls*90 : 20 );
    heightfld_->setMaxValue( horizontal ? 35 : numlbls*100 );
    heightfld_->setMinValue( horizontal ? 20 : numlbls*90 );

    widthfld_->setValue( scenecoltab_->getSize().width() );
    heightfld_->setValue( scenecoltab_->getSize().height() );
}
