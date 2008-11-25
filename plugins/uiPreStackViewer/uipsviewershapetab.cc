/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          5-11-2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uipsviewershapetab.cc,v 1.3 2008-11-25 15:35:21 cvsbert Exp $";

#include "uipsviewershapetab.h"

#include "iopar.h"
#include "settings.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uipsviewermanager.h"
#include "uislider.h"
#include "visprestackviewer.h"

#define mSliderMinFactor 0.1
#define mSliderMaxFactor 1.9
#define mSliderDecimal   2
#define mSliderStep      0.01


namespace PreStackView
{


uiPSViewerShapeTab::uiPSViewerShapeTab( uiParent* p, PreStackViewer& viewer,
       				      uiPSViewerMgr& mgr )
    : uiDlgGroup( p, "Shape" )
    , factorslider_( 0 )
    , widthslider_( 0 )		
    , applyall_( false )			
    , savedefault_( false )						
    , viewer_( viewer )
    , mgr_( mgr )		       
    , initialside_( viewer.displayOnPositiveSide() )
{
    autowidthfld_ = new uiGenInput( this, "Width",
	    BoolInpSpec( true, "Relative", "Absolute" ) );
    autowidthfld_->setValue( viewer_.displayAutoWidth() );
    autowidthfld_->valuechanged.notify(
            mCB( this, uiPSViewerShapeTab, widthTypeChangeCB ) );

    factorslider_ = new uiSlider( this,0,mSliderDecimal,false );
    factorslider_->attach( alignedBelow, autowidthfld_ );
    const float curfactor = viewer_.getFactor();
    factorslider_->setInterval( StepInterval<float>(mSliderMinFactor*curfactor,
				mSliderMaxFactor*curfactor, mSliderStep) );
    factorslider_->setValue( curfactor );
    factorslider_->valueChanged.notify(
	    mCB(this, uiPSViewerShapeTab, factorMoveCB) );
    
    widthslider_ = new uiSlider( this,0,mSliderDecimal,false );
    widthslider_->attach( alignedBelow, autowidthfld_ );
    const float curwidth = viewer_.getWidth();
    widthslider_->setInterval( StepInterval<float>( mSliderMinFactor*curwidth,
		mSliderMaxFactor*curwidth, mSliderStep ) );
    widthslider_->setValue( curwidth );
    widthslider_->valueChanged.notify( 
	    mCB(this, uiPSViewerShapeTab, widthMoveCB) );

    switchsidebutton_ = new uiPushButton( this, "Switch View Side",
	    mCB(this,uiPSViewerShapeTab,switchPushCB), true );
    switchsidebutton_->attach( alignedBelow, widthslider_ );

    initialfactor_ = curfactor;
    initialwidth_ =  curwidth;
    initialautowidth_ = autowidthfld_->getBoolValue();
    widthTypeChangeCB(0);
}


uiPSViewerShapeTab::~uiPSViewerShapeTab()
{
    if ( autowidthfld_ )
	autowidthfld_->valuechanged.remove(
		mCB( this, uiPSViewerShapeTab, widthTypeChangeCB ) );

    if ( factorslider_ )
	factorslider_->valueChanged.remove(
		mCB(this, uiPSViewerShapeTab, factorMoveCB) );

    if ( widthslider_ )
	widthslider_->valueChanged.remove(
		mCB(this, uiPSViewerShapeTab, widthMoveCB) );
}


void uiPSViewerShapeTab::widthTypeChangeCB( CallBacker* cb )
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


void uiPSViewerShapeTab::widthMoveCB( CallBacker* cb )
{
    mDynamicCastGet( uiSlider*,sldr,cb );
    if ( !sldr ) return;

    viewer_.setWidth( widthslider_->getValue() );
}


void uiPSViewerShapeTab::factorMoveCB( CallBacker* cb )
{
    mDynamicCastGet( uiSlider*,sldr,cb );
    if ( !sldr ) return;

    viewer_.setFactor( factorslider_->getValue() );
}


void uiPSViewerShapeTab::switchPushCB( CallBacker* )
{
    viewer_.displaysOnPositiveSide( !viewer_.displayOnPositiveSide() );
}


bool uiPSViewerShapeTab::acceptOK( )
{
    if ( !&viewer_ )
	return false;

    if ( applyToAll() )
    {
	for ( int idx=0; idx<mgr_.getViewers().size(); idx++ )
	{
	    PreStackViewer* psv = mgr_.getViewers()[idx];
	    if ( !psv ) continue;
	    
	    psv->displaysAutoWidth( autowidthfld_->getBoolValue() );
	    psv->displaysOnPositiveSide( viewer_.displayOnPositiveSide() );
	    if ( autowidthfld_->getBoolValue() )
		psv->setFactor( factorslider_->getValue() );
	    else
		psv->setWidth( widthslider_->getValue() );
	}
    }

    if ( saveAsDefault() )
    {
	Settings::common().set(
		PreStackViewer::sKeyFactor(),viewer_.getFactor());
	Settings::common().set( 
		PreStackViewer::sKeyWidth(), viewer_.getWidth() );
	Settings::common().set( 
		PreStackViewer::sKeyAutoWidth(), viewer_.displayAutoWidth() );

	if ( !Settings::common().write() )
	{
	    uiMSG().error("Cannot write");
	    return false;
	}
    }

    return true;
}


bool uiPSViewerShapeTab::rejectOK( CallBacker* )
{
    viewer_.displaysOnPositiveSide( initialside_ );
    autowidthfld_->setValue( initialautowidth_ );
    
    if ( initialautowidth_ )
    	factorslider_->setValue( initialfactor_ );
    else
    	widthslider_->setValue( initialwidth_ );

    return true;
}


}; //namespace

