/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/

#include "uipickpropdlg.h"

#include "color.h"
#include "draw.h"
#include "pickset.h"
#include "settings.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uimarkerstyle.h"
#include "uislider.h"
#include "vispicksetdisplay.h"
#include "vistristripset.h"


uiPickPropDlg::uiPickPropDlg( uiParent* p, Pick::Set& set,
			      visSurvey::PickSetDisplay* psd )
    : uiDialog(p,Setup(tr("PointSet Display Properties"),
		       mNoDlgTitle,mTODOHelpKey))
    , set_( set )
    , psd_( psd )
{
    set_.ref();
    setTitleText( uiString::empty() );
    usedrawstylefld_ = new uiCheckBox( this, tr("Connect picks") );
    const bool hasbody = psd && psd->isBodyDisplayed();
    const bool hassty = hasbody
		     || set_.connection()==Pick::Set::Disp::Close;
    usedrawstylefld_->setChecked( hassty );
    usedrawstylefld_->activated.notify( mCB(this,uiPickPropDlg,drawSel) );

    drawstylefld_ = new uiGenInput( this, tr("with"),
		   BoolInpSpec(true,uiStrings::sLine(),uiStrings::sSurface()) );
    drawstylefld_->setValue( !hasbody );
    drawstylefld_->valuechanged.notify( mCB(this,uiPickPropDlg,drawStyleCB) );
    drawstylefld_->attach( rightOf, usedrawstylefld_ );

    TypeSet<OD::MarkerStyle3D::Type> excl;
    excl.add( OD::MarkerStyle3D::None );
    stylefld_ = new uiMarkerStyle3D( this, uiMarkerStyle::Setup(), &excl );
    stylefld_->change.notify( mCB(this,uiPickPropDlg,styleSel) );
    stylefld_->attach( alignedBelow, usedrawstylefld_ );

    bool usethreshold = true;
    Settings::common().getYN( Pick::Set::sKeyUseThreshold(), usethreshold );
    usethresholdfld_ = new uiCheckBox( this,
			   tr("Switch to Point mode for all large PointSets") );
    usethresholdfld_->setChecked( usethreshold );
    usethresholdfld_->activated.notify( mCB(this,uiPickPropDlg,useThresholdCB));
    usethresholdfld_->attach( alignedBelow, stylefld_ );

    thresholdfld_ =  new uiGenInput( this, tr("Threshold size for Point mode"));
    thresholdfld_->attach( rightAlignedBelow, usethresholdfld_ );
    thresholdfld_->valuechanged.notify(
				     mCB(this,uiPickPropDlg,thresholdChangeCB));
    thresholdfld_->setSensitive( usethreshold );
    thresholdfld_->setValue( Pick::Set::getSizeThreshold() );

    stylefld_->setMarkerStyle( set_.markerStyle() );
    drawSel( 0 );
}


uiPickPropDlg::~uiPickPropDlg()
{
    set_.unRef();
}


void uiPickPropDlg::drawSel( CallBacker* )
{
    const bool usestyle = usedrawstylefld_->isChecked();
    drawstylefld_->display( usestyle );

    if ( !usestyle )
    {
	if ( set_.connection() == Pick::Set::Disp::Close )
	    set_.setConnection( Pick::Set::Disp::None );

	if ( psd_ )
	    psd_->displayBody( false );
    }
    else
	drawStyleCB( 0 );
}


void uiPickPropDlg::drawStyleCB( CallBacker* )
{
    const bool showline = drawstylefld_->getBoolValue();
    if ( psd_ )
	psd_->displayBody( !showline );

    if ( showline )
	set_.setConnection( Pick::Set::Disp::Close );
    else
    {
	if ( !psd_ )
	    return;
	set_.setConnection( Pick::Set::Disp::None );

	if ( !psd_->getDisplayBody() )
	    psd_->setBodyDisplay();
    }
}


void uiPickPropDlg::styleSel( CallBacker* )
{
    OD::MarkerStyle3D style( set_.markerStyle() );
    stylefld_->getMarkerStyle( style );
    set_.setMarkerStyle( style );
}


void uiPickPropDlg::useThresholdCB( CallBacker* )
{
    const bool usesthreshold = usethresholdfld_->isChecked();
    thresholdfld_->setSensitive( usesthreshold );
    Settings::common().setYN( Pick::Set::sKeyUseThreshold(), usesthreshold );
    Settings::common().write();
}


void uiPickPropDlg::thresholdChangeCB( CallBacker* )
{
    const int thresholdval = thresholdfld_->getIntValue();
    if ( thresholdval == Pick::Set::getSizeThreshold() )
	return;
    Settings::common().set( Pick::Set::sKeyThresholdSize(), thresholdval );
    Settings::common().write();
}


bool uiPickPropDlg::acceptOK()
{
    return true;
}
