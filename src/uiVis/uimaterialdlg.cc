/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uimaterialdlg.cc,v 1.1 2002-04-04 16:07:29 nanne Exp $
________________________________________________________________________

-*/

#include "uimaterialdlg.h"
#include "vismaterial.h"
#include "uislider.h"

const int cMinVal = 0;
const int cMaxVal = 100;
const int cStepVal = 10;


uiMaterialDlg::uiMaterialDlg( uiParent* p, visBase::Material* mat )
	: uiDialog(p,"Material")
	, material(mat)
{
    ambslider = new uiLabeledSlider( this, "Ambience" );
    ambslider->sldr()->valueChanged.notify( 
	mCB(this,uiMaterialDlg,ambSliderMove) );

    diffslider = new uiLabeledSlider( this, "Diffuse color" );
    diffslider->sldr()->valueChanged.notify( 
	mCB(this,uiMaterialDlg,diffSliderMove) );
    diffslider->attach( alignedBelow, ambslider );

    specslider = new uiLabeledSlider( this, "Specular color" );
    specslider->sldr()->valueChanged.notify( 
	mCB(this,uiMaterialDlg,specSliderMove) );
    specslider->attach( alignedBelow, diffslider );

    emisslider = new uiLabeledSlider( this, "Emissive color" );
    emisslider->sldr()->valueChanged.notify( 
	mCB(this,uiMaterialDlg,emisSliderMove) );
    emisslider->attach( alignedBelow, specslider );

    shineslider = new uiLabeledSlider( this, "Shininess" );
    shineslider->sldr()->valueChanged.notify( 
	mCB(this,uiMaterialDlg,shineSliderMove) );
    shineslider->attach( alignedBelow, emisslider );

    transslider = new uiLabeledSlider( this, "Transparency" );
    transslider->sldr()->valueChanged.notify( 
	mCB(this,uiMaterialDlg,transSliderMove) );
    transslider->attach( alignedBelow, shineslider );

    setCancelText( "" );

    finaliseStart.notify( mCB(this,uiMaterialDlg,doFinalise) );
}


void uiMaterialDlg::doFinalise( CallBacker* )
{
    ambslider->sldr()->setMinValue( cMinVal );
    ambslider->sldr()->setMaxValue( cMaxVal );
    ambslider->sldr()->setStep( cStepVal );
    ambslider->sldr()->setValue( material->getAmbience()*100 );

    diffslider->sldr()->setMinValue( cMinVal );
    diffslider->sldr()->setMaxValue( cMaxVal );
    diffslider->sldr()->setStep( cStepVal );
    diffslider->sldr()->setValue( material->getDiffIntensity()*100 );

    specslider->sldr()->setMinValue( cMinVal );
    specslider->sldr()->setMaxValue( cMaxVal );
    specslider->sldr()->setStep( cStepVal );
    specslider->sldr()->setValue( material->getSpecIntensity()*100 );

    emisslider->sldr()->setMinValue( cMinVal );
    emisslider->sldr()->setMaxValue( cMaxVal );
    emisslider->sldr()->setStep( cStepVal );
    emisslider->sldr()->setValue( material->getEmmIntensity()*100 );

    shineslider->sldr()->setMinValue( cMinVal );
    shineslider->sldr()->setMaxValue( cMaxVal );
    shineslider->sldr()->setStep( cStepVal );
    shineslider->sldr()->setValue( material->getShininess()*100 );

    transslider->sldr()->setMinValue( cMinVal );
    transslider->sldr()->setMaxValue( cMaxVal );
    transslider->sldr()->setStep( cStepVal );
    transslider->sldr()->setValue( material->getTransparency()*100 );
}


bool uiMaterialDlg::acceptOK( CallBacker* )
{
    material->setAmbience( ambslider->sldr()->getValue()/100 );
    material->setDiffIntensity( diffslider->sldr()->getValue()/100 );
    material->setSpecIntensity( specslider->sldr()->getValue()/100 );
    material->setEmmIntensity( emisslider->sldr()->getValue()/100 );
    material->setShininess( shineslider->sldr()->getValue()/100 );
    material->setTransparency( transslider->sldr()->getValue()/100 );
    
    return true;
}


void uiMaterialDlg::ambSliderMove( CallBacker* )
{
    material->setAmbience( ambslider->sldr()->getValue()/100 );
}

void uiMaterialDlg::diffSliderMove( CallBacker* )
{
    material->setDiffIntensity( diffslider->sldr()->getValue()/100 );
}

void uiMaterialDlg::specSliderMove( CallBacker* )
{
    material->setSpecIntensity( specslider->sldr()->getValue()/100 );
}

void uiMaterialDlg::emisSliderMove( CallBacker* )
{
    material->setEmmIntensity( emisslider->sldr()->getValue()/100 );
}

void uiMaterialDlg::shineSliderMove( CallBacker* )
{
    material->setShininess( shineslider->sldr()->getValue()/100 );
}

void uiMaterialDlg::transSliderMove( CallBacker* )
{
    material->setTransparency( transslider->sldr()->getValue()/100 );
}

