/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Sep 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uivisdirlightdlg.cc,v 1.4 2009-09-24 09:53:20 cvskarthika Exp $";

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

    scenefld_ = new uiLabeledComboBox( this, "Apply light to" );
    scenefld_->box()->selectionChanged.notify( 
					mCB(this,uiDirLightDlg,sceneSel) );

    int scenesavl = updateSceneSelector();

    const CallBack chgCB ( mCB(this,uiDirLightDlg,fieldChangedCB) );

    azimuthsldrfld_ = new uiSliderExtra( this,
      uiSliderExtra::Setup("Azimuth (degrees)").withedit(true).nrdec(1).
      logscale(false), "Azimuth slider" );
    azimuthsldrfld_->attach( alignedBelow, scenefld_ );
    azimuthsldrfld_->sldr()->setMinValue( 0 );
    azimuthsldrfld_->sldr()->setMaxValue( 360 );
    azimuthsldrfld_->sldr()->setStep( 1 );
    azimuthsldrfld_->sldr()->setValue( 0 );
    azimuthsldrfld_->sldr()->valueChanged.notify( chgCB );

    dipsldrfld_ = new uiSliderExtra( this,
      uiSliderExtra::Setup("Dip (degrees)").withedit(true).nrdec(1).logscale(
	  false), "Dip slider" );
    dipsldrfld_->attach( alignedBelow, azimuthsldrfld_ );
    dipsldrfld_->sldr()->setMinValue( 0 );
    dipsldrfld_->sldr()->setMaxValue( 90 );
    dipsldrfld_->sldr()->setStep( 1 );
    dipsldrfld_->sldr()->setValue( 0 );
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
    updateWidgetValues( false );
}


int uiDirLightDlg::updateSceneSelector()
{
    TypeSet<int>                sceneids;

    visBase::DM().getIds( typeid(visSurvey::Scene), sceneids );

    if ( sceneids == sceneids_ )
    {
	sceneids_ = sceneids;
	return ( ( sceneids_.size() == 0 ) ? 0 : 1  );
    }
    
    sceneids_ = sceneids;
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
		    	    visBase::DM().getObject(sceneids_[idx]))
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

        visBase::DirectionalLight* dl = getDirLight( idx );
        if ( !dl )
        {
   	    if ( reset )
	    {
	        initazimuthval_ = 0;
	        initdipval_ = 0;
    	        initintensityval_ = 1;
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

        azimuthsldrfld_->sldr()->setValue( azimuth );
        dipsldrfld_->sldr()->setValue( dip );
        intensityfld_->setValue( dl->intensity() );

        if ( reset )
        {
    	    initazimuthval_ = azimuth;
	    initdipval_ = dip;
	    initintensityval_ = dl->intensity();
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

	static const float deg2rad = M_PI / 180.0;

	float x = cos( azimuthsldrfld_->sldr()->getValue()*deg2rad ) * 
	          cos( dipsldrfld_->sldr()->getValue()*deg2rad );
	float y = sin( azimuthsldrfld_->sldr()->getValue()*deg2rad ) * 
	          cos( dipsldrfld_->sldr()->getValue()*deg2rad );
       	float z = sin (dipsldrfld_->sldr()->getValue()*deg2rad );



	if ( !getDirLight( idx ) )
	{
	    RefMan<visBase::DirectionalLight> dl =
		visBase::DirectionalLight::create();
	    scene->setDirectionalLight( *dl );
	}

	RefMan<visBase::DirectionalLight> dl = getDirLight( idx );

	dl->setDirection( x, y, z ); 
 	dl->setIntensity( intensityfld_->getfValue() );
    }
}


visBase::DirectionalLight* uiDirLightDlg::getDirLight( int sceneidx ) const
{
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
    if ( updateSceneSelector() )
    {
    	updateWidgetValues( true );
        showWidgets( true );
    }
    else
	showWidgets( false );
    
    uiDialog::show();
}


void uiDirLightDlg::showWidgets( bool showAll )
{
    if ( scenefld_ )
        scenefld_->display( showAll );
    azimuthsldrfld_->setSensitive( showAll );
    dipsldrfld_->setSensitive( showAll );
    intensityfld_->setSensitive ( showAll );
}




