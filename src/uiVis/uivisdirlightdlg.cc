/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Sep 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uivisdirlightdlg.cc,v 1.3 2009-09-22 09:54:08 cvskarthika Exp $";

#include "uivisdirlightdlg.h"

#include "survinfo.h"
#include "vislight.h"
#include "uislider.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "uigeninput.h"
#include "visdataman.h"
#include "vistransmgr.h"
#include "vissurvscene.h"
#include "math.h"

uiDirLightDlg::uiDirLightDlg( uiParent* p )
    : uiDialog(p,
	       uiDialog::Setup("Directional light",
		   "Set directional light properties", mNoHelpID).modal(false))
	       // to do: specify proper help ID
    , scenefld_(0)
    , azimuthsldrfld_(0)
    , dipsldrfld_(0)
    , intensityfld_(0)
    , valchgd_(false)      
{
    visBase::DM().getIds( typeid(visSurvey::Scene), sceneids_ );
    if ( sceneids_.size() == 0 )
    {
	uiLabel* lbl = new uiLabel( this, "No scene available!" );
	return;
    }

    if ( sceneids_.size() > 1 )
    {
	BufferStringSet scenenms;
	scenenms.add( "All" );
	for ( int idx=0; idx<sceneids_.size(); idx++ )
	{
	    mDynamicCastGet(visSurvey::Scene*,scene,
		    	    visBase::DM().getObject(sceneids_[idx]))
	    scenenms.add( scene->name() );
	}

	scenefld_ = new uiLabeledComboBox( this, scenenms, "Apply light to" );
	scenefld_->box()->setCurrentItem( 1 );
	scenefld_->box()->selectionChanged.notify( 
					mCB(this,uiDirLightDlg,sceneSel) );
    }

    const CallBack chgCB ( mCB(this,uiDirLightDlg,fieldChangedCB) );

    azimuthsldrfld_ = new uiSliderExtra( this,
      uiSliderExtra::Setup("Azimuth (degrees)").withedit(true).nrdec(1).
      logscale(false), "Azimuth slider" );
    if ( sceneids_.size() > 1 )
	azimuthsldrfld_->attach( alignedBelow, scenefld_ );
    azimuthsldrfld_->sldr()->setMinValue( 0 );
    azimuthsldrfld_->sldr()->setMaxValue( 360 );
    azimuthsldrfld_->sldr()->setStep( 1 );
    azimuthsldrfld_->sldr()->setValue( 180 );
    azimuthsldrfld_->sldr()->valueChanged.notify( chgCB );

    dipsldrfld_ = new uiSliderExtra( this,
      uiSliderExtra::Setup("Dip (degrees)").withedit(true).nrdec(1).logscale(
	  false), "Dip slider" );
    dipsldrfld_->attach( alignedBelow, azimuthsldrfld_ );
    dipsldrfld_->sldr()->setMinValue( 0 );
    dipsldrfld_->sldr()->setMaxValue( 90 );
    dipsldrfld_->sldr()->setStep( 1 );
    dipsldrfld_->sldr()->setValue( 45 );
    dipsldrfld_->sldr()->valueChanged.notify( chgCB ); 

    intensityfld_ = new uiGenInput( this, "Intensity (0..1)", 
	    FloatInpSpec( 1 ) );
    intensityfld_->attach( alignedBelow, dipsldrfld_ );
    intensityfld_->valuechanged.notify( chgCB ); 
}


bool uiDirLightDlg::valueChanged() const
{
    return valchgd_;
}


void uiDirLightDlg::sceneSel( CallBacker* )
{
    updateWidgets( false );
}


void uiDirLightDlg::updateWidgets( bool initvalues )
{
    visBase::DirectionalLight* dl = getCurrentDirLight();
    if ( !dl )
    {
	if ( initvalues )
	{
	    initazimuthval_ = 0;
	    initdipval_ = 0;
	    initintensityval_ = 1;
	}
	return;
    }

    float x = dl->direction( 0 );
    float y = dl->direction( 1 );
    float z = dl->direction( 2 );
    float dip = asin( z );
    float azimuth = acos( x / cos( dip ) ) ;
    dip *= 180.0 / M_PI;
    azimuth *= 180.0 / M_PI;

    azimuthsldrfld_->sldr()->setValue( azimuth );
    dipsldrfld_->sldr()->setValue( dip );
    intensityfld_->setValue( dl->intensity() );

    if ( initvalues)
    {
	    initazimuthval_ = azimuth;
	    initdipval_ = dip;
	    initintensityval_ = dl->intensity();
    }
}


void uiDirLightDlg::setDirLight()
{
    validateInput();

    const bool lightall = scenefld_ && scenefld_->box()->currentItem()==0;
    
    for ( int idx=0; idx<sceneids_.size(); idx++ )
    {
	bool dolight = !scenefld_ || lightall ||
		       idx == scenefld_->box()->currentItem()-1;
	if ( !dolight ) continue;

	mDynamicCastGet(visSurvey::Scene*,scene,
			visBase::DM().getObject(sceneids_[idx]));

	static const float deg2rad = M_PI / 180.0;

	float x = cos( azimuthsldrfld_->sldr()->getValue()*deg2rad ) * 
	          cos( dipsldrfld_->sldr()->getValue()*deg2rad );
	float y = sin( azimuthsldrfld_->sldr()->getValue()*deg2rad ) * 
	          cos( dipsldrfld_->sldr()->getValue()*deg2rad );
       	float z = sin (dipsldrfld_->sldr()->getValue()*deg2rad );

	if ( !getCurrentDirLight() )
	{
	    RefMan<visBase::DirectionalLight> dl =
		visBase::DirectionalLight::create();
	    scene->setDirectionalLight( *dl );
	}

	RefMan<visBase::DirectionalLight> dl = getCurrentDirLight();

	dl->setDirection( x, y, z ); 
 	dl->setIntensity( intensityfld_->getfValue() );
    }
}


visBase::DirectionalLight* uiDirLightDlg::getCurrentDirLight() const
{
    if ( sceneids_.size() == 0 )
	return 0;

    int sceneidx = scenefld_ ? scenefld_->box()->currentItem()-1 : 0;
    if ( sceneidx < 0 ) sceneidx = 0;
    mDynamicCastGet(visSurvey::Scene*,scene,
		    visBase::DM().getObject(sceneids_[sceneidx]))
    return scene->getDirectionalLight();
}


void uiDirLightDlg::validateInput()
{
    const float az = azimuthsldrfld_->sldr()->getValue();
    if ( ( az < 0 ) || ( az > 360 ) )
	azimuthsldrfld_->sldr()->setValue( 0 );
    
    const float dip = dipsldrfld_->sldr()->getValue();
    if ( ( dip < 0 ) || ( dip > 90 ) )
	dipsldrfld_->sldr()->setValue( 0 );
    
    const float intensity = intensityfld_->getfValue();
    if ( ( intensity < 0 ) || ( intensity > 1 ) )
	intensityfld_->setValue( 1 );
}


bool uiDirLightDlg::acceptOK( CallBacker* )
{
    if ( !azimuthsldrfld_ )
	return true;

    azimuthsldrfld_->processInput();
    dipsldrfld_->processInput();
    valchgd_ = ( ( azimuthsldrfld_->sldr()->getValue() != initazimuthval_ ) 
	         || ( dipsldrfld_->sldr()->getValue() != initdipval_ )
		 || ( intensityfld_->getfValue() != initintensityval_ ) )
	       ? true : false;

    if ( valchgd_ )
	setDirLight();

    return true;
}


bool uiDirLightDlg::rejectOK( CallBacker* )
{
    valchgd_ = false;
    
    azimuthsldrfld_->sldr()->setValue( initazimuthval_ );
    dipsldrfld_->sldr()->setValue( initdipval_ );
    intensityfld_->setValue( initintensityval_ );

    setDirLight();

    return true;
}


void uiDirLightDlg::fieldChangedCB( CallBacker* )
{
    setDirLight();
}


void uiDirLightDlg::show()
{
    updateWidgets( true );
    uiDialog::show();
}



