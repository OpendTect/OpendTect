/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uimaterialdlg.cc,v 1.6 2004-03-03 12:43:14 nanne Exp $
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
    uiGroup* prevslider = 0;

    if ( ambience )
    {
	uiSliderExtra* se = new uiSliderExtra( this, "Ambience" );
	ambslider = se->sldr();
	ambslider->valueChanged.notify( 
				mCB(this,uiMaterialDlg,ambSliderMove) );
	prevslider = se;
    }

    if ( diffusecolor )
    {
	uiSliderExtra* se = new uiSliderExtra( this, "Diffuse color" );
	diffslider = se->sldr();
	diffslider->valueChanged.notify( 
				mCB(this,uiMaterialDlg,diffSliderMove) );
	if ( prevslider ) se->attach( alignedBelow, prevslider );
	prevslider = se;
    }

    if ( specularcolor )
    {
	uiSliderExtra* se = new uiSliderExtra( this, "Specular color" );
	specslider = se->sldr();
	specslider->valueChanged.notify( 
				mCB(this,uiMaterialDlg,specSliderMove) );
	if ( prevslider ) se->attach( alignedBelow, prevslider );
	prevslider = se;
    }

    if ( emmissivecolor )
    {
	uiSliderExtra* se = new uiSliderExtra( this, "Emissive color" );
	emisslider = se->sldr();
	emisslider->valueChanged.notify(
				mCB(this,uiMaterialDlg,emisSliderMove) );
	if ( prevslider ) se->attach( alignedBelow, prevslider );
	prevslider = se;
    }

    if ( shininess )
    {
	uiSliderExtra* se = new uiSliderExtra( this, "Shininess" );
	shineslider = se->sldr();
	shineslider->valueChanged.notify( 
				mCB(this,uiMaterialDlg,shineSliderMove) );
	if ( prevslider ) se->attach( alignedBelow, prevslider );
	prevslider = se;
    }

    if ( transparency )
    {
	uiSliderExtra* se = new uiSliderExtra( this, "Transparency" );
	transslider = se->sldr();
	transslider->valueChanged.notify(
				mCB(this,uiMaterialDlg,transSliderMove) );
	if ( prevslider ) se->attach( alignedBelow, prevslider );
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

