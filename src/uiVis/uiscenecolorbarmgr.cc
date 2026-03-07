/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiscenecolorbarmgr.h"

#include "notify.h"
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

    auto* wfld = new uiLabeledSpinBox( this, uiStrings::sWidth() );
    widthfld_ = wfld->box();
    widthfld_->setMaxValue( horizontal ? ColorBarBounds::maxHorWidth()
				       : ColorBarBounds::maxVertWidth() );
    widthfld_->setMinValue( horizontal ? ColorBarBounds::minHorWidth()
				       : ColorBarBounds::minVertWidth() );
    widthfld_->setValue( scenecoltab_->getSize().width() );
    mAttachCB( widthfld_->valueChanging, uiSceneColorbarMgr::sizeChangedCB );

    auto* hfld = new uiLabeledSpinBox( this, uiStrings::sHeight() );
    heightfld_ = hfld->box();
    heightfld_->setMaxValue( horizontal ? ColorBarBounds::maxHorHeight()
					: ColorBarBounds::maxVertHeight() );
    heightfld_->setMinValue( horizontal ? ColorBarBounds::minHorHeight()
					: ColorBarBounds::minVertHeight() );
    heightfld_->setValue( scenecoltab_->getSize().height() );
    mAttachCB( heightfld_->valueChanging, uiSceneColorbarMgr::sizeChangedCB );
    hfld->attach( rightOf, wfld );

    BufferStringSet positms;
    positms.add( "Left" ).add( "Right" )
	   .add( "Top" ).add( "Bottom" );
    posfld_ = new uiGenInput( this, uiStrings::sPosition(),
			      StringListInpSpec(positms) );
    posfld_->attach( alignedBelow, wfld );
    posfld_->setValue( scenecoltab_->getPos() );
    mAttachCB( posfld_->valueChanged, uiSceneColorbarMgr::posChangedCB );
}


uiSceneColorbarMgr::~uiSceneColorbarMgr()
{
    detachAllNotifiers();
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

    const int width = scenecoltab_->getSize().width();
    const int height = scenecoltab_->getSize().height();

    widthfld_->setMaxValue( horizontal ? ColorBarBounds::maxHorWidth()
				       : ColorBarBounds::maxVertWidth() );
    widthfld_->setMinValue( horizontal ? ColorBarBounds::minHorWidth()
				       : ColorBarBounds::minVertWidth() );
    heightfld_->setMaxValue( horizontal ? ColorBarBounds::maxHorHeight()
					: ColorBarBounds::maxVertHeight() );
    heightfld_->setMinValue( horizontal ? ColorBarBounds::minHorHeight()
					: ColorBarBounds::minVertHeight() );

    widthfld_->setValue( width );
    heightfld_->setValue( height );

    sizeChangedCB( nullptr );
}
