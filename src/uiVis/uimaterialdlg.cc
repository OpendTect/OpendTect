/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uimaterialdlg.cc,v 1.9 2005-09-06 08:41:44 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uimaterialdlg.h"

#include "uicolor.h"
#include "uisellinest.h"
#include "uislider.h"
#include "uitabstack.h"
#include "vismaterial.h"
#include "visobject.h"
#include "vissurvobj.h"

const int cMinVal = 0;
const int cMaxVal = 100;
const int cStepVal = 10;


uiLineStyleGrp::uiLineStyleGrp( uiParent* p, visSurvey::SurveyObject* so )
    : uiPropertyGrp( p, "Line style" )
    , survobj( so )
    , backup( *so->lineStyle() )
{
    field = new uiSelLineStyle( this, backup, "Line style", true,
	    			so->hasSpecificLineColor(), true );
    field->changed.notify( mCB(this,uiLineStyleGrp,changedCB) );
}


void uiLineStyleGrp::changedCB(CallBacker*)
{
    survobj->setLineStyle(field->getStyle());
}


bool uiLineStyleGrp::rejectOK(CallBacker*)
{
    survobj->setLineStyle(backup);
    return true;
}


uiPropertiesDlg::uiPropertiesDlg( uiParent* p, visSurvey::SurveyObject* so )
    : uiDialog(p,"Display properties")
    , survobj( so )
    , visobj( dynamic_cast<visBase::VisualObject*>(so) )
    , tabstack( new uiTabStack(this,"TabStack") )
{
    if ( survobj->allowMaterialEdit() && visobj->getMaterial() )
    {
	uiPropertyGrp* grp = new uiMaterialGrp(tabstack->tabGroup(),
				survobj,
				true, true, false, false, false, true,
				survobj->hasColor() );
	tabstack->addTab(grp);
	tabs += grp;
    }

    if ( survobj->lineStyle() )
    {
	uiPropertyGrp* grp = new uiLineStyleGrp(tabstack->tabGroup(), survobj );
	tabstack->addTab(grp);
	tabs += grp;
    }

    setCancelText( "" );

    finaliseStart.notify( mCB(this,uiPropertiesDlg,doFinalise) );
}


void uiPropertiesDlg::doFinalise(CallBacker* cb)
{ for ( int idx=0; idx<tabs.size(); idx++ ) tabs[idx]->doFinalise(cb); }


bool uiPropertiesDlg::acceptOK(CallBacker* cb)
{
    for ( int idx=0; idx<tabs.size(); idx++ )
    {
	if ( !tabs[idx]->acceptOK(cb) )
	    return false;
    }

    return true;
}


bool uiPropertiesDlg::rejectOK(CallBacker* cb)
{
    for ( int idx=0; idx<tabs.size(); idx++ )
    {
	if ( !tabs[idx]->rejectOK(cb) )
	    return false;
    }

    return true;
}


uiMaterialGrp::uiMaterialGrp( uiParent* p, visSurvey::SurveyObject* so,
       bool ambience, bool diffusecolor, bool specularcolor,
       bool emmissivecolor, bool shininess, bool transparency, bool color )
    : uiPropertyGrp(p,"Material")
    , material( dynamic_cast<visBase::VisualObject*>(so)->getMaterial() )
    , survobj( so )
    , ambslider( 0 )
    , diffslider( 0 )
    , specslider( 0 )
    , emisslider( 0 )
    , shineslider( 0 )
    , transslider( 0 )
    , colinp( 0 )
{
    uiGroup* prevslider = 0;

    if ( so->hasColor() )
    {
	colinp = new uiColorInput(this,Color(0,0,0),"Base color");
	colinp->colorchanged.notify( mCB(this,uiMaterialGrp,colorChangeCB) );
	colinp->setSensitive( color );
	prevslider = colinp;
    }

    if ( ambience )
    {
	uiSliderExtra* se = new uiSliderExtra( this, "Ambient reflectivity" );
	ambslider = se->sldr();
	ambslider->valueChanged.notify( 
				mCB(this,uiMaterialGrp,ambSliderMove) );
	if ( prevslider ) se->attach( alignedBelow, prevslider );
	prevslider = se;
    }

    if ( diffusecolor )
    {
	uiSliderExtra* se = new uiSliderExtra( this, "Diffuse reflectivity" );
	diffslider = se->sldr();
	diffslider->valueChanged.notify( 
				mCB(this,uiMaterialGrp,diffSliderMove) );
	if ( prevslider ) se->attach( alignedBelow, prevslider );
	prevslider = se;
    }

    if ( specularcolor )
    {
	uiSliderExtra* se = new uiSliderExtra( this, "Specular reflectivity" );
	specslider = se->sldr();
	specslider->valueChanged.notify( 
				mCB(this,uiMaterialGrp,specSliderMove) );
	if ( prevslider ) se->attach( alignedBelow, prevslider );
	prevslider = se;
    }

    if ( emmissivecolor )
    {
	uiSliderExtra* se = new uiSliderExtra( this, "Emissive intensity" );
	emisslider = se->sldr();
	emisslider->valueChanged.notify(
				mCB(this,uiMaterialGrp,emisSliderMove) );
	if ( prevslider ) se->attach( alignedBelow, prevslider );
	prevslider = se;
    }

    if ( shininess )
    {
	uiSliderExtra* se = new uiSliderExtra( this, "Shininess" );
	shineslider = se->sldr();
	shineslider->valueChanged.notify( 
				mCB(this,uiMaterialGrp,shineSliderMove) );
	if ( prevslider ) se->attach( alignedBelow, prevslider );
	prevslider = se;
    }

    if ( transparency )
    {
	uiSliderExtra* se = new uiSliderExtra( this, "Transparency" );
	transslider = se->sldr();
	transslider->valueChanged.notify(
				mCB(this,uiMaterialGrp,transSliderMove) );
	if ( prevslider ) se->attach( alignedBelow, prevslider );
	prevslider = se;
    }

}


void uiMaterialGrp::doFinalise( CallBacker* )
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

    if ( colinp ) colinp->setColor( material->getColor() );
}


void uiMaterialGrp::ambSliderMove( CallBacker* )
{
    if ( ambslider )
	material->setAmbience( ambslider->getValue()/100 );
}

void uiMaterialGrp::diffSliderMove( CallBacker* )
{
    if ( diffslider )
	material->setDiffIntensity( diffslider->getValue()/100 );
}

void uiMaterialGrp::specSliderMove( CallBacker* )
{
    if ( specslider )
	material->setSpecIntensity( specslider->getValue()/100 );
}

void uiMaterialGrp::emisSliderMove( CallBacker* )
{
    if ( specslider )
	material->setEmmIntensity( emisslider->getValue()/100 );
}

void uiMaterialGrp::shineSliderMove( CallBacker* )
{
    if ( shineslider )
	material->setShininess( shineslider->getValue()/100 );
}

void uiMaterialGrp::transSliderMove( CallBacker* )
{
    if ( transslider )
	material->setTransparency( transslider->getValue()/100 );
}


void uiMaterialGrp::colorChangeCB(CallBacker*)
{
    if ( colinp ) survobj->setColor( colinp->color() );
}

