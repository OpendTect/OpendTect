/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uimaterialdlg.cc,v 1.5 2004-03-02 13:29:56 nanne Exp $
________________________________________________________________________

-*/

#include "uimaterialdlg.h"
#include "vismaterial.h"
#include "uislider.h"

const int cMinVal = 0;
const int cMaxVal = 100;
const int cStepVal = 10;


uiMaterialDlg::uiMaterialDlg( uiParent* p, visBase::Material* mat,
       bool ambience, bool diffusecolor, bool specularcolor,
       bool emmissivecolor, bool shininess, bool transparency )
    : uiDialog(p,"Material")
    , material(mat)
    , ambslider( 0 )
    , diffslider( 0 )
    , specslider( 0 )
    , emisslider( 0 )
    , shineslider( 0 )
    , transslider( 0 )
{
    uiObject* prevslider = 0;

    if ( ambience )
    {
	uiSliderExtra* slider = new uiSliderExtra( this, "Ambience" );
	ambslider = slider->sldr();
	ambslider->valueChanged.notify( mCB(this,uiMaterialDlg,ambSliderMove) );
	prevslider = ambslider;
    }

    if ( diffusecolor )
    {
	uiSliderExtra* slider = new uiSliderExtra( this, "Diffuse color" );
	diffslider = slider->sldr();
	diffslider->valueChanged.notify( 
				mCB(this,uiMaterialDlg,diffSliderMove) );
	if ( prevslider ) diffslider->attach( alignedBelow, prevslider );
	prevslider = diffslider;
    }

    if ( specularcolor )
    {
	uiSliderExtra* slider = new uiSliderExtra( this, "Specular color" );
	specslider = slider->sldr();
	specslider->valueChanged.notify(
				mCB(this,uiMaterialDlg,specSliderMove) );
	if ( prevslider ) specslider->attach( alignedBelow, prevslider );
	prevslider = specslider;

    }

    if ( emmissivecolor )
    {
	uiSliderExtra* slider = new uiSliderExtra( this, "Emissive color" );
	emisslider = slider->sldr();
	emisslider->valueChanged.notify(
				mCB(this,uiMaterialDlg,emisSliderMove) );
	if ( prevslider ) emisslider->attach( alignedBelow, prevslider );
	prevslider = emisslider;
    }

    if ( shininess )
    {
	uiSliderExtra* slider = new uiSliderExtra( this, "Shininess" );
	shineslider = slider->sldr();
	shineslider->valueChanged.notify( 
				mCB(this,uiMaterialDlg,shineSliderMove) );
	if (  prevslider ) shineslider->attach( alignedBelow, prevslider );
	prevslider = shineslider;
    }

    if ( transparency )
    {
	uiSliderExtra* slider = new uiSliderExtra( this, "Transparency" );
	transslider = slider->sldr();
	transslider->valueChanged.notify( 
				mCB(this,uiMaterialDlg,transSliderMove) );
	if (  prevslider ) transslider->attach( alignedBelow, prevslider );
	prevslider = transslider;
    }

    setCancelText( "" );

    finaliseStart.notify( mCB(this,uiMaterialDlg,doFinalise) );
}


void uiMaterialDlg::doFinalise( CallBacker* )
{
    if ( ambslider )
    {
	ambslider->setMinValue( cMinVal );
	ambslider->setMaxValue( cMaxVal );
	ambslider->setTickStep( cStepVal );
	ambslider->setValue( material->getAmbience()*100 );
    }

    if ( diffslider )
    {
	diffslider->setMinValue( cMinVal );
	diffslider->setMaxValue( cMaxVal );
	diffslider->setTickStep( cStepVal );
	diffslider->setValue( material->getDiffIntensity()*100 );
    }

    if ( specslider )
    {
	specslider->setMinValue( cMinVal );
	specslider->setMaxValue( cMaxVal );
	specslider->setTickStep( cStepVal );
	specslider->setValue( material->getSpecIntensity()*100 );
    }

    if ( emisslider )
    {
	emisslider->setMinValue( cMinVal );
	emisslider->setMaxValue( cMaxVal );
	emisslider->setTickStep( cStepVal );
	emisslider->setValue( material->getEmmIntensity()*100 );
    }

    if ( shineslider )
    {
	shineslider->setMinValue( cMinVal );
	shineslider->setMaxValue( cMaxVal );
	shineslider->setTickStep( cStepVal );
	shineslider->setValue( material->getShininess()*100 );
    }

    if ( transslider )
    {
	transslider->setMinValue( cMinVal );
	transslider->setMaxValue( cMaxVal );
	transslider->setTickStep( cStepVal );
	transslider->setValue( material->getTransparency()*100 );
    }
}


bool uiMaterialDlg::acceptOK( CallBacker* )
{
    if ( ambslider )
	material->setAmbience( ambslider->getValue()/100 );
    if ( diffslider )
	material->setDiffIntensity( diffslider->getValue()/100 );
    if ( specslider )
	material->setSpecIntensity( specslider->getValue()/100 );
    if ( emisslider )
	material->setEmmIntensity( emisslider->getValue()/100 );
    if ( shineslider )
	material->setShininess( shineslider->getValue()/100 );
    if ( transslider )
	material->setTransparency( transslider->getValue()/100 );
    
    return true;
}


void uiMaterialDlg::ambSliderMove( CallBacker* )
{
    if ( ambslider )
	material->setAmbience( ambslider->getValue()/100 );
}

void uiMaterialDlg::diffSliderMove( CallBacker* )
{
    if ( diffslider )
	material->setDiffIntensity( diffslider->getValue()/100 );
}

void uiMaterialDlg::specSliderMove( CallBacker* )
{
    if ( specslider )
	material->setSpecIntensity( specslider->getValue()/100 );
}

void uiMaterialDlg::emisSliderMove( CallBacker* )
{
    if ( specslider )
	material->setEmmIntensity( emisslider->getValue()/100 );
}

void uiMaterialDlg::shineSliderMove( CallBacker* )
{
    if ( shineslider )
	material->setShininess( shineslider->getValue()/100 );
}

void uiMaterialDlg::transSliderMove( CallBacker* )
{
    if ( transslider )
	material->setTransparency( transslider->getValue()/100 );
}

