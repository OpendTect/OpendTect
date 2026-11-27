/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivsgscenepropertydlg.h"

#include "visvsgscenecoltab.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uiseparator.h"
#include "uispinbox.h"
#include "uistrings.h"
#include "uivsgviewer.h"

#include "bufstringset.h"
#include "datainpspec.h"
#include "notify.h"


uiVSGScenePropertyDlg::uiVSGScenePropertyDlg( uiParent* p,
					      uiVSGViewer& viewer )
    : uiDialog(p,Setup(tr("Scene properties"),mNoHelpKey))
    , viewer_(viewer)
    , hadbgcolor_(viewer.getBackgroundColor())
    , hadannotcolor_(viewer.getAnnotationColor())
    , hadgradient_(viewer.isBackgroundGradient())
    , hadaxes_(viewer.rotAxisShown())
    , hadmodernaxes_(viewer.isModernAxes())
    , hadwheelmode_(int(viewer.getWheelDisplayMode()))
    , hadwheelgrow_(viewer.isWheelsGrowOnHover())
    , hadglasswheels_(viewer.isGlassWheels())
    , hadcolorbar_(viewer.isColorBarShown())
    , hadcolorbarpos_(int(viewer.getColorBarPos()))
    , hadcolorbarwidth_(viewer.getColorBarSize().width())
    , hadcolorbarheight_(viewer.getColorBarSize().height())
{
    bgcolfld_ = new uiColorInput( this, uiColorInput::Setup(hadbgcolor_)
					.lbltxt(tr("Background color")) );
    mAttachCB( bgcolfld_->colorChanged, uiVSGScenePropertyDlg::colorCB );

    gradientfld_ = new uiCheckBox( this, tr("Gradient background") );
    gradientfld_->setChecked( hadgradient_ );
    mAttachCB( gradientfld_->activated, uiVSGScenePropertyDlg::gradientCB );
    gradientfld_->attach( alignedBelow, bgcolfld_ );

    annotcolfld_ = new uiColorInput( this, uiColorInput::Setup(hadannotcolor_)
					.lbltxt(tr("Annotation color")) );
    mAttachCB( annotcolfld_->colorChanged, uiVSGScenePropertyDlg::colorCB );
    annotcolfld_->attach( alignedBelow, gradientfld_ );

    auto* bgsep = new uiSeparator( this, "Background separator" );
    bgsep->attach( stretchedBelow, annotcolfld_ );

    auto* axeslbl = new uiLabel( this, tr("Axes") );
    axeslbl->attach( hCentered );
    axeslbl->attach( ensureBelow, bgsep );

    axesfld_ = new uiCheckBox( this, tr("Show axes") );
    axesfld_->setChecked( hadaxes_ );
    mAttachCB( axesfld_->activated, uiVSGScenePropertyDlg::axesCB );
    axesfld_->attach( ensureBelow, axeslbl );

    BufferStringSet axesstyles;
    axesstyles.add( "Classic" ).add( "Modern" );
    axesstylefld_ = new uiGenInput( this, tr("Axes style"),
				    StringListInpSpec(axesstyles) );
    axesstylefld_->setValue( hadmodernaxes_ ? 1 : 0 );
    mAttachCB( axesstylefld_->valueChanged, uiVSGScenePropertyDlg::axesCB );
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
	       uiVSGScenePropertyDlg::wheelModeCB );
    wheelmodefld_->attach( ensureBelow, wheelslbl );
    wheelmodefld_->attach( alignedWith, axesstylefld_ );

    wheelgrowfld_ = new uiCheckBox( this, tr("Enlarge wheels on mouse hover") );
    wheelgrowfld_->setChecked( hadwheelgrow_ );
    mAttachCB( wheelgrowfld_->activated,
	       uiVSGScenePropertyDlg::wheelOptionsCB );
    wheelgrowfld_->attach( alignedBelow, wheelmodefld_ );

    BufferStringSet wheellooks;
    wheellooks.add( "Classic" ).add( "Glass roller" );
    wheellookfld_ = new uiGenInput( this, tr("Wheel look"),
				    StringListInpSpec(wheellooks) );
    wheellookfld_->setValue( hadglasswheels_ ? 1 : 0 );
    mAttachCB( wheellookfld_->valueChanged,
	       uiVSGScenePropertyDlg::wheelOptionsCB );
    wheellookfld_->attach( alignedBelow, wheelgrowfld_ );

    auto* wheelssep = new uiSeparator( this, "Wheels separator" );
    wheelssep->attach( stretchedBelow, wheellookfld_ );

    auto* colorbarlbl = new uiLabel( this, tr("Color bar") );
    colorbarlbl->attach( hCentered );
    colorbarlbl->attach( ensureBelow, wheelssep );

    colorbarfld_ = new uiCheckBox( this, tr("Show color bar") );
    colorbarfld_->setChecked( hadcolorbar_ );
    mAttachCB( colorbarfld_->activated, uiVSGScenePropertyDlg::colorBarCB );
    colorbarfld_->attach( ensureBelow, colorbarlbl );
    colorbarfld_->attach( alignedWith, wheellookfld_ );

    BufferStringSet colorbarpositions;
    colorbarpositions.add( "Left" ).add( "Top" ).add( "Right" ).add( "Bottom" );
    colorbarposfld_ = new uiGenInput( this, uiStrings::sPosition(),
				      StringListInpSpec(colorbarpositions) );
    colorbarposfld_->setValue( hadcolorbarpos_ );
    mAttachCB( colorbarposfld_->valueChanged,
	       uiVSGScenePropertyDlg::colorBarPosCB );
    colorbarposfld_->attach( alignedBelow, colorbarfld_ );

    auto* widthfld = new uiLabeledSpinBox( this, uiStrings::sWidth() );
    colorbarwidthfld_ = widthfld->box();
    auto* heightfld = new uiLabeledSpinBox( this, uiStrings::sHeight() );
    colorbarheightfld_ = heightfld->box();
    setColorBarSizeLimits();
    colorbarwidthfld_->setValue( hadcolorbarwidth_ );
    colorbarheightfld_->setValue( hadcolorbarheight_ );
    mAttachCB( colorbarwidthfld_->valueChanging,
	       uiVSGScenePropertyDlg::colorBarSizeCB );
    mAttachCB( colorbarheightfld_->valueChanging,
	       uiVSGScenePropertyDlg::colorBarSizeCB );
    widthfld->attach( alignedBelow, colorbarposfld_ );
    heightfld->attach( rightOf, widthfld );

    updateSensitivity();
}


uiVSGScenePropertyDlg::~uiVSGScenePropertyDlg()
{
    detachAllNotifiers();
}


void uiVSGScenePropertyDlg::colorCB( CallBacker* )
{
    viewer_.setBackgroundColor( bgcolfld_->color() );
    viewer_.setAnnotationColor( annotcolfld_->color() );
}


void uiVSGScenePropertyDlg::gradientCB( CallBacker* )
{
    viewer_.setBackgroundGradient( gradientfld_->isChecked() );
}


void uiVSGScenePropertyDlg::axesCB( CallBacker* )
{
    viewer_.showRotAxis( axesfld_->isChecked() );
    viewer_.setModernAxes( axesstylefld_->getIntValue() == 1 );
    updateSensitivity();
}


void uiVSGScenePropertyDlg::wheelModeCB( CallBacker* )
{
    viewer_.setWheelDisplayMode(
		uiVSGViewer::WheelMode(wheelmodefld_->getIntValue()) );
    updateSensitivity();
}


void uiVSGScenePropertyDlg::wheelOptionsCB( CallBacker* )
{
    viewer_.setWheelsGrowOnHover( wheelgrowfld_->isChecked() );
    viewer_.setGlassWheels( wheellookfld_->getIntValue() == 1 );
}


void uiVSGScenePropertyDlg::colorBarCB( CallBacker* )
{
    viewer_.showColorBar( colorbarfld_->isChecked() );
    updateSensitivity();
}


void uiVSGScenePropertyDlg::colorBarPosCB( CallBacker* )
{
    viewer_.setColorBarPos(
		ODVsg::SceneColTab::Pos(colorbarposfld_->getIntValue()) );
    NotifyStopper widthstopper( colorbarwidthfld_->valueChanging );
    NotifyStopper heightstopper( colorbarheightfld_->valueChanging );
    const Geom::Size2D<int> size = viewer_.getColorBarSize();
    setColorBarSizeLimits();
    colorbarwidthfld_->setValue( size.width() );
    colorbarheightfld_->setValue( size.height() );
    colorBarSizeCB( nullptr );
}


void uiVSGScenePropertyDlg::colorBarSizeCB( CallBacker* )
{
    viewer_.setColorBarSize( colorbarwidthfld_->getIntValue(),
			     colorbarheightfld_->getIntValue() );
}


void uiVSGScenePropertyDlg::setColorBarSizeLimits()
{
    const int pos = colorbarposfld_->getIntValue();
    const bool horizontal = pos==ODVsg::SceneColTab::Top ||
			    pos==ODVsg::SceneColTab::Bottom;
    colorbarwidthfld_->setMaxValue( horizontal
				? ODVsg::ColorBarBounds::maxHorWidth()
				: ODVsg::ColorBarBounds::maxVertWidth() );
    colorbarwidthfld_->setMinValue( horizontal
				? ODVsg::ColorBarBounds::minHorWidth()
				: ODVsg::ColorBarBounds::minVertWidth() );
    colorbarheightfld_->setMaxValue( horizontal
				? ODVsg::ColorBarBounds::maxHorHeight()
				: ODVsg::ColorBarBounds::maxVertHeight() );
    colorbarheightfld_->setMinValue( horizontal
				? ODVsg::ColorBarBounds::minHorHeight()
				: ODVsg::ColorBarBounds::minVertHeight() );
}


void uiVSGScenePropertyDlg::updateSensitivity()
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


bool uiVSGScenePropertyDlg::rejectOK( CallBacker* )
{
    viewer_.setBackgroundColor( hadbgcolor_ );
    viewer_.setBackgroundGradient( hadgradient_ );
    viewer_.setAnnotationColor( hadannotcolor_ );
    viewer_.showRotAxis( hadaxes_ );
    viewer_.setModernAxes( hadmodernaxes_ );
    viewer_.setWheelDisplayMode( uiVSGViewer::WheelMode(hadwheelmode_) );
    viewer_.setWheelsGrowOnHover( hadwheelgrow_ );
    viewer_.setGlassWheels( hadglasswheels_ );
    viewer_.setColorBarPos( ODVsg::SceneColTab::Pos(hadcolorbarpos_) );
    viewer_.setColorBarSize( hadcolorbarwidth_, hadcolorbarheight_ );
    viewer_.showColorBar( hadcolorbar_ );
    return true;
}


mExternC(uiVSG) void showVSGScenePropertyDlg( uiParent*, ui3DViewerBase* );

mExternC(uiVSG) void showVSGScenePropertyDlg( uiParent* parent,
					      ui3DViewerBase* viewer )
{
    uiVSGViewer* vsgviewer = dCast(uiVSGViewer*,viewer);
    if ( !vsgviewer )
	return;

    uiVSGScenePropertyDlg dlg( parent, *vsgviewer );
    dlg.go();
}
