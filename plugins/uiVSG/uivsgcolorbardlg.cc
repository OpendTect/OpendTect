/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivsgcolorbardlg.h"

#include "bufstringset.h"
#include "datainpspec.h"
#include "notify.h"

#include "uigeninput.h"
#include "uispinbox.h"
#include "uistrings.h"
#include "uivsgviewer.h"

#include "visvsgscenecoltab.h"


uiVSGColorBarDlg::uiVSGColorBarDlg( uiParent* p, uiVSGViewer& viewer )
    : uiDialog(p,Setup(tr("Color Bar Settings"),mNoHelpKey))
    , viewer_(viewer)
{
    setCtrlStyle( CloseOnly );

    auto* wfld = new uiLabeledSpinBox( this, uiStrings::sWidth() );
    widthfld_ = wfld->box();
    auto* hfld = new uiLabeledSpinBox( this, uiStrings::sHeight() );
    heightfld_ = hfld->box();
    hfld->attach( rightOf, wfld );

    BufferStringSet positms;
    positms.add( "Left" ).add( "Top" ).add( "Right" ).add( "Bottom" );
    posfld_ = new uiGenInput( this, uiStrings::sPosition(),
			      StringListInpSpec(positms) );
    posfld_->attach( alignedBelow, wfld );
    posfld_->setValue( sCast(int,viewer_.getColorBarPos()) );

    setSizeLimits();
    const Geom::Size2D<int> size = viewer_.getColorBarSize();
    widthfld_->setValue( size.width() );
    heightfld_->setValue( size.height() );
    mAttachCB( widthfld_->valueChanging, uiVSGColorBarDlg::sizeChangedCB );
    mAttachCB( heightfld_->valueChanging, uiVSGColorBarDlg::sizeChangedCB );
    mAttachCB( posfld_->valueChanged, uiVSGColorBarDlg::posChangedCB );
}


uiVSGColorBarDlg::~uiVSGColorBarDlg()
{
    detachAllNotifiers();
}


void uiVSGColorBarDlg::sizeChangedCB( CallBacker* )
{
    viewer_.setColorBarSize( widthfld_->getIntValue(),
			     heightfld_->getIntValue() );
}


void uiVSGColorBarDlg::posChangedCB( CallBacker* )
{
    viewer_.setColorBarPos(
		sCast(ODVsg::SceneColTab::Pos,posfld_->getIntValue()) );
    NotifyStopper widthstopper( widthfld_->valueChanging );
    NotifyStopper heightstopper( heightfld_->valueChanging );
    const Geom::Size2D<int> size = viewer_.getColorBarSize();
    setSizeLimits();
    widthfld_->setValue( size.width() );
    heightfld_->setValue( size.height() );
    sizeChangedCB( nullptr );
}


void uiVSGColorBarDlg::setSizeLimits()
{
    const int pos = posfld_->getIntValue();
    const bool horizontal = pos==ODVsg::SceneColTab::Top ||
			    pos==ODVsg::SceneColTab::Bottom;
    widthfld_->setMaxValue( horizontal
			    ? ODVsg::ColorBarBounds::maxHorWidth()
			    : ODVsg::ColorBarBounds::maxVertWidth() );
    widthfld_->setMinValue( horizontal
			    ? ODVsg::ColorBarBounds::minHorWidth()
			    : ODVsg::ColorBarBounds::minVertWidth() );
    heightfld_->setMaxValue( horizontal
			     ? ODVsg::ColorBarBounds::maxHorHeight()
			     : ODVsg::ColorBarBounds::maxVertHeight() );
    heightfld_->setMinValue( horizontal
			     ? ODVsg::ColorBarBounds::minHorHeight()
			     : ODVsg::ColorBarBounds::minVertHeight() );
}
