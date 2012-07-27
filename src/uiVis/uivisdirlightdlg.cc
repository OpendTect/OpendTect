/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Sep 2009
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uivisdirlightdlg.cc,v 1.35 2012-07-27 09:43:42 cvsjaap Exp $";

#include "uivisdirlightdlg.h"

#include "angles.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uidial.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uiseparator.h"
#include "visdataman.h"
#include "uislider.h"
#include "vislight.h"
#include "vissurvscene.h"
#include "uivispartserv.h"


#define mInitAzimuth		0
#define mInitDip		90
#define mInitIntensity		100
#define mInitHeadOnIntensity	100
#define mInitAmbIntensity	50

uiDirLightDlg::uiDirLightDlg( uiParent* p, uiVisPartServer* visserv )
    : uiDialog(p,uiDialog::Setup("Light properties",
				 "Set light properties","50.0.18").modal(false))
    , visserv_(visserv)
    , pd_(0)
    , pddlg_(new uiDialog(this, uiDialog::Setup("Polar diagram", 
		    "Set azimuth and dip", mNoHelpID).modal(false)))
{
    pddlg_->setCtrlStyle( LeaveOnly );
    pddlg_->postFinalise().notify( mCB(this,uiDirLightDlg,pdDlgDoneCB) );

    scenefld_ = new uiLabeledComboBox( this, "Apply light to" );
    scenefld_->attach( hCentered );

    switchfld_ = new uiGenInput( this, "Turn light", 
	    BoolInpSpec(false,"On","Off") );
    switchfld_->attach( alignedBelow, scenefld_ );
    switchfld_->valuechanged.notify( mCB(this,uiDirLightDlg,onOffChg) );

    uiSeparator* sep1 = new uiSeparator( this, "HSep", true );
    sep1->attach( stretchedBelow, switchfld_ );

    uiGroup* lightgrp = new uiGroup( this, "Light group" );
    lightgrp->attach( ensureBelow, sep1 );
    lightgrp->attach( leftBorder );

    lighttypefld_ = new uiGenInput( lightgrp, "Light type", BoolInpSpec(
		false,"Positioned at the camera","Relative to the scene") );
    lighttypefld_->valuechanged.notify( 
	    mCB(this,uiDirLightDlg,lightSelChangedCB) );

    lightgrp->setHAlignObj( lighttypefld_ );
	
    intensityfld_ = new uiSliderExtra( this,
	    uiSliderExtra::Setup("Intensity (%)").withedit(true).
	    	         nrdec(1).logscale(false), "Intensity slider" );
    intensityfld_->attach( alignedBelow, lightgrp );
    intensityfld_->sldr()->setMinValue( 0 );
    intensityfld_->sldr()->setMaxValue( 100 );
    intensityfld_->sldr()->setStep( 5 );

    headonintensityfld_ = new uiSliderExtra( this,
	    uiSliderExtra::Setup("Camera light intensity (%)").
	    			 withedit(true).nrdec(1).logscale(false), 
				 "Camera light intensity slider" );
    headonintensityfld_->attach( alignedBelow, lightgrp );
    headonintensityfld_->sldr()->setMinValue( 0 );
    headonintensityfld_->sldr()->setMaxValue( 100 );
    headonintensityfld_->sldr()->setStep( 5 );

    const CallBack chgCB( mCB(this,uiDirLightDlg,fieldChangedCB) );

    azimuthfld_ = new uiDialExtra( this, 
	    uiDialExtra::Setup("Azimuth (degrees)").withedit(true),
	    "Azimuth slider" );
    azimuthfld_->attach( centeredBelow, intensityfld_ );
    azimuthfld_->dial()->setWrapping( true );
    azimuthfld_->dial()->setMinValue( 0 );
    azimuthfld_->dial()->setMaxValue( 360 );
    azimuthfld_->dial()->setInterval( StepInterval<int>( 0, 360, 5 ) );

    dipfld_ = new uiSliderExtra( this,
	        uiSliderExtra::Setup("Dip (degrees)").withedit(true).nrdec(0).
	    	logscale(false), "Dip slider" );
    dipfld_->attach( centeredBelow, azimuthfld_ );
    dipfld_->sldr()->setMinValue( 0 );
    dipfld_->sldr()->setMaxValue( 90 );
    dipfld_->sldr()->setStep( 5 );

    showpdfld_ = new uiPushButton( this, "Show polar diagram", false );
    showpdfld_->attach( alignedBelow, dipfld_ );

    uiSeparator* sep2 = new uiSeparator( this, "HSep", true );
    sep2->attach( stretchedBelow, showpdfld_ );

    ambintensityfld_ = new uiSliderExtra( this,
	    	        uiSliderExtra::Setup("Ambient light intensity (%)").
	    			 withedit(true).nrdec(1).logscale(false), 
				 "Ambient light intensity slider" );
    ambintensityfld_->attach( centeredBelow, sep2 );
    ambintensityfld_->sldr()->setMinValue( 0 );
    ambintensityfld_->sldr()->setMaxValue( 100 );
    ambintensityfld_->sldr()->setStep( 5 );

    InitInfoType initinfo;
    if ( updateSceneSelector() )
    {
	setWidgets(true);
        initinfo = initinfo_[0];
    }

    azimuthfld_->dial()->setValue( (int)initinfo.azimuth_ );
    azimuthfld_->dial()->valueChanged.notify( chgCB );
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

    initlighttype_ = lighttypefld_->getBoolValue();
}


uiDirLightDlg::~uiDirLightDlg()
{
    removeSceneNotifiers();

    visserv_->nrScenesChange().remove(
	    mCB(this,uiDirLightDlg,nrScenesChangedCB) );

    delete pd_;
    
    pddlg_->close();
    delete pddlg_;
}


void uiDirLightDlg::show()
{
    uiDialog::show();
    onOffChg(0);
}


void uiDirLightDlg::onOffChg( CallBacker* cb )
{
    const bool turnedon = switchfld_->getBoolValue();
    const bool dirlight = turnedon && !lighttypefld_->getBoolValue();
   
    lighttypefld_->display( turnedon );
    azimuthfld_->display( dirlight );
    dipfld_->display( dirlight );
    intensityfld_->display( dirlight );
    headonintensityfld_->display( turnedon && lighttypefld_->getBoolValue() );
    showpdfld_->display( dirlight );
    
    const bool forall = initinfo_.size() && !scenefld_->box()->currentItem();
    for ( int idx=0; idx<initinfo_.size(); idx++ )
    {
	if ( !forall && idx!=scenefld_->box()->currentItem()-1 )
	    continue;
	
	mDynamicCastGet(visSurvey::Scene*,scene,
    		visBase::DM().getObject(initinfo_[idx].sceneid_));
    	if ( scene )
    	    scene->getDirectionalLight()->turnOn( turnedon );
    }
}


void uiDirLightDlg::pdDlgDoneCB( CallBacker* )
{
    if ( pd_ ) return;

    pd_ = new uiPolarDiagram( pddlg_ );
    pd_->setValues( azimuthfld_->dial()->getValue(), 
	    	    dipfld_->sldr()->getValue() );
    pd_->valueChanged.notify( mCB(this, uiDirLightDlg, polarDiagramCB) );
}


float uiDirLightDlg::getHeadOnIntensity() const
{
    float intensity = headonintensityfld_->sldr()->getValue();
    if ( intensity<0 || intensity>100 )
	headonintensityfld_->sldr()->setValue( 100 );
    return headonintensityfld_->sldr()->getValue() / 100;
}


void uiDirLightDlg::setHeadOnIntensity( float value )
{
    if ( value>=0 && value<=1.0 )
	headonintensityfld_->sldr()->setValue( value*100 );
}


void uiDirLightDlg::removeSceneNotifiers()
{
    for ( int idx=0; idx<initinfo_.size(); idx++ )
    {
	mDynamicCastGet(visSurvey::Scene*,scene,
		visserv_->getObject(initinfo_[idx].sceneid_));
	if ( scene )
	    scene->nameChanged.remove(
		   mCB(this,uiDirLightDlg,sceneNameChangedCB) );
    }
}


bool uiDirLightDlg::updateSceneSelector()
{
    updateInitInfo();
    scenefld_->box()->setEmpty();

    if ( !initinfo_.size() )
    {
        scenefld_->label()->setText( "No scene!" );
	return false;
    }

    BufferStringSet scenenms;
    if ( initinfo_.size()>1 )
	scenenms.add( "All" );

    for ( int idx=0; idx<initinfo_.size(); idx++ )
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

    return true;
}


void uiDirLightDlg::updateInitInfo()
{
    TypeSet<int> newsceneids;
    visserv_->getChildIds( -1, newsceneids );

    // remove info for scene(s) removed
    for (int idx=initinfo_.size()-1; idx>=0; idx--)
    {
	if ( newsceneids.isPresent( initinfo_[idx].sceneid_ ) )
	    continue;

	mDynamicCastGet(visSurvey::Scene*,scene,
		visserv_->getObject(initinfo_[idx].sceneid_));
	if ( scene )
	{
	    scene->nameChanged.remove(
		    mCB(this,uiDirLightDlg,sceneNameChangedCB) );
	    scene->getDirectionalLight()->turnOn( 
		    initinfo_[idx].directlighton_ );
	}

	initinfo_.remove( idx );
    }    
    
    // append new info for scene(s) added
    const int size = initinfo_.size();
    for ( int newidx=0; newidx<newsceneids.size(); newidx++ )
    {
	bool present = false;
	for ( int idx=0; idx<size; idx++ )
	{
	    if ( newsceneids[newidx]==initinfo_[idx].sceneid_ )
	    {
		present = true;
		break;
	    }
	}

	if ( present ) continue;
	
	mDynamicCastGet(visSurvey::Scene*,scene,
		visserv_->getObject(newsceneids[newidx]));
	if ( !scene ) continue;
	scene->nameChanged.notify(mCB(this,uiDirLightDlg,sceneNameChangedCB));

	InitInfoType it;
	it.sceneid_ = newsceneids[newidx];
	it.directlighton_ = scene->getDirectionalLight()->isOn();
	it.intensity_ = scene->getDirectionalLight()->intensity();
	it.dx_ = scene->getDirectionalLight()->direction(0);
	it.dy_ = scene->getDirectionalLight()->direction(1);
	it.dz_ = scene->getDirectionalLight()->direction(2);
	initinfo_.add( it );
    }
    setlightSwitch();
}


// Copy values from widgets to init info data structure.
void uiDirLightDlg::saveInitInfo()
{
    const bool saveall = scenefld_->box()->currentItem() == 0;
    for ( int idx=0; idx<initinfo_.size(); idx++ )
    {
	const bool dosave = saveall || idx==scenefld_->box()->currentItem()-1;
	if ( !dosave ) continue;

        initinfo_[idx].azimuth_ = azimuthfld_->dial()->getValue();
        initinfo_[idx].dip_ = dipfld_->sldr()->getValue();
        initinfo_[idx].intensity_ = intensityfld_->sldr()->getValue();
        initinfo_[idx].headonintensity_ = 
	    headonintensityfld_->sldr()->getValue();
        initinfo_[idx].ambintensity_ = ambintensityfld_->sldr()->getValue();
    }

    initlighttype_ = lighttypefld_->getBoolValue();
}


// Reset widgets to the intial values for the current scene.
void uiDirLightDlg::resetWidgets()
{
    if ( !initinfo_.size() )
	return;

    int idx = scenefld_->box()->currentItem()-1;
    // If 'All' is selected, the of the first scene in the list are used.
    if ( idx < 0 )
	idx = 0;

    azimuthfld_->dial()->setValue( (int)initinfo_[idx].azimuth_ );
    dipfld_->sldr()->setValue( initinfo_[idx].dip_ );
    intensityfld_->sldr()->setValue( initinfo_[idx].intensity_ );
    headonintensityfld_->sldr()->setValue( 
	    initinfo_[idx].headonintensity_ );
    ambintensityfld_->sldr()->setValue( initinfo_[idx].ambintensity_ );
}


// Set the values of the widgets from the scene.
void uiDirLightDlg::setWidgets( bool resetinitinfo )
{
    bool updateall = initinfo_.size() > 0 
		     && scenefld_->box()->currentItem() == 0;

    bool anySceneDone = false;

    for ( int idx=0; idx<initinfo_.size(); idx++ )
    {
	const bool doupd = updateall || idx==scenefld_->box()->currentItem()-1;
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
        float y mUnusedVar = dl->direction( 1 );
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
	    azimuthfld_->dial()->setValue( (int)azimuth );
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
    for ( int idx=0; idx<initinfo_.size(); idx++ )
    {
	bool dolight = lightall || idx == scenefld_->box()->currentItem()-1;
	if ( !dolight ) continue;

	mDynamicCastGet(visSurvey::Scene*,scene,
			visBase::DM().getObject(initinfo_[idx].sceneid_));
	if ( !scene )
	    continue;

	float az_rad = Angle::convert( Angle::UsrDeg, 
		(float) (azimuthfld_->dial()->getValue()), Angle::Rad );
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


void uiDirLightDlg::ambientChangedCB( CallBacker* )
{
    if  ( !initinfo_.size() )
	return;

    float intensity = ambintensityfld_->sldr()->getValue();
    if ( intensity<0 || intensity>100 )
	ambintensityfld_->sldr()->setValue(100);

    intensity = ambintensityfld_->sldr()->getValue() / 100 ;
    const bool lightall = scenefld_->box()->currentItem()==0;
    
    for ( int idx=0; idx<initinfo_.size(); idx++ )
    {
	bool dolight = lightall || idx==scenefld_->box()->currentItem()-1;
	if ( !dolight ) continue;

	mDynamicCastGet(visSurvey::Scene*,scene,
			visBase::DM().getObject(initinfo_[idx].sceneid_));
	if ( scene )
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
    
    for ( int idx=0; idx<initinfo_.size(); idx++ )
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
    const float az = azimuthfld_->dial()->getValue();
    if ( az<0 || az>360 )
	azimuthfld_->dial()->setValue( mInitAzimuth );
    
    const float dip = dipfld_->sldr()->getValue();
    if ( dip<0 || dip>90 )
	dipfld_->sldr()->setValue( mInitDip );
    
    const float intensity = intensityfld_->sldr()->getValue();
    if ( intensity<0 || intensity>100 )
	intensityfld_->sldr()->setValue( mInitIntensity );
}


bool uiDirLightDlg::isInSync()
{
    if ( pd_ )
    {
	float az, dip;
	pd_->getValues( &az, &dip );
	if ( az != azimuthfld_->dial()->getValue() ||
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
    
	setDirLight();
    	setHeadOnLight();
        saveInitInfo();
    }

    const bool forall = initinfo_.size() && !scenefld_->box()->currentItem();
    for ( int idx=0; idx<initinfo_.size(); idx++ )
    {
	if ( !forall && idx!=scenefld_->box()->currentItem()-1 )
	    continue;
	
	mDynamicCastGet(visSurvey::Scene*,scene,
    		visBase::DM().getObject(initinfo_[idx].sceneid_));
    	if ( !scene ) continue;
	initinfo_[idx].directlighton_ = scene->getDirectionalLight()->isOn();
	initinfo_[idx].intensity_ = scene->getDirectionalLight()->intensity();
	initinfo_[idx].dx_ = scene->getDirectionalLight()->direction(0);
	initinfo_[idx].dy_ = scene->getDirectionalLight()->direction(1);
	initinfo_[idx].dz_ = scene->getDirectionalLight()->direction(2);
    }

    return true;
}


bool uiDirLightDlg::rejectOK( CallBacker* )
{
    resetWidgets();
    
    lighttypefld_->setValue(initlighttype_);
   
    bool lighton = false; 
    const bool forall = initinfo_.size() && !scenefld_->box()->currentItem();
    for ( int idx=0; idx<initinfo_.size(); idx++ )
    {
	if ( !forall && idx!=scenefld_->box()->currentItem()-1 )
	    continue;
	
	mDynamicCastGet(visSurvey::Scene*,scene,
    		visBase::DM().getObject(initinfo_[idx].sceneid_));
    	if ( scene )
	{
	    visBase::DirectionalLight& dl = *scene->getDirectionalLight();
	    dl.turnOn(initinfo_[idx].directlighton_);
	    dl.setDirection( initinfo_[idx].dx_, initinfo_[idx].dy_, 
		    	     initinfo_[idx].dz_ );
	    dl.setIntensity( initinfo_[idx].intensity_ );
	    if ( !forall || initinfo_[idx].directlighton_ )
		lighton = initinfo_[idx].directlighton_;
	}
    }

    switchfld_->setValue( lighton );

    return true;
}


void uiDirLightDlg::lightSelChangedCB( CallBacker* c )
{
    static bool pdshown;
    const bool dirlight = !lighttypefld_->getBoolValue();

    // Save visibility of polar diagram dialog to restore when the scene light
    // is chosen the next time.
    if ( dirlight )
	pdshown = pddlg_ && !pddlg_->isHidden();

    azimuthfld_->display( dirlight );
    dipfld_->display( dirlight );
    intensityfld_->display( dirlight );
    headonintensityfld_->display( !dirlight );
    showpdfld_->display( dirlight );
    if ( dirlight && pdshown )
	pddlg_->show();
    else 
	pddlg_->close();
    setDirLight();
    setHeadOnLight();
}


void uiDirLightDlg::sceneSelChangedCB( CallBacker* )
{
    setWidgets( false );
    setlightSwitch();
}
	

void uiDirLightDlg::setlightSwitch()
{
    bool lighton = false;    
    const bool forall = initinfo_.size() && !scenefld_->box()->currentItem();
    for ( int idx=0; idx<initinfo_.size(); idx++ )
    {
	if ( !forall && idx!=scenefld_->box()->currentItem()-1 )
	    continue;
	
	mDynamicCastGet(visSurvey::Scene*,scene,
    		visBase::DM().getObject(initinfo_[idx].sceneid_));
    	if ( scene && scene->getDirectionalLight()->isOn() )
	{
	    lighton = true;
	    break;
	}
    }

    switchfld_->setValue( lighton );
}


void uiDirLightDlg::fieldChangedCB( CallBacker* c )
{
    if ( !pd_ )
    	setDirLight();

    // do the work only if this is not called by setValue of polarDiagramCB
    else if ( !pd_->hasFocus() )
    {
	pd_->setValues( azimuthfld_->dial()->getValue(), 
		dipfld_->sldr()->getValue() );
        setDirLight();
    }
}


void uiDirLightDlg::polarDiagramCB( CallBacker* )
{
    float azimuth, dip;
    pd_->getValues( &azimuth, &dip );
    azimuthfld_->dial()->setValue( (int)azimuth );
    dipfld_->sldr()->setValue( dip );
    setDirLight();
}


void uiDirLightDlg::headOnChangedCB( CallBacker* )
{
    setHeadOnLight();
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
    const int offset = initinfo_.size()>1 ? 1 : 0;
    for ( int idx=0; idx<initinfo_.size(); idx++ )
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
    directlighton_ = false;
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


uiDirLightDlg::InitInfo&
    uiDirLightDlg::InitInfo::operator= ( const InitInfo& it )
{
    directlighton_ = it.directlighton_;
    sceneid_ = it.sceneid_;
    azimuth_ = it.azimuth_;
    dip_ = it.dip_;
    intensity_ = it.intensity_;
    headonintensity_ = it.headonintensity_;
    ambintensity_ = it.ambintensity_;
    return *this;
}


bool uiDirLightDlg::InitInfo::operator== ( const InitInfo& it ) const
{
    return sceneid_==it.sceneid_ && azimuth_==it.azimuth_ && 
	   dip_==it.dip_ && intensity_==it.intensity_ && 
	   headonintensity_==it.headonintensity_ && 
	   ambintensity_==it.ambintensity_ && directlighton_==it.directlighton_;
}


bool uiDirLightDlg::InitInfo::operator!= ( const InitInfo& it ) const
{
    return  sceneid_ != it.sceneid_ || azimuth_ != it.azimuth_ || 
	    dip_ != it.dip_ || intensity_ != it.intensity_ || 
	    headonintensity_ != it.headonintensity_ || 
	    ambintensity_ != it.ambintensity_ || 
	    directlighton_ != it.directlighton_;
}
