/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uipsviewershapetab.h"

#include "iopar.h"
#include "settings.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uipsviewermanager.h"
#include "uislider.h"
#include "visprestackdisplay.h"

#define mSliderMinFactor 0.1f
#define mSliderMaxFactor 1.9f
#define mSliderDecimal   2
#define mSliderStep      0.01f


namespace PreStackView
{

uiViewer3DShapeTab::uiViewer3DShapeTab( uiParent* p,
	visSurvey::PreStackDisplay& vwr, uiViewer3DMgr& mgr )
    : uiDlgGroup( p, tr("Shape") )
    , factorslider_( 0 )
    , widthslider_( 0 )
    , applyall_( false )
    , savedefault_( false )
    , viewer_( vwr )
    , mgr_( mgr )
    , initialside_( vwr.displayOnPositiveSide() )
{
    autowidthfld_ = new uiGenInput( this, tr("Width"),
	    BoolInpSpec( true, tr("Relative"), tr("Absolute") ) );
    autowidthfld_->setValue( viewer_.displayAutoWidth() );
    autowidthfld_->valueChanged.notify(
	    mCB( this, uiViewer3DShapeTab, widthTypeChangeCB ) );

    uiSlider::Setup ss; ss.nrdec(mSliderDecimal);
    factorslider_ = new uiSlider( this, ss );
    factorslider_->attach( alignedBelow, autowidthfld_ );
    const float curfactor = viewer_.getFactor();
    factorslider_->setInterval( StepInterval<float>(mSliderMinFactor*curfactor,
				mSliderMaxFactor*curfactor, mSliderStep) );
    factorslider_->setValue( curfactor );
    factorslider_->valueChanged.notify(
	    mCB(this, uiViewer3DShapeTab, factorMoveCB) );

    widthslider_ = new uiSlider( this, ss );
    widthslider_->attach( alignedBelow, autowidthfld_ );
    const float curwidth = viewer_.getWidth();
    widthslider_->setInterval( StepInterval<float>( mSliderMinFactor*curwidth,
		mSliderMaxFactor*curwidth, mSliderStep ) );
    widthslider_->setValue( curwidth );
    widthslider_->valueChanged.notify(
	    mCB(this,uiViewer3DShapeTab,widthMoveCB) );

    switchsidebutton_ = new uiPushButton( this, tr("Switch View Side"),
	    mCB(this,uiViewer3DShapeTab,switchPushCB), true );
    switchsidebutton_->attach( alignedBelow, widthslider_ );

    initialfactor_ = curfactor;
    initialwidth_ =  curwidth;
    initialautowidth_ = autowidthfld_->getBoolValue();
    widthTypeChangeCB(0);
}


uiViewer3DShapeTab::~uiViewer3DShapeTab()
{
    if ( autowidthfld_ )
	autowidthfld_->valueChanged.remove(
		mCB( this, uiViewer3DShapeTab, widthTypeChangeCB ) );

    if ( factorslider_ )
	factorslider_->valueChanged.remove(
		mCB(this, uiViewer3DShapeTab, factorMoveCB) );

    if ( widthslider_ )
	widthslider_->valueChanged.remove(
		mCB(this, uiViewer3DShapeTab, widthMoveCB) );
}


void uiViewer3DShapeTab::widthTypeChangeCB( CallBacker* cb )
{
    const bool yn = autowidthfld_->getBoolValue();
    viewer_.displaysAutoWidth( yn );
    if ( widthslider_ )
    {
	widthslider_->setValue( viewer_.getWidth() );
	widthslider_->display( !yn );
    }

    if ( factorslider_ )
    {
	factorslider_->display( yn );
	factorslider_->setValue( viewer_.getFactor() );
    }
}


void uiViewer3DShapeTab::widthMoveCB( CallBacker* cb )
{
    mDynamicCastGet( uiSlider*,sldr,cb );
    if ( !sldr ) return;

    viewer_.setWidth( widthslider_->getFValue() );
}


void uiViewer3DShapeTab::factorMoveCB( CallBacker* cb )
{
    mDynamicCastGet( uiSlider*,sldr,cb );
    if ( !sldr ) return;

    viewer_.setFactor( factorslider_->getFValue() );
}


void uiViewer3DShapeTab::switchPushCB( CallBacker* )
{
    viewer_.displaysOnPositiveSide( !viewer_.displayOnPositiveSide() );
}


bool uiViewer3DShapeTab::acceptOK( )
{
    if ( applyToAll() )
    {
	for ( int idx=0; idx<mgr_.get3DViewers().size(); idx++ )
	{
	    visSurvey::PreStackDisplay* psv = mgr_.get3DViewers()[idx];
	    if ( !psv ) continue;

	    psv->displaysAutoWidth( autowidthfld_->getBoolValue() );
	    psv->displaysOnPositiveSide( viewer_.displayOnPositiveSide() );
	    if ( autowidthfld_->getBoolValue() )
		psv->setFactor( factorslider_->getFValue() );
	    else
		psv->setWidth( widthslider_->getFValue() );
	}
    }

#define mPSD visSurvey::PreStackDisplay
    if ( saveAsDefault() )
    {
	Settings& settings = Settings::fetch( uiViewer3DMgr::sSettings3DKey() );
	settings.set( mPSD::sKeyFactor(),viewer_.getFactor());
	settings.set( mPSD::sKeyWidth(), viewer_.getWidth() );
	settings.setYN( mPSD::sKeyAutoWidth(), viewer_.displayAutoWidth() );

	if ( !settings.write() )
	{
	    uiMSG().error(tr("Cannot write default settings"));
	    return false;
	}
    }

    initialside_ = viewer_.displayOnPositiveSide();
    initialautowidth_ = viewer_.displayAutoWidth();
    initialfactor_ = viewer_.getFactor();
    initialwidth_ = viewer_.getWidth();

    return true;
}


bool uiViewer3DShapeTab::rejectOK()
{
    viewer_.displaysOnPositiveSide( initialside_ );
    autowidthfld_->setValue( initialautowidth_ );

    if ( initialautowidth_ )
	factorslider_->setValue( initialfactor_ );
    else
	widthslider_->setValue( initialwidth_ );

    return true;
}

} // namespace PreStackView
