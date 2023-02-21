/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimaterialdlg.h"

#include "uicolor.h"
#include "uifltdispoptgrp.h"
#include "uigeninput.h"
#include "uimarkerstyle.h"
#include "uisellinest.h"
#include "uislider.h"
#include "uitabstack.h"
#include "uivisplanedatadisplaydragprop.h"
#include "uivispolygonsurfbezierdlg.h"

#include "visfaultdisplay.h"
#include "vismaterial.h"
#include "visobject.h"
#include "visplanedatadisplay.h"
#include "vispolygonbodydisplay.h"
#include "vissurvobj.h"

#include "envvars.h"
#include "od_helpids.h"


// uiPropertiesDlg
CNotifier<uiPropertiesDlg,VisID>& uiPropertiesDlg::closeNotifier()
{
    static CNotifier<uiPropertiesDlg,VisID> notif( nullptr );
    return notif;
}


uiPropertiesDlg::uiPropertiesDlg( uiParent* p, visSurvey::SurveyObject* so )
    : uiTabStackDlg(p,uiDialog::Setup(tr("Display properties"),
				mNoDlgTitle,mODHelpKey(mPropertiesDlgHelpID)))
    , survobj_(so)
{
    if ( !survobj_ )
	return;

    mDynamicCastGet(visBase::VisualObject*,vo,survobj_)
    if ( survobj_->allowMaterialEdit() && vo && vo->getMaterial() )
    {
	addGroup(  new uiMaterialGrp(tabstack_->tabGroup(),
			survobj_,true,true,false,false,false,true,
			survobj_->usesColor()) );
    }

    if ( survobj_->canEnableTextureInterpolation() )
	addGroup( new uiTextureInterpolateGrp(tabstack_->tabGroup(),survobj_) );

    if ( survobj_->lineStyle() )
	addGroup( new uiLineStyleGrp(tabstack_->tabGroup(),survobj_)  );

    if ( survobj_->markerStyle() )
	addGroup( new uiMarkerStyleGrp(tabstack_->tabGroup(),survobj_) );

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,survobj_)
    if ( pdd )
	addGroup( new uiVisPlaneDataDisplayDragProp(tabstack_->tabGroup(),pdd));

    mDynamicCastGet(visSurvey::PolygonBodyDisplay*,pbd,survobj_)
    if ( pbd )
	addGroup( new uiVisPolygonSurfBezierDlg(tabstack_->tabGroup(),pbd) );

    if ( GetEnvVarYN("USE_FAULT_RETRIANGULATION") )
    {
	mDynamicCastGet(visSurvey::FaultDisplay*,fd,survobj_)
	if ( fd )
	    addGroup( new uiFaultDisplayOptGrp(tabstack_->tabGroup(),fd) );
    }

    setCancelText( uiString::emptyString() );

    mAttachCB( windowClosed, uiPropertiesDlg::dlgClosed );
}


uiPropertiesDlg::~uiPropertiesDlg()
{
    detachAllNotifiers();
}


void uiPropertiesDlg::dlgClosed( CallBacker* )
{
    mDynamicCastGet(visBase::VisualObject*,vo,survobj_)
    if ( vo )
	closeNotifier().trigger( vo->id(), this );
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


uiLineStyleGrp::~uiLineStyleGrp()
{}


void uiLineStyleGrp::changedCB( CallBacker* )
{
    survobj_->setLineStyle( field_->getStyle() );
}


bool uiLineStyleGrp::rejectOK( CallBacker* )
{
    survobj_->setLineStyle( backup_ );
    return true;
}

#define cDefMaxMarkerSize 18
//uiMarkerStyleGrp
uiMarkerStyleGrp::uiMarkerStyleGrp( uiParent* p, visSurvey::SurveyObject* so )
    : uiDlgGroup(p,tr("Marker style"))
    , survobj_(so)
{
    const MarkerStyle3D* ms = so ? so->markerStyle() : nullptr;
    if ( ms )
    {
	MarkerStyle3D::Type excludetype = MarkerStyle3D::None;
	stylefld_ = new uiMarkerStyle3D( this, true,
		Interval<int>(1,cDefMaxMarkerSize), 1, &excludetype );

	stylefld_->setMarkerStyle( *ms );
	stylefld_->enableColorSelection( !so->hasSpecificMarkerColor() );
	mAttachCB( *stylefld_->typeSel(), uiMarkerStyleGrp::typeSel );
	mAttachCB( *stylefld_->sliderMove(), uiMarkerStyleGrp::sizeChg );
	mAttachCB( *stylefld_->colSel(), uiMarkerStyleGrp::colSel );
    }
}


uiMarkerStyleGrp::~uiMarkerStyleGrp()
{
    detachAllNotifiers();
}


void uiMarkerStyleGrp::sizeChg( CallBacker* cb )
{
    typeSel( cb );
}


void uiMarkerStyleGrp::colSel( CallBacker* cb )
{
    typeSel( cb );
}


void uiMarkerStyleGrp::typeSel( CallBacker* )
{
    if ( !stylefld_ || !survobj_ )
	return;

    MarkerStyle3D mkstyle;
    stylefld_->getMarkerStyle( mkstyle );
    survobj_->setMarkerStyle( mkstyle );
}


// uiTextureInterpolateGrp
uiTextureInterpolateGrp::uiTextureInterpolateGrp( uiParent* p,
						  visSurvey::SurveyObject* so )
    : uiDlgGroup(p,tr("Texture"))
    , survobj_(so)
{
    if ( !so || !so->canEnableTextureInterpolation() )
	return;

    const bool intpenabled = so->textureInterpolationEnabled();

    textclasssify_ = new uiGenInput( this, tr("Data:   "),
	    BoolInpSpec(intpenabled,tr("Interpolation"),tr("Classification")) );
    textclasssify_->valueChanged.notify(
	    mCB(this,uiTextureInterpolateGrp,chgIntpCB) );
}


uiTextureInterpolateGrp::~uiTextureInterpolateGrp()
{}


void uiTextureInterpolateGrp::chgIntpCB( CallBacker* cb )
{
    if ( survobj_ && textclasssify_ )
	survobj_->enableTextureInterpolation( textclasssify_->getBoolValue() );
}


// uiMaterialGrp
static void initSlider( uiSlider* sldr, float val )
{
    if ( !sldr )
	return;

    sldr->setInterval( 0, 100, 10 );
    sldr->setValue( val*100.f );
}


uiMaterialGrp::uiMaterialGrp( uiParent* p, visSurvey::SurveyObject* so,
       bool ambience, bool diffusecolor, bool specularcolor,
       bool emmissivecolor, bool shininess, bool transparency, bool color )
    : uiDlgGroup(p,tr("Material"))
    , material_(dynamic_cast<visBase::VisualObject*>(so)->getMaterial())
    , survobj_(so)
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

    initSlider( ambslider_, material_->getAmbience() );
    initSlider( diffslider_, material_->getDiffIntensity() );
    initSlider( specslider_, material_->getSpecIntensity() );
    initSlider( emisslider_, material_->getEmmIntensity() );
    initSlider( shineslider_, material_->getShininess() );
    initSlider( transslider_, material_->getTransparency() );
}


uiMaterialGrp::~uiMaterialGrp()
{
    detachAllNotifiers();
}


void uiMaterialGrp::createSlider( bool domk, uiSlider*& slider,
				  const uiString& lbltxt )
{
    if ( !domk )
	return;

    uiSlider::Setup ss( lbltxt ); ss.withedit(true);
    slider = new uiSlider( this, ss,
	    BufferString( lbltxt.getFullString(), "slider").buf() );
    mAttachCB( slider->valueChanged, uiMaterialGrp::sliderMove );
    if ( prevobj_ )
	slider->attach( alignedBelow, prevobj_ );

    prevobj_ = slider;
}


void uiMaterialGrp::sliderMove( CallBacker* cb )
{
    mDynamicCastGet(uiSlider*,sldr,cb)
    if ( !sldr )
	return;

    if ( sldr == ambslider_ )
	material_->setAmbience( ambslider_->getFValue()/100 );
    else if ( sldr == diffslider_ )
	material_->setDiffIntensity( diffslider_->getFValue()/100 );
    else if ( sldr == specslider_ )
	material_->setSpecIntensity( specslider_->getFValue()/100 );
    else if ( sldr == emisslider_ )
	material_->setEmmIntensity( emisslider_->getFValue()/100 );
    else if ( sldr == shineslider_ )
	material_->setShininess( shineslider_->getFValue()/100 );
    else if ( sldr == transslider_ )
	material_->setTransparency( transslider_->getFValue()/100 );
}


void uiMaterialGrp::colorChangeCB( CallBacker* )
{
    if ( colinp_ )
	survobj_->setColor( colinp_->color() );
}
