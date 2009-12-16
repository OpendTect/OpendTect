/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Sep 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uivisdirlightdlg.cc,v 1.15 2009-12-16 10:56:22 cvskarthika Exp $";

#include "uivisdirlightdlg.h"

#include "math.h"
#include "survinfo.h"
#include "vislight.h"
#include "uibuttongroup.h"
#include "uislider.h"
#include "uicombobox.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uigeninput.h"
#include "uiseparator.h"
#include "visdataman.h"
#include "vistransmgr.h"
#include "vissurvscene.h"
#include "uivispartserv.h"
#include "angles.h"

#define mInitAzimuth		0
#define mInitDip		90
#define mInitIntensity		100
#define mInitHeadOnIntensity	100
#define mInitAmbIntensity	100

uiDirLightDlg::uiDirLightDlg( uiParent* p, uiVisPartServer* visserv )
    : uiDialog(p,
	       uiDialog::Setup("Light properties",
		   "Set light properties", "50.0.18")
	       .modal(false))
	       // to do: specify proper help ID
    , visserv_(visserv)
    , lightgrp_(0)
    , lightlbl_(0)		  
    , cameralightfld_(0)		    
    , scenelightfld_(0)
    , scenefld_(0)
    , azimuthfld_(0)
    , dipfld_(0)
    , intensityfld_(0)
    , headonintensityfld_(0)
    , ambintensityfld_(0)
    , sep_(0)  
    , showpdfld_(0)
    , pd_(0)
    , pddlg_(new uiDialog(this, 
		uiDialog::Setup("Polar diagram", "Set azimuth and dip", 
		    mNoHelpID)
		.modal(false)))
{
    pddlg_->setCtrlStyle( LeaveOnly );
    pddlg_->finaliseDone.notify( mCB(this, uiDirLightDlg, pdDlgDoneCB) );

    lightgrp_ = new uiButtonGroup( this, "", false );
    lightgrp_->setExclusive( true );
    lightlbl_ = new uiLabel( this, "Type of directional light" );
    lightlbl_->attach( leftOf, lightgrp_ );
    cameralightfld_ = new uiRadioButton( lightgrp_, 
	    "positioned at the camera" );
    scenelightfld_ = new uiRadioButton( lightgrp_, "relative to the scene" );

    scenefld_ = new uiLabeledComboBox( this, "Apply light to" );
    scenefld_->attach( alignedBelow, lightgrp_ );

    const CallBack chgCB ( mCB(this,uiDirLightDlg,fieldChangedCB) );

    azimuthfld_ = new uiSliderExtra( this,
      uiSliderExtra::Setup("Azimuth (degrees)").withedit(true).nrdec(1).
      logscale(false), "Azimuth slider" );
    azimuthfld_->attach( alignedBelow, scenefld_ );
    azimuthfld_->sldr()->setMinValue( 0 );
    azimuthfld_->sldr()->setMaxValue( 360 );
    azimuthfld_->sldr()->setStep( 5 );

    dipfld_ = new uiSliderExtra( this,
      uiSliderExtra::Setup("Dip (degrees)").withedit(true).nrdec(1).logscale(
	  false), "Dip slider" );
    dipfld_->attach( alignedBelow, azimuthfld_ );
    dipfld_->sldr()->setMinValue( 0 );
    dipfld_->sldr()->setMaxValue( 90 );
    dipfld_->sldr()->setStep( 5 );

    intensityfld_ = new uiSliderExtra( this,
	    uiSliderExtra::Setup("Intensity (%)").withedit(true).
	    		         nrdec(1).logscale(false), "Intensity slider" );
    intensityfld_->attach( alignedBelow, dipfld_ );
    intensityfld_->sldr()->setMinValue( 0 );
    intensityfld_->sldr()->setMaxValue( 100 );
    intensityfld_->sldr()->setStep( 5 );
    intensityfld_->display( false );

    showpdfld_ = new uiPushButton( this, "Show polar diagram", false );
    showpdfld_->attach( alignedBelow, dipfld_ );

    sep_ = new uiSeparator( this, "HSep", true );
    sep_->attach( stretchedBelow, showpdfld_ );
	    
    headonintensityfld_ = new uiSliderExtra( this,
	    uiSliderExtra::Setup("Camera light intensity (%)").
	    			 withedit(true).nrdec(1).logscale(false), 
				 "Camera light intensity slider" );
//    headonintensityfld_->attach( centeredBelow, showpdfld_ );
    headonintensityfld_->sldr()->setMinValue( 0 );
    headonintensityfld_->sldr()->setMaxValue( 100 );
    headonintensityfld_->sldr()->setStep( 5 );
    headonintensityfld_->display( false );

    ambintensityfld_ = new uiSliderExtra( this,
	    uiSliderExtra::Setup("Ambient light intensity (%)").
	    			 withedit(true).nrdec(1).logscale(false), 
				 "Ambient light intensity slider" );
    ambintensityfld_->attach( centeredBelow, sep_ );
    ambintensityfld_->sldr()->setMinValue( 0 );
    ambintensityfld_->sldr()->setMaxValue( 100 );
    ambintensityfld_->sldr()->setStep( 5 );

    InitInfoType initinfo;
    if ( updateSceneSelector() )
        initinfo = initinfo_[0];

    initlighttype_ = 1;
    lightgrp_->selectButton( (int) initlighttype_ );
    cameralightfld_->activated.notify( 
	    mCB( this,uiDirLightDlg,lightSelChangedCB) );
    scenelightfld_->activated.notify( 
	    mCB( this,uiDirLightDlg,lightSelChangedCB) );
    azimuthfld_->sldr()->setValue( initinfo.azimuth_ );
    azimuthfld_->sldr()->valueChanged.notify( chgCB );
    dipfld_->sldr()->setValue( initinfo.dip_ );
    dipfld_->sldr()->valueChanged.notify( chgCB ); 
    intensityfld_->sldr()->setValue( initinfo.intensity_ );
    intensityfld_->sldr()->valueChanged.notify( chgCB ); 
    headonintensityfld_->sldr()->setValue( initinfo.headonintensity_ );
    headonintensityfld_->sldr()->valueChanged.notify( 
	    mCB(this,uiDirLightDlg,headOnChangedCB) ); 
    ambintensityfld_->sldr()->setValue( initinfo.ambintensity_ );
    ambintensityfld_->sldr()->valueChanged.notify( 
	    mCB(this,uiDirLightDlg,ambientChangedCB) ); 
    scenefld_->box()->selectionChanged.notify( 
	    mCB(this,uiDirLightDlg,sceneSelChangedCB) );
    visserv_->nrScenesChange().notify(
	    mCB(this,uiDirLightDlg,nrScenesChangedCB) );
    showpdfld_->activated.notify(
	    mCB(this,uiDirLightDlg,showPolarDiagramCB) );
}


uiDirLightDlg::~uiDirLightDlg()
{
    removeSceneNotifiers();

    const CallBack chgCB ( mCB(this,uiDirLightDlg,fieldChangedCB) );
    azimuthfld_->sldr()->valueChanged.remove( chgCB );
    dipfld_->sldr()->valueChanged.remove( chgCB ); 
    intensityfld_->sldr()->valueChanged.remove( chgCB ); 
    headonintensityfld_->sldr()->valueChanged.remove( 
	    mCB(this,uiDirLightDlg,headOnChangedCB) ); 
    ambintensityfld_->sldr()->valueChanged.remove( 
	    mCB(this,uiDirLightDlg,ambientChangedCB) ); 
    scenefld_->box()->selectionChanged.remove( 
	    mCB(this,uiDirLightDlg,sceneSelChangedCB) );
    showpdfld_->activated.remove(
	    mCB(this,uiDirLightDlg,showPolarDiagramCB) );
    visserv_->nrScenesChange().remove(
	    mCB(this,uiDirLightDlg,nrScenesChangedCB) );
    cameralightfld_->activated.remove( 
	    mCB( this,uiDirLightDlg,lightSelChangedCB) );
    scenelightfld_->activated.remove( 
	    mCB( this,uiDirLightDlg,lightSelChangedCB) );

    if ( pd_ )
    {
	pd_->valueChanged.remove( mCB(this, uiDirLightDlg, polarDiagramCB) );
        delete pd_;
	pd_ = 0;
    }
    
    pddlg_->finaliseDone.remove( mCB(this, uiDirLightDlg, pdDlgDoneCB) );
    pddlg_->close();
    delete pddlg_;
}


void uiDirLightDlg::pdDlgDoneCB( CallBacker* )
{
    if ( !pd_ )
    {
        pd_ = new uiPolarDiagram( pddlg_ );
        pd_->setValues( azimuthfld_->sldr()->getValue(), 
		dipfld_->sldr()->getValue() );
        pd_->valueChanged.notify( mCB(this, uiDirLightDlg, polarDiagramCB) );
    }
}


float uiDirLightDlg::getHeadOnIntensity() const
{
    float intensity = headonintensityfld_->sldr()->getValue();
    if ( ( intensity < 0 ) || ( intensity > 100 ) )
	headonintensityfld_->sldr()->setValue( 100 );
    return headonintensityfld_->sldr()->getValue() / 100;
}


void uiDirLightDlg::setHeadOnIntensity( float value )
{
    if ( ( value >= 0 ) && ( value <= 1.0) )
	headonintensityfld_->sldr()->setValue( value * 100 );
}


void uiDirLightDlg::removeSceneNotifiers()
{
    for ( int idx = 0; idx < initinfo_.size(); idx++ )
    {
	mDynamicCastGet(visSurvey::Scene*,scene,
		visserv_->getObject(initinfo_[idx].sceneid_));
	if ( scene )
	    scene->nameChanged.remove(
		    mCB(this,uiDirLightDlg,sceneNameChangedCB) );
    }
}


int uiDirLightDlg::updateSceneSelector()
{
    updateInitInfo();
    scenefld_->box()->empty();

    if ( initinfo_.size() == 0 )
    {
        scenefld_->label()->setText( "No scene!" );
	return 0;
    }

    if ( initinfo_.size() >= 1 )
    {
	BufferStringSet scenenms;
	if ( initinfo_.size() > 1 )
	    scenenms.add( "All" );
	for ( int idx = 0; idx < initinfo_.size(); idx++ )
	{
	    mDynamicCastGet(visSurvey::Scene*,scene,
		    	    visserv_->getObject(initinfo_[idx].sceneid_));
	    if ( scene )
	    {
		scenenms.add( scene->name() );
		scene->nameChanged.notify(
			mCB(this,uiDirLightDlg,sceneNameChangedCB) );
	    }
	}

	scenefld_->label()->setText( "Apply light to" );
	scenefld_->box()->addItems( scenenms );
        scenefld_->box()->setCurrentItem( 0 );
	resetWidgets();
    }

    return 1;
}


// Add or remove the init info and nameChanged notifier of the scenes affected.
void uiDirLightDlg::updateInitInfo()
{
    TypeSet<int> newsceneids;
    visserv_->getChildIds( -1, newsceneids );
    
    // remove info for scene(s) removed
    for ( int idx = 0; idx < initinfo_.size(); idx++)
	if ( !newsceneids.isPresent( initinfo_[idx].sceneid_ ) )
	{
	    mDynamicCastGet(visSurvey::Scene*,scene,
		    visserv_->getObject(initinfo_[idx].sceneid_));
	    if ( scene )
		scene->nameChanged.remove(
			mCB(this,uiDirLightDlg,sceneNameChangedCB) );
	    initinfo_.remove( idx );
	    if ( idx != initinfo_.size()-1 )
	        idx--;	// items would have moved up by the removal
	}    
    
    // append new info for scene(s) added
    const int size = initinfo_.size();
    for ( int newidx = 0; newidx < newsceneids.size(); newidx++)
    {
	int idx;
	bool present = false;

	for ( idx = 0; idx < size; idx++)
	    if ( newsceneids[newidx] == initinfo_[idx].sceneid_ )
	    {
		present = true;
		break;
	    }

	if ( !present )
	{
	    InitInfoType it;
	    it.sceneid_ = newsceneids[newidx];
	    // actual values will be got from Scene later by setWidgets
	    initinfo_.add( it );

	    mDynamicCastGet(visSurvey::Scene*,scene,
		    visserv_->getObject(it.sceneid_));
	    if ( scene )
		scene->nameChanged.notify(
			mCB(this,uiDirLightDlg,sceneNameChangedCB) );
	}
    }
}


// Copy values from widgets to init info data structure.
void uiDirLightDlg::saveInitInfo()
{
    bool saveall = scenefld_->box()->currentItem() == 0;

    for ( int idx = 0; idx < initinfo_.size(); idx++ )
    {
	bool dosave = saveall || idx == scenefld_->box()->currentItem()-1;
	if ( !dosave ) continue;

        initinfo_[idx].azimuth_ = azimuthfld_->sldr()->getValue();
        initinfo_[idx].dip_ = dipfld_->sldr()->getValue();
        initinfo_[idx].intensity_ = intensityfld_->sldr()->getValue();
        initinfo_[idx].headonintensity_ = 
	    headonintensityfld_->sldr()->getValue();
        initinfo_[idx].ambintensity_ = ambintensityfld_->sldr()->getValue();
    }

    initlighttype_ = ( cameralightfld_->isChecked() ) ? 0 : 1;
}


// Reset widgets to the intial values for the current scene.
void uiDirLightDlg::resetWidgets()
{
    if ( initinfo_.size() > 0 )
    {
	int idx = scenefld_->box()->currentItem()-1;
	// If 'All' is selected, the of the first scene in the list are used.
	if ( idx < 0 )
	    idx = 0;

	azimuthfld_->sldr()->setValue( initinfo_[idx].azimuth_ );
	dipfld_->sldr()->setValue( initinfo_[idx].dip_ );
	intensityfld_->sldr()->setValue( initinfo_[idx].intensity_ );
	headonintensityfld_->sldr()->setValue( 
		initinfo_[idx].headonintensity_ );
	ambintensityfld_->sldr()->setValue( initinfo_[idx].ambintensity_ );
    }
}


// Set the values of the widgets from the scene.
void uiDirLightDlg::setWidgets( bool resetinitinfo )
{
    bool updateall = initinfo_.size() > 0 
		     && scenefld_->box()->currentItem() == 0;

    bool anySceneDone = false;

    for ( int idx = 0; idx < initinfo_.size(); idx++ )
    {
	bool doupd = updateall || idx == scenefld_->box()->currentItem()-1;
	if ( !doupd ) continue;

	// head on light
	headonintensityfld_->sldr()->setValue( getHeadOnLight( idx ) );
        if ( resetinitinfo )
	    initinfo_[idx].headonintensity_ = 
		headonintensityfld_->sldr()->getValue();

	// ambient intensity
	ambintensityfld_->sldr()->setValue( getAmbientLight( idx ) );
        if ( resetinitinfo )
	    initinfo_[idx].ambintensity_ = 
		ambintensityfld_->sldr()->getValue();

	// directional light
        visBase::DirectionalLight* dl = getDirLight( idx );
        if ( !dl )
        {
   	    if ( resetinitinfo )
	        initinfo_[idx].reset( false );
	    continue;
        }

        float x = dl->direction( 0 );
        float y = dl->direction( 1 );
        float z = dl->direction( 2 );
        float dip = Angle::convert( Angle::Rad, asin( z ), Angle::Deg );
	dip += 180;  // offset for observed deviation
        float azimuth = Angle::convert( Angle::Rad, acos( x / cos( dip ) ),
		Angle::UsrDeg );

	if ( pd_ )
   	    pd_->setValues( azimuth, dip );

        if ( resetinitinfo )
        {
    	    initinfo_[idx].azimuth_ = azimuth;
	    initinfo_[idx].dip_ = dip;
	    initinfo_[idx].intensity_ = dl->intensity() * 100;
	}

	if ( !anySceneDone )
	{
	    azimuthfld_->sldr()->setValue( azimuth );
            dipfld_->sldr()->setValue( dip );
            intensityfld_->sldr()->setValue( dl->intensity() * 100 );
   	    anySceneDone = true;
	}
    }
}


visBase::DirectionalLight* uiDirLightDlg::getDirLight( int sceneidx ) const
{
    mDynamicCastGet(visSurvey::Scene*,scene,
		    visBase::DM().getObject( initinfo_[sceneidx].sceneid_));
    return (scene) ? scene->getDirectionalLight() : 0;
}


void uiDirLightDlg::setDirLight()
{
    if  ( !initinfo_.size() )
	return;

    validateInput();

    const bool lightall = scenefld_->box()->currentItem()==0;
    
    for ( int idx = 0; idx < initinfo_.size(); idx++ )
    {
	bool dolight = lightall || idx == scenefld_->box()->currentItem()-1;
	if ( !dolight ) continue;

	mDynamicCastGet(visSurvey::Scene*,scene,
			visBase::DM().getObject(initinfo_[idx].sceneid_));
	if ( !scene )
	    continue;

	float az_rad = Angle::convert( Angle::UsrDeg, 
		azimuthfld_->sldr()->getValue(), Angle::Rad );
	float dip_rad = Angle::convert( Angle::Deg,
		dipfld_->sldr()->getValue() - 180, Angle::Rad );
	  // offset for observed deviation

	float x = cos( az_rad ) * cos( dip_rad );
	float y = sin( az_rad ) * cos( dip_rad );
       	float z = sin (dip_rad );

	if ( !getDirLight( idx ) )
	{
	    RefMan<visBase::DirectionalLight> dl =
		visBase::DirectionalLight::create();
	    scene->setDirectionalLight( *dl );
	}

	RefMan<visBase::DirectionalLight> dl = getDirLight( idx );

	dl->setDirection( x, y, z ); 
 	dl->setIntensity( intensityfld_->sldr()->getValue() / 100 );
    }
}


float uiDirLightDlg::getAmbientLight( int sceneidx ) const
{
    mDynamicCastGet(visSurvey::Scene*,scene,
		    visBase::DM().getObject( initinfo_[sceneidx].sceneid_));
    return (scene) ? scene->ambientLight() * 100 : 0;
}


void uiDirLightDlg::setAmbientLight()
{
    if  ( !initinfo_.size() )
	return;

    float intensity = ambintensityfld_->sldr()->getValue();
    if ( ( intensity < 0 ) || ( intensity > 100 ) )
	ambintensityfld_->sldr()->setValue( 100 );

    intensity = ambintensityfld_->sldr()->getValue() / 100 ;
    const bool lightall = scenefld_->box()->currentItem()==0;
    
    for ( int idx = 0; idx < initinfo_.size(); idx++ )
    {
	bool dolight = lightall || idx == scenefld_->box()->currentItem()-1;
	if ( !dolight ) continue;

	mDynamicCastGet(visSurvey::Scene*,scene,
			visBase::DM().getObject(initinfo_[idx].sceneid_));
	if ( !scene )
	    continue;

	scene->setAmbientLight( intensity );
    }
}


float uiDirLightDlg::getHeadOnLight( int sceneidx ) const
{
    mDynamicCastGet(visSurvey::Scene*,scene,
		    visBase::DM().getObject(initinfo_[sceneidx].sceneid_));
    float hol = (scene) ? visserv_->sendGetHeadOnIntensityEvent( 
	    		initinfo_[sceneidx].sceneid_ ) * 100 : 0;
    if ( hol < 0 )
	// scene not yet up
	hol = mInitHeadOnIntensity;
    return hol;
}


void uiDirLightDlg::setHeadOnLight()
{
    if  ( !initinfo_.size() )
	return;

    float intensity = getHeadOnIntensity();
    const bool lightall = scenefld_->box()->currentItem()==0;
    
    for ( int idx = 0; idx < initinfo_.size(); idx++ )
    {
	bool dolight = lightall || idx == scenefld_->box()->currentItem()-1;
	if ( !dolight ) continue;

	mDynamicCastGet(visSurvey::Scene*,scene,
			visBase::DM().getObject(initinfo_[idx].sceneid_));
	if ( !scene )
	    continue;

	visserv_->sendSetHeadOnIntensityEvent( initinfo_[idx].sceneid_, 
		intensity );
    }
}


void uiDirLightDlg::turnOnDirLight( bool turnOn )
{
    // true : turns on directional light and turns off headon light
    // false: the other way around
    if ( turnOn )
    {
	intensityfld_->sldr()->setValue( mInitIntensity );
	headonintensityfld_->sldr()->setValue( 0 );
    }
    else
    {
        intensityfld_->sldr()->setValue( 0 );
        headonintensityfld_->sldr()->setValue( mInitHeadOnIntensity );
    }

    setDirLight();
    setHeadOnLight();
}


void uiDirLightDlg::showWidgets( bool showAll )
{
    if ( scenefld_ )
        scenefld_->display( showAll );
    azimuthfld_->setSensitive( showAll );
    dipfld_->setSensitive( showAll );
    intensityfld_->setSensitive ( showAll );
    if ( pd_ )
	pd_->setSensitive( showAll );
    headonintensityfld_->setSensitive ( showAll );
    ambintensityfld_->setSensitive ( showAll );
}


void uiDirLightDlg::validateInput()
{
    const float az = azimuthfld_->sldr()->getValue();
    if ( ( az < 0 ) || ( az > 360 ) )
	azimuthfld_->sldr()->setValue( mInitAzimuth );
    
    const float dip = dipfld_->sldr()->getValue();
    if ( ( dip < 0 ) || ( dip > 90 ) )
	dipfld_->sldr()->setValue( mInitDip );
    
    const float intensity = intensityfld_->sldr()->getValue();
    if ( ( intensity < 0 ) || ( intensity > 100 ) )
	intensityfld_->sldr()->setValue( mInitIntensity );
}


bool uiDirLightDlg::isInSync()
{
    if ( pd_ )
    {
	float az, dip;
	pd_->getValues( &az, &dip );
	if ( az != azimuthfld_->sldr()->getValue() ||
	     dip != dipfld_->sldr()->getValue() )
	    return false;
    }

    return true;
}


bool uiDirLightDlg::acceptOK( CallBacker* )
{
    if ( initinfo_.size() > 0 )
    {
        azimuthfld_->processInput();
        dipfld_->processInput();
        intensityfld_->processInput();
        headonintensityfld_->processInput();
        ambintensityfld_->processInput();
/*        setDirLight();
        setHeadOnLight();*/
	turnOnDirLight( scenelightfld_->isChecked() );
        saveInitInfo();
    }
    return true;
}


bool uiDirLightDlg::rejectOK( CallBacker* )
{
    resetWidgets();
/*    setDirLight();
    setHeadOnLight();*/
    if ( initlighttype_ )
	scenelightfld_->click();
    else
	cameralightfld_->click();

    return true;
}


void uiDirLightDlg::lightSelChangedCB( CallBacker* c )
{
  static bool pdshown;
 
  mDynamicCastGet(uiRadioButton*,but,c);
  bool showControls = ( but == cameralightfld_ ) ? false : true;

  // Save visibility of polar diagram dialog to restore when the scene light
  // is chosen the next time.
  if ( !showControls )
      pdshown = pddlg_ && !pddlg_->isHidden();

  scenefld_->display( showControls );
  azimuthfld_->display( showControls );
  dipfld_->display( showControls );
  showpdfld_->display( showControls );
  if ( showControls && pdshown )
      pddlg_->show();
  else 
      pddlg_->close();
  ambintensityfld_->display( showControls );
  turnOnDirLight( showControls );
}


void uiDirLightDlg::sceneSelChangedCB( CallBacker* )
{
    setWidgets( false );
}


void uiDirLightDlg::fieldChangedCB( CallBacker* c )
{
    if ( !pd_ )
    	setDirLight();

    // do the work only if this is not called by setValue of polarDiagramCB
    else if ( !pd_->hasFocus() )
    {
	pd_->setValues( azimuthfld_->sldr()->getValue(), 
		dipfld_->sldr()->getValue() );
        setDirLight();
    }
}


void uiDirLightDlg::polarDiagramCB( CallBacker* )
{
    float azimuth, dip;

    pd_->getValues( &azimuth, &dip );
    azimuthfld_->sldr()->setValue( azimuth );
    dipfld_->sldr()->setValue( dip );

    setDirLight();
}


void uiDirLightDlg::headOnChangedCB( CallBacker* )
{
    setHeadOnLight();
}


void uiDirLightDlg::ambientChangedCB( CallBacker* )
{
    setAmbientLight();
}


void uiDirLightDlg::nrScenesChangedCB( CallBacker* )
{
    if ( updateSceneSelector() )
    {
    	setWidgets( true );
        showWidgets( true );
    }
    else
	showWidgets( false );
}


void uiDirLightDlg::sceneNameChangedCB( CallBacker* cb )
{
    mDynamicCastGet(visSurvey::Scene*,scene,cb);
    
    int offset = ( initinfo_.size() > 1 ) ? 1 : 0;
    
    for ( int idx = 0; idx < initinfo_.size(); idx++ )
	if ( scene->id() == initinfo_[idx].sceneid_ )
	    scenefld_->box()->setItemText( idx+offset, scene->name() );
}


void uiDirLightDlg::activeSceneChangedCB( CallBacker* )
{
    // to do: bring that ID to focus
}


void uiDirLightDlg::showPolarDiagramCB( CallBacker* )
{
    pddlg_->show();
}


uiDirLightDlg::InitInfo::InitInfo()
{
    sceneid_ = 0;
    reset();
}


void uiDirLightDlg::InitInfo::reset( bool resetheadonval )
{
    azimuth_ = mInitAzimuth;
    dip_ = mInitDip;
    intensity_ = mInitIntensity;
    if ( resetheadonval )
        headonintensity_ = mInitHeadOnIntensity;
    ambintensity_ = mInitAmbIntensity;
}


uiDirLightDlg::InitInfo& uiDirLightDlg::InitInfo::operator = ( 
	const InitInfo& it )
{
    sceneid_ = it.sceneid_;
    azimuth_ = it.azimuth_;
    dip_ = it.dip_;
    intensity_ = it.intensity_;
    headonintensity_ = it.headonintensity_;
    ambintensity_ = it.ambintensity_;
    return *this;
}


bool uiDirLightDlg::InitInfo::operator == ( const InitInfo& it ) const
{
    return ( (sceneid_ == it.sceneid_) && (azimuth_ == it.azimuth_)
	     && (dip_ == it.dip_) && (intensity_ == it.intensity_)
	     && (headonintensity_ == it.headonintensity_)
	     && (ambintensity_ == it.ambintensity_) ) ? true : false;
}


bool uiDirLightDlg::InitInfo::operator != ( const InitInfo& it ) const
{
    return ( (sceneid_ != it.sceneid_) || (azimuth_ != it.azimuth_)
	     || (dip_ != it.dip_) || (intensity_ != it.intensity_)
	     || (headonintensity_ != it.headonintensity_)
	     || (ambintensity_ != it.ambintensity_) ) ? true : false;
}

