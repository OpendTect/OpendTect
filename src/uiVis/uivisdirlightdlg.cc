/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Sep 2009
________________________________________________________________________

-*/

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
#include "od_helpids.h"

#define mInitAzimuth		0
#define mInitDip		90
#define mInitIntensity		100
#define mInitCameraIntensity	100
#define mInitAmbIntensity	50

uiDirLightDlg::uiDirLightDlg( uiParent* p, uiVisPartServer* visserv )
    : uiDialog(p,uiDialog::Setup(tr("Light options"),
				 tr("Set light options"),
                                 mODHelpKey(mDirLightDlgHelpID) ).modal(false))
    , visserv_(visserv)
    , pd_(0)
    , pddlg_(new uiDialog(this, uiDialog::Setup(tr("Polar diagram"),
		    tr("Set azimuth and dip"), mNoHelpKey).modal(false)))
{
    pddlg_->setCtrlStyle( CloseOnly );
    pddlg_->postFinalise().notify( mCB(this,uiDirLightDlg,pdDlgDoneCB) );

    scenefld_ = new uiLabeledComboBox( this, tr("Apply light to") );
    scenefld_->attach( hCentered );

    switchfld_ = new uiGenInput( this, tr("Turn directional light"),
				 BoolInpSpec(false, tr("On"),
                                             tr("Off")));
    switchfld_->attach( alignedBelow, scenefld_ );
    switchfld_->valuechanged.notify( mCB(this,uiDirLightDlg,onOffChg) );
    uiSeparator* sep1 = new uiSeparator( this, "HSep" );
    sep1->attach( stretchedBelow, switchfld_ );

    uiGroup* cameralightgrp = new uiGroup( this,"Camera group" );
    cameralightgrp->attach( ensureBelow,switchfld_ );
    cameralightgrp->attach( leftBorder );
    cameradirintensityfld_ = new uiSlider( cameralightgrp,
	uiSlider::Setup( tr( "Light intensity (%)" ) ).
	withedit( true ).nrdec( 1 ).logscale( false ),
	"Camera light intensity slider" );
    cameradirintensityfld_->setMinValue( 0 );
    cameradirintensityfld_->setMaxValue( 100 );
    cameradirintensityfld_->setStep( 5 );

    cameralightgrp->setHAlignObj( cameradirintensityfld_ );
    uiLabel* cmlabel = new uiLabel(cameralightgrp,tr("Camera mounted light") );
    cmlabel->attach( centeredAbove,cameradirintensityfld_ );


    cameraambintensityfld_ = new uiSlider( cameralightgrp,
	uiSlider::Setup( tr( " Ambient intensity (%)" ) ).
	withedit( true ).nrdec( 1 ).logscale( false ),
	"Ambient light intensity slider" );
    cameraambintensityfld_->attach( ensureBelow,cameradirintensityfld_ );
    cameraambintensityfld_->attach( alignedWith,cameradirintensityfld_ );
    cameraambintensityfld_->setMinValue( 0 );
    cameraambintensityfld_->setMaxValue( 100 );
    cameraambintensityfld_->setStep( 5 );

    uiSeparator* sep2 = new uiSeparator( this,"HSep" );
    sep2->attach( stretchedBelow,cameralightgrp );

    uiGroup* directionallightgrp = new uiGroup(this,"Directional light group");
    directionallightgrp->attach( alignedBelow,cameralightgrp );

    dirintensityfld_ = new uiSlider( directionallightgrp,
	uiSlider::Setup( tr( "Light intensity (%)" ) ).
	withedit( true ).nrdec( 1 ).logscale( false ),
	"Intensity slider" );
    dirintensityfld_->setMinValue( 0 );
    dirintensityfld_->setMaxValue( 100 );
    dirintensityfld_->setStep( 5 );

    directionallightgrp->setHAlignObj( dirintensityfld_ );

    uiLabel* dirlabel=new uiLabel(directionallightgrp,tr("Directional light"));
    dirlabel->attach( centeredAbove,dirintensityfld_ );

    const CallBack chgCB( mCB( this,uiDirLightDlg,fieldChangedCB ) );

    dipfld_ = new uiSlider( directionallightgrp,
	uiSlider::Setup( tr( "Dip (degrees)" ) ).withedit( true )
	.nrdec( 0 ).logscale( false ),"Dip slider" );
    dipfld_->attach( alignedBelow,dirintensityfld_ );
    dipfld_->setMinValue( 0 );
    dipfld_->setMaxValue( 90 );
    dipfld_->setStep( 5 );

    azimuthfld_ = new uiDialExtra( directionallightgrp,
	uiDialExtra::Setup( tr( "Azimuth (degrees)" ) ).withedit( true ),
	"Azimuth slider" );
    azimuthfld_->attach( centeredBelow, dipfld_ );
    azimuthfld_->dial()->setWrapping( true );
    azimuthfld_->dial()->setMinValue( 0 );
    azimuthfld_->dial()->setMaxValue( 360 );
    azimuthfld_->dial()->setInterval( StepInterval<int>( 0,360,5 ) );
    azimuthfld_->setSpacing( 66 );

    showpdfld_ = new uiPushButton(
	directionallightgrp,tr( "Show polar diagram" ),false );
    showpdfld_->attach( centeredBelow,azimuthfld_ );

    InitInfoType initinfo;
    if ( updateSceneSelector() )
    {
	setWidgets( true );
	initinfo = initinfo_[ 0 ];
    }

    azimuthfld_->dial()->setValue( ( int ) initinfo.azimuth_ );
    azimuthfld_->dial()->valueChanged.notify( chgCB );
    dipfld_->setValue( initinfo.dip_ );
    dipfld_->valueChanged.notify( chgCB );
    dirintensityfld_->setValue( initinfo.dirintensity_ );
    dirintensityfld_->valueChanged.notify( chgCB );
    cameradirintensityfld_->setValue( initinfo.cameraintensity_ );
    cameradirintensityfld_->valueChanged.notify(
	mCB( this,uiDirLightDlg,cameraLightChangedCB ) );
    cameraambintensityfld_->setValue( initinfo.ambintensity_ );
    cameraambintensityfld_->valueChanged.notify(
	mCB( this,uiDirLightDlg,cameraAmbientChangedCB ) );
    scenefld_->box()->selectionChanged.notify(
	mCB( this,uiDirLightDlg,sceneSelChangedCB ) );
    visserv_->nrScenesChange().notify(
	mCB( this,uiDirLightDlg,nrScenesChangedCB ) );
    showpdfld_->activated.notify( mCB(this,uiDirLightDlg,showPolarDiagramCB) );

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

    azimuthfld_->display( turnedon );
    dipfld_->display( turnedon );
    dirintensityfld_->display( turnedon );
    showpdfld_->display( turnedon );

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
    pd_->setValues( mCast(float,azimuthfld_->dial()->getValue()),
		    dipfld_->getFValue() );
    pd_->valueChanged.notify( mCB(this, uiDirLightDlg, polarDiagramCB) );
}


float uiDirLightDlg::getDiffuseIntensity() const
{
    float diffuseintensity = cameradirintensityfld_->getFValue();
    if ( diffuseintensity < 0 || diffuseintensity>100 )
	cameradirintensityfld_->setValue( 100 );
    return cameradirintensityfld_->getFValue() / 100;
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
        scenefld_->label()->setText( tr("No scene!") );
	return false;
    }

    uiStringSet scenenms;
    if ( initinfo_.size()>1 )
	scenenms.add( uiStrings::sAll() );

    for ( int idx=0; idx<initinfo_.size(); idx++ )
    {
	mDynamicCastGet(visSurvey::Scene*,scene,
			visserv_->getObject(initinfo_[idx].sceneid_));
	if ( scene )
	{
	    scenenms.add( scene->uiName() );
	    scene->nameChanged.notify(
		    mCB(this,uiDirLightDlg,sceneNameChangedCB) );
	}
    }

    scenefld_->label()->setText( tr("Apply light to") );
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
	    scene->getDirectionalLight( )->turnOn(
		    initinfo_[idx].directlighton_ );
	}

	initinfo_.removeSingle( idx );
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
	it.dirintensity_ = scene->getDirectionalLight()->getDiffuse();
	it.dx_ = scene->getDirectionalLight()->direction( 0 );
	it.dy_ = scene->getDirectionalLight()->direction( 1 );
	it.dz_ = scene->getDirectionalLight()->direction( 2 );
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

        initinfo_[idx].azimuth_ = mCast(float, azimuthfld_->dial()->getValue());
	initinfo_[idx].dip_ = dipfld_->getFValue();
	initinfo_[ idx ].dirintensity_ = dirintensityfld_->getFValue();
	initinfo_[ idx ].cameraintensity_ =
	    cameradirintensityfld_->getFValue();
	initinfo_[idx].ambintensity_ = cameraambintensityfld_->getFValue();
    }

    //initlighttype_ = lighttypefld_->getBoolValue();
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
    dipfld_->setValue( initinfo_[idx].dip_ );
    dirintensityfld_->setValue( initinfo_[ idx ].dirintensity_ );
    cameradirintensityfld_->setValue(
	initinfo_[ idx ].cameraintensity_ );
    cameraambintensityfld_->setValue( initinfo_[idx].ambintensity_ );
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

	// diffuse intensity
	cameradirintensityfld_->setValue( getCameraLightIntensity(idx) );
        if ( resetinitinfo )
	    initinfo_[idx].cameraintensity_ =
		cameradirintensityfld_->getFValue();

	// ambient intensity
	cameraambintensityfld_->setValue( getCameraAmbientIntensity( idx ) );
        if ( resetinitinfo )
	    initinfo_[idx].ambintensity_ =
		cameraambintensityfld_->getFValue();

	// directional light
        visBase::Light* dl = getDirLight( idx );
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
	    initinfo_[idx].dirintensity_ = dl->getDiffuse() * 100;
	}

	if ( !anySceneDone )
	{
	    azimuthfld_->dial()->setValue( (int)azimuth );
	    dipfld_->setValue( dip );
	    dirintensityfld_->setValue( dl->getDiffuse() * 100 );
	    anySceneDone = true;
	}
    }
}


visBase::Light* uiDirLightDlg::getDirLight( int sceneidx ) const
{
    mDynamicCastGet(visSurvey::Scene*,scene,
		    visBase::DM().getObject( initinfo_[sceneidx].sceneid_));
    return ( scene ) ? scene->getDirectionalLight() : 0;
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
		dipfld_->getFValue() - 180, Angle::Rad );
	  // offset for observed deviation

	float x = cos( az_rad ) * cos( dip_rad );
	float y = sin( az_rad ) * cos( dip_rad );
	float z = sin (dip_rad );

	RefMan<visBase::Light> dl = getDirLight( idx );

	// swap the direction sign let it towards to light source
	dl->setDirection( -x, -y, -z );
	dl->setDiffuse( dirintensityfld_->getFValue() / 100 );
    }
}


float uiDirLightDlg::getCameraAmbientIntensity( int sceneidx ) const
{
    mDynamicCastGet(visSurvey::Scene*,scene,
		    visBase::DM().getObject( initinfo_[sceneidx].sceneid_));
    return (scene) ? scene->getCameraAmbientLight() * 100 : 0;
}


void uiDirLightDlg::cameraAmbientChangedCB( CallBacker* )
{
    if  ( !initinfo_.size() )
	return;

    float intensity = cameraambintensityfld_->getFValue();
    if ( intensity<0 || intensity>100 )
	cameraambintensityfld_->setValue(100);

    intensity = cameraambintensityfld_->getFValue() / 100 ;
    const bool lightall = scenefld_->box()->currentItem()==0;

    for ( int idx=0; idx<initinfo_.size(); idx++ )
    {
	bool dolight = lightall || idx==scenefld_->box()->currentItem()-1;
	if ( !dolight ) continue;

	mDynamicCastGet(visSurvey::Scene*,scene,
			visBase::DM().getObject(initinfo_[idx].sceneid_));
	if ( scene )
	    scene->setCameraAmbientLight( intensity );
    }
}


float uiDirLightDlg::getCameraLightIntensity( int sceneidx ) const
{
    mDynamicCastGet(visSurvey::Scene*,scene,
		    visBase::DM().getObject(initinfo_[sceneidx].sceneid_));

    return scene ? scene->getCameraLightIntensity() * 100 : 0;
}


void uiDirLightDlg::setCameraLightIntensity()
{
    if  ( !initinfo_.size() )
	return;

    float diffuseintensity = getDiffuseIntensity();
    const bool lightall = scenefld_->box()->currentItem()==0;

    for ( int idx=0; idx<initinfo_.size(); idx++ )
    {
	bool dolight = lightall || idx == scenefld_->box()->currentItem()-1;
	if ( !dolight ) continue;

	mDynamicCastGet(visSurvey::Scene*,scene,
			visBase::DM().getObject(initinfo_[idx].sceneid_));
	if ( !scene )
	    continue;
	scene->setCameraLightIntensity( diffuseintensity );
    }
}


void uiDirLightDlg::showWidgets( bool showAll )
{
    if ( scenefld_ )
        scenefld_->display( showAll );
    azimuthfld_->setSensitive( showAll );
    dipfld_->setSensitive( showAll );
    dirintensityfld_->setSensitive( showAll );
    if ( pd_ )
	pd_->setSensitive( showAll );
    cameradirintensityfld_->setSensitive ( showAll );
    cameraambintensityfld_->setSensitive ( showAll );
}


void uiDirLightDlg::validateInput()
{
    const float az = mCast( float, azimuthfld_->dial()->getValue() );
    if ( az<0 || az>360 )
	azimuthfld_->dial()->setValue( mInitAzimuth );

    const float dip = dipfld_->getFValue();
    if ( dip<0 || dip>90 )
	dipfld_->setValue( mInitDip );

    const float intensity = dirintensityfld_->getFValue();
    if ( intensity<0 || intensity>100 )
	dirintensityfld_->setValue( mInitIntensity );
}


bool uiDirLightDlg::isInSync()
{
    if ( pd_ )
    {
	float az, dip;
	pd_->getValues( &az, &dip );
	if ( az != azimuthfld_->dial()->getValue() ||
		dip != dipfld_->getFValue() )
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
	dirintensityfld_->processInput();
        cameradirintensityfld_->processInput();
        cameraambintensityfld_->processInput();

	setDirLight();
	setCameraLightIntensity();
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
	initinfo_[ idx ].directlighton_ = scene->getDirectionalLight()->isOn();
	initinfo_[ idx ].dirintensity_=
	    scene->getDirectionalLight()->getDiffuse();
	initinfo_[ idx ].dx_ = scene->getDirectionalLight()->direction( 0 );
	initinfo_[ idx ].dy_ = scene->getDirectionalLight()->direction( 1 );
	initinfo_[ idx ].dz_ = scene->getDirectionalLight()->direction( 2 );
    }

    return true;
}


bool uiDirLightDlg::rejectOK( CallBacker* )
{
    resetWidgets();

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
	    visBase::Light& dl = *scene->getDirectionalLight();
	    dl.turnOn(initinfo_[idx].directlighton_);
	    dl.setDirection( initinfo_[idx].dx_, initinfo_[idx].dy_,
			     initinfo_[idx].dz_ );
	    dl.setDiffuse( initinfo_[idx].dirintensity_ );
	    if ( !forall || initinfo_[idx].directlighton_ )
		lighton = initinfo_[idx].directlighton_;
	}
    }

    switchfld_->setValue( lighton );

    return true;
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
	pd_->setValues( mCast(float,azimuthfld_->dial()->getValue()),
		dipfld_->getFValue() );
        setDirLight();
    }
}


void uiDirLightDlg::polarDiagramCB( CallBacker* )
{
    float azimuth, dip;
    pd_->getValues( &azimuth, &dip );
    azimuthfld_->dial()->setValue( (int)azimuth );
    dipfld_->setValue( dip );
    setDirLight();
}


void uiDirLightDlg::cameraLightChangedCB( CallBacker* )
{
    setCameraLightIntensity();
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
	    scenefld_->box()->setItemText( idx+offset, scene->uiName() );
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


void uiDirLightDlg::InitInfo::reset( bool resetcameralightval )
{
    azimuth_ = mInitAzimuth;
    dip_ = mInitDip;
    dirintensity_ = mInitIntensity;
    if ( resetcameralightval )
	cameraintensity_ = mInitCameraIntensity;
    ambintensity_ = mInitAmbIntensity;
}


uiDirLightDlg::InitInfo&
    uiDirLightDlg::InitInfo::operator= ( const InitInfo& it )
{
    directlighton_ = it.directlighton_;
    sceneid_ = it.sceneid_;
    azimuth_ = it.azimuth_;
    dip_ = it.dip_;
    dirintensity_ = it.dirintensity_;
    cameraintensity_ = it.cameraintensity_;
    ambintensity_ = it.ambintensity_;
    return *this;
}


bool uiDirLightDlg::InitInfo::operator== ( const InitInfo& it ) const
{
    return sceneid_==it.sceneid_ && azimuth_==it.azimuth_ &&
	   dip_==it.dip_ && dirintensity_==it.dirintensity_ &&
	   cameraintensity_ == it.cameraintensity_ &&
	   ambintensity_==it.ambintensity_ && directlighton_==it.directlighton_;
}


bool uiDirLightDlg::InitInfo::operator!= ( const InitInfo& it ) const
{
    return  sceneid_ != it.sceneid_ || azimuth_ != it.azimuth_ ||
	    dip_ != it.dip_ || dirintensity_ != it.dirintensity_ ||
	    cameraintensity_ != it.cameraintensity_ ||
	    ambintensity_ != it.ambintensity_ ||
	    directlighton_ != it.directlighton_;
}
