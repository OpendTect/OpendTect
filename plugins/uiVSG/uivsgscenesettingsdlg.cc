/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivsgscenesettingsdlg.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uiseparator.h"
#include "uivsgviewer.h"

#include "bufstringset.h"
#include "datainpspec.h"
#include "notify.h"


uiVSGSceneSettingsDlg::uiVSGSceneSettingsDlg( uiParent* p,
					      uiVSGViewer& viewer )
    : uiDialog(p,Setup(tr("VSG Scene Settings"),mNoHelpKey))
    , viewer_(viewer)
    , hadbgcolor_(viewer.getBackgroundColor())
    , hadannotcolor_(viewer.getAnnotationColor())
    , hadgradient_(viewer.isBackgroundGradient())
    , hadwheelmode_(int(viewer.getWheelDisplayMode()))
    , hadwheelgrow_(viewer.isWheelsGrowOnHover())
    , hadglasswheels_(viewer.isGlassWheels())
{
    bgcolfld_ = new uiColorInput( this, uiColorInput::Setup(hadbgcolor_)
					.lbltxt(tr("Background color")) );
    mAttachCB( bgcolfld_->colorChanged, uiVSGSceneSettingsDlg::colorCB );

    gradientfld_ = new uiCheckBox( this, tr("Gradient background") );
    gradientfld_->setChecked( hadgradient_ );
    mAttachCB( gradientfld_->activated, uiVSGSceneSettingsDlg::gradientCB );
    gradientfld_->attach( alignedBelow, bgcolfld_ );

    annotcolfld_ = new uiColorInput( this, uiColorInput::Setup(hadannotcolor_)
					.lbltxt(tr("Annotation color")) );
    mAttachCB( annotcolfld_->colorChanged, uiVSGSceneSettingsDlg::colorCB );
    annotcolfld_->attach( alignedBelow, gradientfld_ );

    auto* bgsep = new uiSeparator( this, "Background separator" );
    bgsep->attach( stretchedBelow, annotcolfld_ );

    auto* wheelslbl = new uiLabel( this, tr("Wheels") );
    wheelslbl->attach( hCentered );
    wheelslbl->attach( ensureBelow, bgsep );

    BufferStringSet wheelmodes;
    wheelmodes.add( "Hide" ).add( "Always show" ).add( "Show on mouse hover" );
    wheelmodefld_ = new uiGenInput( this, tr("Zoom/Rotate wheels display"),
				    StringListInpSpec(wheelmodes) );
    wheelmodefld_->setValue( hadwheelmode_ );
    mAttachCB( wheelmodefld_->valueChanged,
	       uiVSGSceneSettingsDlg::wheelModeCB );
    wheelmodefld_->attach( ensureBelow, wheelslbl );

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

    updateSensitivity();
}


uiVSGSceneSettingsDlg::~uiVSGSceneSettingsDlg()
{
    detachAllNotifiers();
}


void uiVSGSceneSettingsDlg::colorCB( CallBacker* )
{
    viewer_.setBackgroundColor( bgcolfld_->color() );
    viewer_.setAnnotationColor( annotcolfld_->color() );
}


void uiVSGSceneSettingsDlg::gradientCB( CallBacker* )
{
    viewer_.setBackgroundGradient( gradientfld_->isChecked() );
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


void uiVSGSceneSettingsDlg::updateSensitivity()
{
    const bool showwheels = wheelmodefld_->getIntValue() !=
			    int(uiVSGViewer::WheelMode::Never);
    wheelgrowfld_->setSensitive( showwheels );
    wheellookfld_->setSensitive( showwheels );
}


bool uiVSGSceneSettingsDlg::rejectOK( CallBacker* )
{
    viewer_.setBackgroundColor( hadbgcolor_ );
    viewer_.setBackgroundGradient( hadgradient_ );
    viewer_.setAnnotationColor( hadannotcolor_ );
    viewer_.setWheelDisplayMode( uiVSGViewer::WheelMode(hadwheelmode_) );
    viewer_.setWheelsGrowOnHover( hadwheelgrow_ );
    viewer_.setGlassWheels( hadglasswheels_ );
    return true;
}
