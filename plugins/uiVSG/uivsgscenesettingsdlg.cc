/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivsgscenesettingsdlg.h"

#include "visvsgscenecoltab.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uiseparator.h"
#include "uispinbox.h"
#include "uistrings.h"
#include "uivsgviewer.h"

#include "bufstringset.h"
#include "datainpspec.h"
#include "notify.h"


uiVSGSceneSettingsDlg::uiVSGSceneSettingsDlg( uiParent* p,
					      uiVSGViewer& viewer )
    : uiDialog(p,Setup(tr("VSG Scene Settings"),mNoHelpKey))
    , viewer_(viewer)
    , hadaxes_(viewer.isAxisShown())
    , hadmodernaxes_(viewer.isModernAxes())
    , hadwheelmode_(int(viewer.getWheelDisplayMode()))
    , hadwheelgrow_(viewer.isWheelsGrowOnHover())
    , hadglasswheels_(viewer.isGlassWheels())
    , hadcolorbar_(viewer.isColorBarShown())
    , hadcolorbarpos_(int(viewer.getColorBarPos()))
    , hadcolorbarwidth_(viewer.getColorBarSize().width())
    , hadcolorbarheight_(viewer.getColorBarSize().height())
{
    auto* axeslbl = new uiLabel( this, tr("Axes") );
    axeslbl->attach( hCentered );

    axesfld_ = new uiCheckBox( this, tr("Show axes") );
    axesfld_->setChecked( hadaxes_ );
    mAttachCB( axesfld_->activated, uiVSGSceneSettingsDlg::axesCB );
    axesfld_->attach( ensureBelow, axeslbl );

    BufferStringSet axesstyles;
    axesstyles.add( "Classic" ).add( "Modern" );
    axesstylefld_ = new uiGenInput( this, tr("Axes style"),
				    StringListInpSpec(axesstyles) );
    axesstylefld_->setValue( hadmodernaxes_ ? 1 : 0 );
    mAttachCB( axesstylefld_->valueChanged, uiVSGSceneSettingsDlg::axesCB );
    axesstylefld_->attach( alignedBelow, axesfld_ );

    auto* axessep = new uiSeparator( this, "Axes separator" );
    axessep->attach( stretchedBelow, axesstylefld_ );

    auto* wheelslbl = new uiLabel( this, tr("Wheels") );
    wheelslbl->attach( hCentered );
    wheelslbl->attach( ensureBelow, axessep );

    BufferStringSet wheelmodes;
    wheelmodes.add( "Hide" ).add( "Always show" ).add( "Show on mouse hover" );
    wheelmodefld_ = new uiGenInput( this, tr("Zoom/Rotate wheels display"),
				    StringListInpSpec(wheelmodes) );
    wheelmodefld_->setValue( hadwheelmode_ );
    mAttachCB( wheelmodefld_->valueChanged,
	       uiVSGSceneSettingsDlg::wheelModeCB );
    wheelmodefld_->attach( ensureBelow, wheelslbl );
    wheelmodefld_->attach( alignedWith, axesstylefld_ );

    wheelgrowfld_ = new uiCheckBox( this, tr("Enlarge wheels on mouse hover") );
    wheelgrowfld_->setChecked( hadwheelgrow_ );
    mAttachCB( wheelgrowfld_->activated,
	       uiVSGSceneSettingsDlg::wheelOptionsCB );
    wheelgrowfld_->attach( alignedBelow, wheelmodefld_ );

    BufferStringSet wheellooks;
    wheellooks.add( "Classic" ).add( "Glass roller" );
    wheellookfld_ = new uiGenInput( this, tr("Wheel look"),
				    StringListInpSpec(wheellooks) );
    wheellookfld_->setValue( hadglasswheels_ ? 1 : 0 );
    mAttachCB( wheellookfld_->valueChanged,
	       uiVSGSceneSettingsDlg::wheelOptionsCB );
    wheellookfld_->attach( alignedBelow, wheelgrowfld_ );

    auto* wheelssep = new uiSeparator( this, "Wheels separator" );
    wheelssep->attach( stretchedBelow, wheellookfld_ );

    auto* colorbarlbl = new uiLabel( this, tr("Color bar") );
    colorbarlbl->attach( hCentered );
    colorbarlbl->attach( ensureBelow, wheelssep );

    colorbarfld_ = new uiCheckBox( this, tr("Show color bar") );
    colorbarfld_->setChecked( hadcolorbar_ );
    mAttachCB( colorbarfld_->activated, uiVSGSceneSettingsDlg::colorBarCB );
    colorbarfld_->attach( ensureBelow, colorbarlbl );
    colorbarfld_->attach( alignedWith, wheellookfld_ );

    BufferStringSet colorbarpositions;
    colorbarpositions.add( "Left" ).add( "Top" ).add( "Right" ).add( "Bottom" );
    colorbarposfld_ = new uiGenInput( this, uiStrings::sPosition(),
				      StringListInpSpec(colorbarpositions) );
    colorbarposfld_->setValue( hadcolorbarpos_ );
    mAttachCB( colorbarposfld_->valueChanged,
	       uiVSGSceneSettingsDlg::colorBarPosCB );
    colorbarposfld_->attach( alignedBelow, colorbarfld_ );

    auto* widthfld = new uiLabeledSpinBox( this, uiStrings::sWidth() );
    colorbarwidthfld_ = widthfld->box();
    auto* heightfld = new uiLabeledSpinBox( this, uiStrings::sHeight() );
    colorbarheightfld_ = heightfld->box();
    setColorBarSizeLimits();
    colorbarwidthfld_->setValue( hadcolorbarwidth_ );
    colorbarheightfld_->setValue( hadcolorbarheight_ );
    mAttachCB( colorbarwidthfld_->valueChanging,
	       uiVSGSceneSettingsDlg::colorBarSizeCB );
    mAttachCB( colorbarheightfld_->valueChanging,
	       uiVSGSceneSettingsDlg::colorBarSizeCB );
    widthfld->attach( alignedBelow, colorbarposfld_ );
    heightfld->attach( rightOf, widthfld );

    updateSensitivity();
}


uiVSGSceneSettingsDlg::~uiVSGSceneSettingsDlg()
{
    detachAllNotifiers();
}


void uiVSGSceneSettingsDlg::axesCB( CallBacker* )
{
    viewer_.showRotAxis( axesfld_->isChecked() );
    viewer_.setModernAxes( axesstylefld_->getIntValue() == 1 );
    updateSensitivity();
}


void uiVSGSceneSettingsDlg::wheelModeCB( CallBacker* )
{
    viewer_.setWheelDisplayMode(
		uiVSGViewer::WheelMode(wheelmodefld_->getIntValue()) );
    updateSensitivity();
}


void uiVSGSceneSettingsDlg::wheelOptionsCB( CallBacker* )
{
    viewer_.setWheelsGrowOnHover( wheelgrowfld_->isChecked() );
    viewer_.setGlassWheels( wheellookfld_->getIntValue() == 1 );
}


void uiVSGSceneSettingsDlg::colorBarCB( CallBacker* )
{
    viewer_.showColorBar( colorbarfld_->isChecked() );
    updateSensitivity();
}


void uiVSGSceneSettingsDlg::colorBarPosCB( CallBacker* )
{
    viewer_.setColorBarPos(
		dGBVis::SceneColTab::Pos(colorbarposfld_->getIntValue()) );
    NotifyStopper widthstopper( colorbarwidthfld_->valueChanging );
    NotifyStopper heightstopper( colorbarheightfld_->valueChanging );
    const Geom::Size2D<int> size = viewer_.getColorBarSize();
    setColorBarSizeLimits();
    colorbarwidthfld_->setValue( size.width() );
    colorbarheightfld_->setValue( size.height() );
    colorBarSizeCB( nullptr );
}


void uiVSGSceneSettingsDlg::colorBarSizeCB( CallBacker* )
{
    viewer_.setColorBarSize( colorbarwidthfld_->getIntValue(),
			     colorbarheightfld_->getIntValue() );
}


void uiVSGSceneSettingsDlg::setColorBarSizeLimits()
{
    const int pos = colorbarposfld_->getIntValue();
    const bool horizontal = pos==dGBVis::SceneColTab::Top ||
			    pos==dGBVis::SceneColTab::Bottom;
    colorbarwidthfld_->setMaxValue( horizontal
				? dGBVis::ColorBarBounds::maxHorWidth()
				: dGBVis::ColorBarBounds::maxVertWidth() );
    colorbarwidthfld_->setMinValue( horizontal
				? dGBVis::ColorBarBounds::minHorWidth()
				: dGBVis::ColorBarBounds::minVertWidth() );
    colorbarheightfld_->setMaxValue( horizontal
				? dGBVis::ColorBarBounds::maxHorHeight()
				: dGBVis::ColorBarBounds::maxVertHeight() );
    colorbarheightfld_->setMinValue( horizontal
				? dGBVis::ColorBarBounds::minHorHeight()
				: dGBVis::ColorBarBounds::minVertHeight() );
}


void uiVSGSceneSettingsDlg::updateSensitivity()
{
    axesstylefld_->setSensitive( axesfld_->isChecked() );
    const bool showwheels = wheelmodefld_->getIntValue() !=
			    int(uiVSGViewer::WheelMode::Never);
    wheelgrowfld_->setSensitive( showwheels );
    wheellookfld_->setSensitive( showwheels );
    const bool showcolorbar = colorbarfld_->isChecked();
    colorbarposfld_->setSensitive( showcolorbar );
    colorbarwidthfld_->setSensitive( showcolorbar );
    colorbarheightfld_->setSensitive( showcolorbar );
}


bool uiVSGSceneSettingsDlg::rejectOK( CallBacker* )
{
    viewer_.showRotAxis( hadaxes_ );
    viewer_.setModernAxes( hadmodernaxes_ );
    viewer_.setWheelDisplayMode( uiVSGViewer::WheelMode(hadwheelmode_) );
    viewer_.setWheelsGrowOnHover( hadwheelgrow_ );
    viewer_.setGlassWheels( hadglasswheels_ );
    viewer_.setColorBarPos( dGBVis::SceneColTab::Pos(hadcolorbarpos_) );
    viewer_.setColorBarSize( hadcolorbarwidth_, hadcolorbarheight_ );
    viewer_.showColorBar( hadcolorbar_ );
    return true;
}
