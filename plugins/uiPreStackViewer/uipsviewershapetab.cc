/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          5-11-2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uipsviewershapetab.cc,v 1.8 2011/04/28 11:30:53 cvsbert Exp $";

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


uiViewer3DShapeTab::uiViewer3DShapeTab( uiParent* p, 
	PreStackView::Viewer3D& vwr, uiViewer3DMgr& mgr )
    : uiDlgGroup( p, "Shape" )
    , factorslider_( 0 )
    , widthslider_( 0 )		
    , applyall_( false )			
    , savedefault_( false )						
    , viewer_( vwr )
    , mgr_( mgr )		       
    , initialside_( vwr.displayOnPositiveSide() )
{
    autowidthfld_ = new uiGenInput( this, "Width",
	    BoolInpSpec( true, "Relative", "Absolute" ) );
    autowidthfld_->setValue( viewer_.displayAutoWidth() );
    autowidthfld_->valuechanged.notify( 
	    mCB( this, uiViewer3DShapeTab, widthTypeChangeCB ) );

    factorslider_ = new uiSlider( this,0,mSliderDecimal,false );
    factorslider_->attach( alignedBelow, autowidthfld_ );
    const float curfactor = viewer_.getFactor();
    factorslider_->setInterval( StepInterval<float>(mSliderMinFactor*curfactor,
				mSliderMaxFactor*curfactor, mSliderStep) );
    factorslider_->setValue( curfactor );
    factorslider_->valueChanged.notify( 
	    mCB(this, uiViewer3DShapeTab, factorMoveCB) );
    
    widthslider_ = new uiSlider( this,0,mSliderDecimal,false );
    widthslider_->attach( alignedBelow, autowidthfld_ );
    const float curwidth = viewer_.getWidth();
    widthslider_->setInterval( StepInterval<float>( mSliderMinFactor*curwidth,
		mSliderMaxFactor*curwidth, mSliderStep ) );
    widthslider_->setValue( curwidth );
    widthslider_->valueChanged.notify( 
	    mCB(this,uiViewer3DShapeTab,widthMoveCB) );

    switchsidebutton_ = new uiPushButton( this, "Switch View Side",
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
	autowidthfld_->valuechanged.remove(
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

    viewer_.setWidth( widthslider_->getValue() );
}


void uiViewer3DShapeTab::factorMoveCB( CallBacker* cb )
{
    mDynamicCastGet( uiSlider*,sldr,cb );
    if ( !sldr ) return;

    viewer_.setFactor( factorslider_->getValue() );
}


void uiViewer3DShapeTab::switchPushCB( CallBacker* )
{
    viewer_.displaysOnPositiveSide( !viewer_.displayOnPositiveSide() );
}


bool uiViewer3DShapeTab::acceptOK( )
{
    if ( !&viewer_ )
	return false;

    if ( applyToAll() )
    {
	for ( int idx=0; idx<mgr_.get3DViewers().size(); idx++ )
	{
	    PreStackView::Viewer3D* psv = mgr_.get3DViewers()[idx];
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
	Settings& settings = Settings::fetch( uiViewer3DMgr::sSettings3DKey() );
	settings.set( PreStackView::Viewer3D::sKeyFactor(),viewer_.getFactor());
	settings.set( PreStackView::Viewer3D::sKeyWidth(), viewer_.getWidth() );
	settings.set( PreStackView::Viewer3D::sKeyAutoWidth(),
		      viewer_.displayAutoWidth() );

	if ( !settings.write() )
	{
	    uiMSG().error("Cannot write default settings");
	    return false;
	}
    }

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


}; //namespace

