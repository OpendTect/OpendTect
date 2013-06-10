/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimaterialdlg.h"

#include "uicolor.h"
#include "uigeninput.h"
#include "uisellinest.h"
#include "uislider.h"
#include "uitabstack.h"
#include "uivisplanedatadisplaydragprop.h"
#include "uivispolygonsurfbezierdlg.h"
#include "uifltdispoptgrp.h"
#include "vismaterial.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "visfaultdisplay.h"
#include "vishorizondisplay.h"
#include "visplanedatadisplay.h"
#include "vispolygonbodydisplay.h"
#include "visemobjdisplay.h"
#include "vismarchingcubessurfacedisplay.h"



uiLineStyleGrp::uiLineStyleGrp( uiParent* p, visSurvey::SurveyObject* so )
    : uiDlgGroup(p,"Line style")
    , survobj_(so)
    , backup_(*so->lineStyle())
{
    uiSelLineStyle::Setup lssu( "Line style" );
    lssu.drawstyle(false).color(so->hasSpecificLineColor());
    field_ = new uiSelLineStyle( this, backup_, lssu );
      // TODO: include drawstyle after properly implementing all line styles.

    field_->changed.notify( mCB(this,uiLineStyleGrp,changedCB) );

    // set maximum limit for line width
    int min, max;
    survobj_->getLineWidthBounds( min, max );
    if ( !mIsUdf(min) && !mIsUdf(max) )
	field_->setLineWidthBounds( min, max );
}


void uiLineStyleGrp::changedCB( CallBacker* )
{
    survobj_->setLineStyle( field_->getStyle() );
}


bool uiLineStyleGrp::rejectOK( CallBacker* )
{
    survobj_->setLineStyle( backup_ );
    return true;
}


uiPropertiesDlg::uiPropertiesDlg( uiParent* p, visSurvey::SurveyObject* so )
    : uiTabStackDlg(p,uiDialog::Setup("Display properties",0,"50.0.4"))
    , survobj_(so)
    , visobj_(dynamic_cast<visBase::VisualObject*>(so))
{
    if ( survobj_->allowMaterialEdit() && visobj_->getMaterial() )
    {
	addGroup(  new uiMaterialGrp( tabstack_->tabGroup(),
			survobj_, true, true, false, false, false, true,
			survobj_->hasColor() )) ;
    }
    
    if ( so && so->canEnableTextureInterpolation() )
	addGroup( new uiTextureInterpolateGrp(tabstack_->tabGroup(),survobj_) );

    if ( survobj_->lineStyle() )
	addGroup( new uiLineStyleGrp( tabstack_->tabGroup(), survobj_ )  );

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,so);
    if ( pdd )
	addGroup( new uiVisPlaneDataDisplayDragProp(tabstack_->tabGroup(),pdd));
    
    mDynamicCastGet(visSurvey::PolygonBodyDisplay*,plg,so);
    if ( plg )
	addGroup( new uiVisPolygonSurfBezierDlg(tabstack_->tabGroup(),plg) );

    mDynamicCastGet(visSurvey::FaultDisplay*,flt,so);
    if ( flt )
	addGroup( new uiFaultDisplayOptGrp(tabstack_->tabGroup(),flt) );

    setCancelText( "" );
}


uiTextureInterpolateGrp::uiTextureInterpolateGrp( uiParent* p, 
						  visSurvey::SurveyObject* so )
    : uiDlgGroup(p,"Texture")
    , survobj_(so)
{
    bool intpenabled = false; 
    mDynamicCastGet( visSurvey::MultiTextureSurveyObject*, mto, survobj_ );
    if ( mto )
	intpenabled = mto->textureInterpolationEnabled();
    else
    {
	mDynamicCastGet( visSurvey::HorizonDisplay*, hor, survobj_ );
	if ( hor ) 
	    intpenabled = hor->textureInterpolationEnabled();
	else
	    return;
    }
    
    textclasssify_ = new uiGenInput( this, "Data:   ", 
	    BoolInpSpec(intpenabled,"Interpolation","Classification") );
    textclasssify_->valuechanged.notify( 
	    mCB(this,uiTextureInterpolateGrp,chgIntpCB) );
}


void uiTextureInterpolateGrp::chgIntpCB( CallBacker* cb )
{
    if ( !textclasssify_ )
	return;
    
    const bool intp = textclasssify_->getBoolValue();
    mDynamicCastGet( visSurvey::MultiTextureSurveyObject*, mto, survobj_ );
    if ( mto  ) 
     	mto->enableTextureInterpolation( intp );    
    else
    {
	mDynamicCastGet( visSurvey::HorizonDisplay*, hor, survobj_ );
	if ( hor ) 
	    hor->enableTextureInterpolation( intp );
    }
}


#define mFinalise( sldr, fn ) \
if ( sldr ) \
{ \
    sldr->setInterval( StepInterval<float>( 0, 100, 10 ) ); \
    sldr->setValue( material_->fn()*100 ); \
}

uiMaterialGrp::uiMaterialGrp( uiParent* p, visSurvey::SurveyObject* so,
       bool ambience, bool diffusecolor, bool specularcolor,
       bool emmissivecolor, bool shininess, bool transparency, bool color )
    : uiDlgGroup(p,"Material")
    , material_(dynamic_cast<visBase::VisualObject*>(so)->getMaterial())
    , survobj_(so)
    , ambslider_(0)
    , diffslider_(0)
    , specslider_(0)
    , emisslider_(0)
    , shineslider_(0)
    , transslider_(0)
    , colinp_(0)
    , prevobj_(0)
{
    if ( so->hasColor() )
    {
	colinp_ = new uiColorInput( this,
		uiColorInput::Setup(so->getColor()).lbltxt("Base color") );
	colinp_->colorChanged.notify( mCB(this,uiMaterialGrp,colorChangeCB) );
	colinp_->setSensitive( color );
	prevobj_ = colinp_;
    }

    mDynamicCastGet( visSurvey::MarchingCubesDisplay*,mcube,so );
    if ( mcube )
	colinp_->setSensitive( !mcube->usesTexture() );

    createSlider( ambience, ambslider_, "Ambient reflectivity" );
    createSlider( diffusecolor, diffslider_, "Diffuse reflectivity" );
    createSlider( specularcolor, specslider_, "Specular reflectivity" );
    createSlider( emmissivecolor, emisslider_, "Emissive intensity" );
    createSlider( shininess, shineslider_, "Shininess" );
    createSlider( transparency, transslider_, "Transparency" );

    mFinalise( ambslider_, getAmbience );
    mFinalise( diffslider_, getDiffIntensity );
    mFinalise( specslider_, getSpecIntensity );
    mFinalise( emisslider_, getEmmIntensity );
    mFinalise( shineslider_, getShininess );
    mFinalise( transslider_, getTransparency );
}


void uiMaterialGrp::createSlider( bool domk, uiSlider*& slider,
				  const char* lbltxt )
{
    if ( !domk ) return;

    uiSliderExtra::Setup ss( lbltxt ); ss.withedit(true);
    uiSliderExtra* se = new uiSliderExtra( this, ss,
	    				   BufferString(lbltxt," slider") );
    slider = se->sldr();
    slider->valueChanged.notify( mCB(this,uiMaterialGrp,sliderMove) );
    if ( prevobj_ ) se->attach( alignedBelow, prevobj_ );
    prevobj_ = se;
}



void uiMaterialGrp::sliderMove( CallBacker* cb )
{
    mDynamicCastGet(uiSlider*,sldr,cb)
    if ( !sldr ) return;

    if ( sldr == ambslider_ )
	material_->setAmbience( ambslider_->getValue()/100 );
    else if ( sldr == diffslider_ )
	material_->setDiffIntensity( diffslider_->getValue()/100 );
    else if ( sldr == specslider_ )
	material_->setSpecIntensity( specslider_->getValue()/100 );
    else if ( sldr == emisslider_ )
	material_->setEmmIntensity( emisslider_->getValue()/100 );
    else if ( sldr == shineslider_ )
	material_->setShininess( shineslider_->getValue()/100 );
    else if ( sldr == transslider_ )
	material_->setTransparency( transslider_->getValue()/100 );
}

void uiMaterialGrp::colorChangeCB(CallBacker*)
{ if ( colinp_ ) survobj_->setColor( colinp_->color() ); }
