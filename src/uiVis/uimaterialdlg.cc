/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uimaterialdlg.cc,v 1.4 2003-11-07 12:22:02 bert Exp $
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
	ambslider = new uiLabeledSlider( this, "Ambience" );
	ambslider->sldr()->valueChanged.notify( 
	    mCB(this,uiMaterialDlg,ambSliderMove) );

	prevslider = ambslider;
    }

    if ( diffusecolor )
    {
	diffslider = new uiLabeledSlider( this, "Diffuse color" );
	diffslider->sldr()->valueChanged.notify( 
	    mCB(this,uiMaterialDlg,diffSliderMove) );
	if ( prevslider ) diffslider->attach( alignedBelow, prevslider );
	prevslider = diffslider;
    }

    if ( specularcolor )
    {
	specslider = new uiLabeledSlider( this, "Specular color" );
	specslider->sldr()->valueChanged.notify( 
	    mCB(this,uiMaterialDlg,specSliderMove) );
	if ( prevslider ) specslider->attach( alignedBelow, prevslider );
	prevslider = specslider;

    }

    if ( emmissivecolor )
    {
	emisslider = new uiLabeledSlider( this, "Emissive color" );
	emisslider->sldr()->valueChanged.notify( 
	    mCB(this,uiMaterialDlg,emisSliderMove) );
	if ( prevslider ) emisslider->attach( alignedBelow, prevslider );
	prevslider = emisslider;
    }

    if ( shininess )
    {
	shineslider = new uiLabeledSlider( this, "Shininess" );
	shineslider->sldr()->valueChanged.notify( 
	    mCB(this,uiMaterialDlg,shineSliderMove) );
	if (  prevslider ) shineslider->attach( alignedBelow, prevslider );
	prevslider = shineslider;
    }

    if ( transparency )
    {
	transslider = new uiLabeledSlider( this, "Transparency" );
	transslider->sldr()->valueChanged.notify( 
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
	ambslider->sldr()->setMinValue( cMinVal );
	ambslider->sldr()->setMaxValue( cMaxVal );
	ambslider->sldr()->setTickStep( cStepVal );
	ambslider->sldr()->setValue( material->getAmbience()*100 );
    }

    if ( diffslider )
    {
	diffslider->sldr()->setMinValue( cMinVal );
	diffslider->sldr()->setMaxValue( cMaxVal );
	diffslider->sldr()->setTickStep( cStepVal );
	diffslider->sldr()->setValue( material->getDiffIntensity()*100 );
    }

    if ( specslider )
    {
	specslider->sldr()->setMinValue( cMinVal );
	specslider->sldr()->setMaxValue( cMaxVal );
	specslider->sldr()->setTickStep( cStepVal );
	specslider->sldr()->setValue( material->getSpecIntensity()*100 );
    }

    if ( emisslider )
    {
	emisslider->sldr()->setMinValue( cMinVal );
	emisslider->sldr()->setMaxValue( cMaxVal );
	emisslider->sldr()->setTickStep( cStepVal );
	emisslider->sldr()->setValue( material->getEmmIntensity()*100 );
    }

    if ( shineslider )
    {
	shineslider->sldr()->setMinValue( cMinVal );
	shineslider->sldr()->setMaxValue( cMaxVal );
	shineslider->sldr()->setTickStep( cStepVal );
	shineslider->sldr()->setValue( material->getShininess()*100 );
    }

    if ( transslider )
    {
	transslider->sldr()->setMinValue( cMinVal );
	transslider->sldr()->setMaxValue( cMaxVal );
	transslider->sldr()->setTickStep( cStepVal );
	transslider->sldr()->setValue( material->getTransparency()*100 );
    }
}


bool uiMaterialDlg::acceptOK( CallBacker* )
{
    if ( ambslider )
	material->setAmbience( ambslider->sldr()->getValue()/100 );
    if ( diffslider )
	material->setDiffIntensity( diffslider->sldr()->getValue()/100 );
    if ( specslider )
	material->setSpecIntensity( specslider->sldr()->getValue()/100 );
    if ( emisslider )
	material->setEmmIntensity( emisslider->sldr()->getValue()/100 );
    if ( shineslider )
	material->setShininess( shineslider->sldr()->getValue()/100 );
    if ( transslider )
	material->setTransparency( transslider->sldr()->getValue()/100 );
    
    return true;
}


void uiMaterialDlg::ambSliderMove( CallBacker* )
{
    if ( ambslider )
	material->setAmbience( ambslider->sldr()->getValue()/100 );
}

void uiMaterialDlg::diffSliderMove( CallBacker* )
{
    if ( diffslider )
	material->setDiffIntensity( diffslider->sldr()->getValue()/100 );
}

void uiMaterialDlg::specSliderMove( CallBacker* )
{
    if ( specslider )
	material->setSpecIntensity( specslider->sldr()->getValue()/100 );
}

void uiMaterialDlg::emisSliderMove( CallBacker* )
{
    if ( specslider )
	material->setEmmIntensity( emisslider->sldr()->getValue()/100 );
}

void uiMaterialDlg::shineSliderMove( CallBacker* )
{
    if ( shineslider )
	material->setShininess( shineslider->sldr()->getValue()/100 );
}

void uiMaterialDlg::transSliderMove( CallBacker* )
{
    if ( transslider )
	material->setTransparency( transslider->sldr()->getValue()/100 );
}

