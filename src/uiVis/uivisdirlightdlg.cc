/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Sep 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uivisdirlightdlg.cc,v 1.2 2009-09-21 21:47:14 cvskris Exp $";

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
		   "Set directional light properties", mNoHelpID)
	       // to do: specify proper help ID
	       .canceltext(""))
    , scenefld_(0)
    , azimuthsliderfld_(0)
    , dipsliderfld_(0)
    , intensityfld_(0)
    , valchgd_(false)      
{
    visBase::DM().getIds( typeid(visSurvey::Scene), sceneids_ );
    if ( sceneids_.size() == 0 )
    {
	uiLabel* lbl = new uiLabel( this, "No scenes available" );
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

    // to do: add validation
    azimuthsliderfld_ = new uiSliderExtra( this,
      uiSliderExtra::Setup("Azimuth (degrees)").withedit(true).nrdec(1).
      logscale(false), "Azimuth slider" );
    if ( sceneids_.size() > 1 )
	azimuthsliderfld_->attach( alignedBelow, scenefld_ );
    azimuthsliderfld_->sldr()->setMinValue( 0 );
    azimuthsliderfld_->sldr()->setMaxValue( 360 );
    azimuthsliderfld_->sldr()->setStep( 1 );
    azimuthsliderfld_->sldr()->setValue( 180 );
    azimuthsliderfld_->sldr()->valueChanged.notify( chgCB );

    dipsliderfld_ = new uiSliderExtra( this,
      uiSliderExtra::Setup("Dip (degrees)").withedit(true).nrdec(1).logscale(
	  false), "Dip slider" );
    dipsliderfld_->attach( alignedBelow, azimuthsliderfld_ );
    dipsliderfld_->sldr()->setMinValue( 0 );
    dipsliderfld_->sldr()->setMaxValue( 90 );
    dipsliderfld_->sldr()->setStep( 1 );
    dipsliderfld_->sldr()->setValue( 45 );
    dipsliderfld_->sldr()->valueChanged.notify( chgCB ); 

    intensityfld_ = new uiGenInput( this, "Intensity (0..1)", 
	    FloatInpSpec( 0.75 ) );
    intensityfld_->attach( alignedBelow, dipsliderfld_ );
    intensityfld_->valuechanged.notify( chgCB ); 
}


bool uiDirLightDlg::valueChanged() const
{
    return valchgd_;
}


void uiDirLightDlg::sceneSel( CallBacker* )
{
    updateWidgets();
}


void uiDirLightDlg::updateWidgets()
{
    visBase::DirectionalLight* dl = getCurrentDirLight();
    if ( !dl )
	return;
    float x = dl->direction( 0 );
    float y = dl->direction( 1 );
    float z = dl->direction( 2 );
    float dip = asin( z );

    azimuthsliderfld_->sldr()->setValue( acos( x / cos( dip ) ) * 180.0 / M_PI );
    dipsliderfld_->sldr()->setValue( dip * 180.0 / M_PI );
    intensityfld_->setValue( dl->intensity() );
}


void uiDirLightDlg::setDirLight()
{
    const bool lightall = scenefld_ && scenefld_->box()->currentItem()==0;
    for ( int idx=0; idx<sceneids_.size(); idx++ )
    {
	bool dolight = !scenefld_ || lightall ||
		       idx == scenefld_->box()->currentItem()-1;
	if ( !dolight ) continue;

	mDynamicCastGet(visSurvey::Scene*,scene,
			visBase::DM().getObject(sceneids_[idx]));

	static const float deg2rad = M_PI / 180.0;

	float x = cos( azimuthsliderfld_->sldr()->getValue()*deg2rad ) * 
	          cos( dipsliderfld_->sldr()->getValue()*deg2rad );
	float y = sin( azimuthsliderfld_->sldr()->getValue()*deg2rad ) * 
	          cos( dipsliderfld_->sldr()->getValue()*deg2rad );
       	float z = sin (dipsliderfld_->sldr()->getValue()*deg2rad );

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
	//return visSurvey::STM().defDirLight();
	return 0;

    int sceneidx = scenefld_ ? scenefld_->box()->currentItem()-1 : 0;
    if ( sceneidx < 0 ) sceneidx = 0;
    mDynamicCastGet(visSurvey::Scene*,scene,
		    visBase::DM().getObject(sceneids_[sceneidx]))
    return scene->getDirectionalLight();
}


bool uiDirLightDlg::acceptOK( CallBacker* )
{
    if ( !azimuthsliderfld_ )
	return true;

    azimuthsliderfld_->processInput();
    dipsliderfld_->processInput();
    valchgd_ = ( ( azimuthsliderfld_->sldr()->getValue() != initazimuthval_ ) 
	         || ( dipsliderfld_->sldr()->getValue() != initdipval_ )
		 || ( intensityfld_->getfValue() != initintensityval_ ) )
	       ? true : false;

    if ( valchgd_ )
	setDirLight();

    return true;
}


void uiDirLightDlg::fieldChangedCB( CallBacker* )
{
    setDirLight();
}


