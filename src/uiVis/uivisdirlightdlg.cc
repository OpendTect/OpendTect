/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Sep 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uivisdirlightdlg.cc,v 1.5 2009-10-02 15:46:26 cvskarthika Exp $";

#include "uivisdirlightdlg.h"

#include "math.h"
#include "survinfo.h"
#include "vislight.h"
#include "uislider.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "uigeninput.h"
#include "uiseparator.h"
#include "visdataman.h"
#include "vistransmgr.h"
#include "vissurvscene.h"
//#include "uiodscenemgr.h"
#include "uivispartserv.h"

uiDirLightDlg::uiDirLightDlg( uiParent* p, uiVisPartServer* visserv )
    : uiDialog(p,
	       uiDialog::Setup("Directional light",
		   "Set directional light properties", mNoHelpID).modal(false))
	       // to do: specify proper help ID
    , visserv_(visserv)
    , scenefld_(0)
    , azimuthfld_(0)
    , dipfld_(0)
    , intensityfld_(0)
    , headonintensityfld_(0)
    , valchgd_(false)      
    , pd_(new uiPolarDiagram(this))
    , initazimuthval_(0)
    , initdipval_(0)
    , initintensityval_(100)
    , initheadonval_(100)
{
    scenefld_ = new uiLabeledComboBox( this, "Apply light to" );
    scenefld_->box()->selectionChanged.notify( 
					mCB(this,uiDirLightDlg,sceneSel) );

    int scenesavl = updateSceneSelector();

    const CallBack chgCB ( mCB(this,uiDirLightDlg,fieldChangedCB) );

    azimuthfld_ = new uiSliderExtra( this,
      uiSliderExtra::Setup("Azimuth (degrees)").withedit(true).nrdec(1).
      logscale(false), "Azimuth slider" );
    azimuthfld_->attach( alignedBelow, scenefld_ );
    azimuthfld_->sldr()->setMinValue( 0 );
    azimuthfld_->sldr()->setMaxValue( 360 );
    azimuthfld_->sldr()->setStep( 5 );
    azimuthfld_->sldr()->setValue( initazimuthval_ );
    azimuthfld_->sldr()->valueChanged.notify( chgCB );

    dipfld_ = new uiSliderExtra( this,
      uiSliderExtra::Setup("Dip (degrees)").withedit(true).nrdec(1).logscale(
	  false), "Dip slider" );
    dipfld_->attach( alignedBelow, azimuthfld_ );
    dipfld_->sldr()->setMinValue( 0 );
    dipfld_->sldr()->setMaxValue( 90 );
    dipfld_->sldr()->setStep( 5 );
    dipfld_->sldr()->setValue( initdipval_ );
    dipfld_->sldr()->valueChanged.notify( chgCB ); 

    intensityfld_ = new uiSliderExtra( this,
	    uiSliderExtra::Setup("Intensity (percentage)").withedit(true).
	    		         nrdec(1).logscale(false), "Intensity slider" );
    intensityfld_->attach( alignedBelow, dipfld_ );
    intensityfld_->sldr()->setMinValue( 0 );
    intensityfld_->sldr()->setMaxValue( 100 );
    intensityfld_->sldr()->setStep( 5 );
    intensityfld_->sldr()->setValue( initintensityval_ );
    intensityfld_->sldr()->valueChanged.notify( chgCB ); 

    pd_->attach( alignedBelow, intensityfld_ );

    uiSeparator* sep = new uiSeparator( this, "HSep", true );
    sep->attach( stretchedBelow, pd_ );
	    
    headonintensityfld_ = new uiSliderExtra( this,
	    uiSliderExtra::Setup("Intensity of camera light (percentage)").
	    			 withedit(true).nrdec(1).logscale(false), 
				 "Camera light intensity slider" );
    headonintensityfld_->attach( centeredBelow, sep );
    headonintensityfld_->sldr()->setMinValue( 0 );
    headonintensityfld_->sldr()->setMaxValue( 100 );
    headonintensityfld_->sldr()->setStep( 5 );
    headonintensityfld_->sldr()->setValue( initheadonval_ );
    headonintensityfld_->sldr()->valueChanged.notify( 
	    mCB(this,uiDirLightDlg,headOnChangedCB) ); 

/*    appl_.sceneMgr().treeToBeAdded.notify( 
	    mCB(this,uiDirLightDlg,nrScenesChangedCB) );
    appl_.sceneMgr().sceneClosed.notify( 
	    mCB(this,uiDirLightDlg,nrScenesChangedCB) );
    appl_.sceneMgr().activeSceneChanged.notify( 
	    mCB(this,uiDirLightDlg,activeSceneChangedCB) );*/
}


uiDirLightDlg::~uiDirLightDlg()
{
/*    appl_.sceneMgr().treeToBeAdded.remove( 
	    mCB(this,uiDirLightDlg,nrScenesChangedCB) );
    appl_.sceneMgr().sceneClosed.remove( 
	    mCB(this,uiDirLightDlg,nrScenesChangedCB) );
    appl_.sceneMgr().activeSceneChanged.remove( 
	    mCB(this,uiDirLightDlg,activeSceneChangedCB) );*/
    delete pd_;
}


bool uiDirLightDlg::valueChanged() const
{
    return valchgd_;
}


void uiDirLightDlg::sceneSel( CallBacker* )
{
    updateWidgetValues( false );
}


int uiDirLightDlg::updateSceneSelector()
{
    visserv_->getChildIds( -1, sceneids_ );
    scenefld_->box()->empty();
    
    if ( sceneids_.size() == 0 )
    {
        scenefld_->label()->setText( "No scene!" );
	return 0;
    }

    if ( sceneids_.size() >= 1 )
    {
	BufferStringSet scenenms;
	if ( sceneids_.size() > 1 )
	    scenenms.add( "All" );
	for ( int idx=0; idx<sceneids_.size(); idx++ )
	{
	    mDynamicCastGet(visSurvey::Scene*,scene,
		    	    visserv_->getObject(sceneids_[idx]));
	    if ( scene )
		scenenms.add( scene->name() );
	}

	scenefld_->label()->setText( "Apply light to" );
	scenefld_->box()->addItems( scenenms );
	scenefld_->box()->setCurrentItem( 1 );
    }

    return 1;
}


void uiDirLightDlg::updateWidgetValues( bool reset )
{
    bool updateall = sceneids_.size() > 0 
		     && scenefld_->box()->currentItem() == 0;

    for ( int idx=0; idx<sceneids_.size(); idx++ )
    {
	bool doupd = updateall || idx == scenefld_->box()->currentItem()-1;
	if ( !doupd ) continue;

	headonintensityfld_->sldr()->setValue( getHeadOnLight( idx ) );
	if ( reset )
	    initheadonval_ = headonintensityfld_->sldr()->getValue();

        visBase::DirectionalLight* dl = getDirLight( idx );
        if ( !dl )
        {
   	    if ( reset )
	    {
	        initazimuthval_ = 0;
	        initdipval_ = 0;
    	        initintensityval_ = 100;
	    }
	    continue;
        }

        float x = dl->direction( 0 );
        float y = dl->direction( 1 );
        float z = dl->direction( 2 );
        float dip = asin( z );
        float azimuth = acos( x / cos( dip ) ) ;
        dip *= 180.0 / M_PI;
        azimuth *= 180.0 / M_PI;

        azimuthfld_->sldr()->setValue( azimuth );
        dipfld_->sldr()->setValue( dip );
        intensityfld_->sldr()->setValue( dl->intensity() * 100 );
	BufferString s;
	s = toString(intensityfld_->sldr()->getValue() );
	pErrMsg(s);

	pd_->setValues( azimuth, dip );

        if ( reset )
        {
    	    initazimuthval_ = azimuth;
	    initdipval_ = dip;
	    initintensityval_ = dl->intensity() * 100;
        }	
    }
}


void uiDirLightDlg::setDirLight()
{
    if  ( !sceneids_.size() )
	return;

    validateInput();

    const bool lightall = scenefld_->box()->currentItem()==0;
    
    for ( int idx=0; idx<sceneids_.size(); idx++ )
    {
	bool dolight = lightall || idx == scenefld_->box()->currentItem()-1;
	if ( !dolight ) continue;

	mDynamicCastGet(visSurvey::Scene*,scene,
			visBase::DM().getObject(sceneids_[idx]));
	if ( !scene )
	    continue;

	static const float deg2rad = M_PI / 180.0;

	float x = cos( azimuthfld_->sldr()->getValue()*deg2rad ) * 
	          cos( dipfld_->sldr()->getValue()*deg2rad );
	float y = sin( azimuthfld_->sldr()->getValue()*deg2rad ) * 
	          cos( dipfld_->sldr()->getValue()*deg2rad );
       	float z = sin (dipfld_->sldr()->getValue()*deg2rad );


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


visBase::DirectionalLight* uiDirLightDlg::getDirLight( int sceneidx ) const
{
    mDynamicCastGet(visSurvey::Scene*,scene,
		    visBase::DM().getObject(sceneids_[sceneidx]));
    return (scene) ? scene->getDirectionalLight() : 0;
}


void uiDirLightDlg::setHeadOnLight()
{
    if  ( !sceneids_.size() )
	return;

    float intensity = headonintensityfld_->sldr()->getValue();
    if ( ( intensity < 0 ) || ( intensity > 100 ) )
	headonintensityfld_->sldr()->setValue( 100 );
    intensity = headonintensityfld_->sldr()->getValue() / 100;

    const bool lightall = scenefld_->box()->currentItem()==0;
    
    for ( int idx=0; idx<sceneids_.size(); idx++ )
    {
	bool dolight = lightall || idx == scenefld_->box()->currentItem()-1;
	if ( !dolight ) continue;

	mDynamicCastGet(visSurvey::Scene*,scene,
			visBase::DM().getObject(sceneids_[idx]));
	if ( !scene )
	    continue;

	scene->setAmbientLight( intensity );
    }
}


float uiDirLightDlg::getHeadOnLight( int sceneidx ) const
{
    mDynamicCastGet(visSurvey::Scene*,scene,
		    visBase::DM().getObject(sceneids_[sceneidx]));
    return (scene) ? scene->ambientLight() * 100 : 0;
}


void uiDirLightDlg::validateInput()
{
    const float az = azimuthfld_->sldr()->getValue();
    if ( ( az < 0 ) || ( az > 360 ) )
	azimuthfld_->sldr()->setValue( 0 );
    
    const float dip = dipfld_->sldr()->getValue();
    if ( ( dip < 0 ) || ( dip > 90 ) )
	dipfld_->sldr()->setValue( 0 );
    
    const float intensity = intensityfld_->sldr()->getValue();
    if ( ( intensity < 0 ) || ( intensity > 100 ) )
	intensityfld_->sldr()->setValue( 100 );
}


bool uiDirLightDlg::acceptOK( CallBacker* )
{
    if ( !azimuthfld_ )
	return true;

    azimuthfld_->processInput();
    dipfld_->processInput();
    intensityfld_->processInput();
    headonintensityfld_->processInput();
    valchgd_ = ( ( azimuthfld_->sldr()->getValue() != initazimuthval_ ) 
	         || ( dipfld_->sldr()->getValue() != initdipval_ )
		 || ( intensityfld_->sldr()->getValue() != initintensityval_ ) )
	       ? true : false;

    if ( valchgd_ )
	setDirLight();

    initazimuthval_ = azimuthfld_->sldr()->getValue();
    initdipval_ = dipfld_->sldr()->getValue();
    initintensityval_ = intensityfld_->sldr()->getValue();

    if ( headonintensityfld_->sldr()->getValue() != initheadonval_ )
    {
	valchgd_ = true;
	setHeadOnLight();
        initheadonval_ = headonintensityfld_->sldr()->getValue();
    }

    return true;
}


bool uiDirLightDlg::rejectOK( CallBacker* )
{
    valchgd_ = false;
    
    azimuthfld_->sldr()->setValue( initazimuthval_ );
    dipfld_->sldr()->setValue( initdipval_ );
    intensityfld_->sldr()->setValue( initintensityval_ );
    pd_->setValues( initazimuthval_, initdipval_ );
    headonintensityfld_->sldr()->setValue( initheadonval_ );

    setDirLight();
    setHeadOnLight();
    return true;
}


void uiDirLightDlg::fieldChangedCB( CallBacker* )
{
    setDirLight();
    if ( pd_ )
	pd_->setValues( azimuthfld_->sldr()->getValue(),
		dipfld_->sldr()->getValue() );
}


void uiDirLightDlg::headOnChangedCB( CallBacker* )
{
    setHeadOnLight();
}


void uiDirLightDlg::nrScenesChangedCB( CallBacker* )
{
    if ( updateSceneSelector() )
    {
    	updateWidgetValues( true );
        showWidgets( true );
    }
    else
	showWidgets( false );
}


void uiDirLightDlg::activeSceneChangedCB( CallBacker* )
{
    // bring that ID to focus
}


void uiDirLightDlg::showWidgets( bool showAll )
{
    if ( scenefld_ )
        scenefld_->display( showAll );
    azimuthfld_->setSensitive( showAll );
    dipfld_->setSensitive( showAll );
    intensityfld_->setSensitive ( showAll );
    pd_->setSensitive( showAll );
    headonintensityfld_->setSensitive ( showAll );
}


float uiDirLightDlg::getHeadOnIntensity() const
{
    return 0;
}


void uiDirLightDlg::setHeadOnIntensity(float)
{
}




