/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
________________________________________________________________________

-*/

#include "uimaterialdlg.h"

#include "uicolor.h"
#include "uigeninput.h"
#include "uisellinest.h"
#include "uislider.h"
#include "uitabstack.h"
#include "uivisplanedatadisplaydragprop.h"
#include "uivispolygonsurfbezierdlg.h"
#include "uifltdispoptgrp.h"
#include "uimarkerstyle.h"
#include "vismaterial.h"
#include "visobject.h"
#include "vissurvobj.h"
#include "visfaultdisplay.h"
#include "vishorizondisplay.h"
#include "visplanedatadisplay.h"
#include "vispolygonbodydisplay.h"
#include "visemobjdisplay.h"
#include "envvars.h"
#include "od_helpids.h"


// uiPropertiesDlg
uiPropertiesDlg::uiPropertiesDlg( uiParent* p, visSurvey::SurveyObject* so )
    : uiTabStackDlg(p,uiDialog::Setup(uiStrings::sDisplayProperties(),
				      mNoDlgTitle,
				      mODHelpKey(mPropertiesDlgHelpID) ))
    , survobj_(so)
    , visobj_(dynamic_cast<visBase::VisualObject*>(so))
{
    mDynamicCastGet(visSurvey::FaultDisplay*,fd,so);
    if ( fd )
	addGroup( new uiFaultDisplayOptGrp(tabstack_->tabGroup(),fd) );

    if ( survobj_->allowMaterialEdit() && visobj_->getMaterial() )
    {
	addGroup(  new uiMaterialGrp( tabstack_->tabGroup(),
			survobj_, true, true, false, false, false, true,
			survobj_->usesColor() )) ;
    }

    if ( so && so->canEnableTextureInterpolation() )
	addGroup( new uiTextureInterpolateGrp(tabstack_->tabGroup(),survobj_) );

    if ( survobj_->lineStyle() )
	addGroup( new uiLineStyleGrp( tabstack_->tabGroup(), survobj_ )  );

    if ( survobj_->markerStyle() )
	addGroup( new uiMarkerStyleGrp( tabstack_->tabGroup(), survobj_ ) );

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,so);
    if ( pdd )
	addGroup( new uiVisPlaneDataDisplayDragProp(tabstack_->tabGroup(),pdd));

    mDynamicCastGet(visSurvey::PolygonBodyDisplay*,plg,so);
    if ( plg )
	addGroup( new uiVisPolygonSurfBezierDlg(tabstack_->tabGroup(),plg) );

    setCancelText( uiString::empty() );
}


//uiMarkerStyleGrp
uiMarkerStyleGrp::uiMarkerStyleGrp( uiParent* p, visSurvey::SurveyObject* so )
    : uiDlgGroup(p,tr("Node style"))
    , survobj_( so )

{
    TypeSet<OD::MarkerStyle3D::Type> excludedtypes;
    excludedtypes.add( OD::MarkerStyle3D::None );

    const OD::MarkerStyle3D* mkstyle = so->markerStyle();
    const bool enabcolsel = so->markerStyleColorSelection();

    stylefld_ = new uiMarkerStyle3D( this,
	uiMarkerStyle::Setup().wcolor(enabcolsel), &excludedtypes );
    if ( mkstyle )
	stylefld_->setMarkerStyle( *mkstyle );

    stylefld_->change.notify( mCB(this,uiMarkerStyleGrp,changeCB) );
}


void uiMarkerStyleGrp::changeCB( CallBacker* )
{
    OD::MarkerStyle3D mkstyle;
    stylefld_->getMarkerStyle( mkstyle );
    if ( survobj_ )
	survobj_->setMarkerStyle( mkstyle );
}


// uiLineStyleGrp
uiLineStyleGrp::uiLineStyleGrp( uiParent* p, visSurvey::SurveyObject* so )
    : uiDlgGroup(p,tr("Line style"))
    , survobj_(so)
    , backup_(*so->lineStyle())
{
    uiSelLineStyle::Setup lssu( tr("Line style") );
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


bool uiLineStyleGrp::rejectOK()
{
    survobj_->setLineStyle( backup_ );
    return true;
}


// uiTextureInterpolateGrp
uiTextureInterpolateGrp::uiTextureInterpolateGrp( uiParent* p,
						  visSurvey::SurveyObject* so )
    : uiDlgGroup(p,uiStrings::sTexture())
    , survobj_(so)
    , textclassify_(0)
{
    if ( !so || !so->canEnableTextureInterpolation() )
	return;

    const bool intpenabled = so->textureInterpolationEnabled();

    textclassify_ = new uiGenInput( this, uiStrings::sData(),
	    BoolInpSpec(intpenabled,uiStrings::sInterpolation(),
		uiStrings::sClassification()) );
    textclassify_->valuechanged.notify(
	    mCB(this,uiTextureInterpolateGrp,chgIntpCB) );
}


void uiTextureInterpolateGrp::chgIntpCB( CallBacker* cb )
{
    if ( survobj_ && textclassify_ )
	survobj_->enableTextureInterpolation( textclassify_->getBoolValue() );
}


// uiMaterialGrp

#define mFinalise( sldr, fn ) \
if ( sldr ) \
{ \
    sldr->setInterval( StepInterval<float>( 0, 100, 10 ) ); \
    sldr->setValue( material_->fn()*100 ); \
}

uiMaterialGrp::uiMaterialGrp( uiParent* p, visSurvey::SurveyObject* so,
       bool ambience, bool diffusecolor, bool specularcolor,
       bool emmissivecolor, bool shininess, bool transparency, bool color )
    : uiDlgGroup(p,tr("Material"))
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
		uiColorInput::Setup(so->getColor()).lbltxt(tr("Base color")) );
	colinp_->colorChanged.notify( mCB(this,uiMaterialGrp,colorChangeCB) );
	colinp_->setSensitive( color );
	prevobj_ = colinp_;
    }

    createSlider( ambience, ambslider_, tr("Ambient reflectivity") );
    createSlider( diffusecolor, diffslider_, tr("Diffuse reflectivity") );
    createSlider( specularcolor, specslider_, tr("Specular reflectivity") );
    createSlider( emmissivecolor, emisslider_, tr("Emissive intensity") );
    createSlider( shininess, shineslider_, tr("Shininess") );
    createSlider( transparency, transslider_, uiStrings::sTransparency() );

    mFinalise( ambslider_, getAmbience );
    mFinalise( diffslider_, getDiffIntensity );
    mFinalise( specslider_, getSpecIntensity );
    mFinalise( emisslider_, getEmmIntensity );
    mFinalise( shineslider_, getShininess );
    mFinalise( transslider_, getTransparency );
}


void uiMaterialGrp::createSlider( bool domk, uiSlider*& slider,
				  const uiString& lbltxt )
{
    if ( !domk ) return;

    uiSlider::Setup ss( lbltxt ); ss.withedit(true);
    slider = new uiSlider( this, ss, toString(lbltxt) );
    slider->valueChanged.notify( mCB(this,uiMaterialGrp,sliderMove) );
    if ( prevobj_ )
	slider->attach( alignedBelow, prevobj_ );
    prevobj_ = slider;
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
