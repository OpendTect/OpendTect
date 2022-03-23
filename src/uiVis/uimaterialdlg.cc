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
#include "visfaultsticksetdisplay.h"
#include "visemsticksetdisplay.h"
#include "vishorizondisplay.h"
#include "visplanedatadisplay.h"
#include "vispolygonbodydisplay.h"
#include "visemobjdisplay.h"
#include "envvars.h"
#include "od_helpids.h"


// uiPropertiesDlg
uiPropertiesDlg::uiPropertiesDlg( uiParent* p, visSurvey::SurveyObject* so )
    : uiTabStackDlg(p,uiDialog::Setup(tr("Display properties"),
				mNoDlgTitle,mODHelpKey(mPropertiesDlgHelpID)))
    , survobj_(so)
    , visobj_(dynamic_cast<visBase::VisualObject*>(so))
{
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

    mDynamicCastGet( visSurvey::FaultDisplay*, ftdspl, so );
    mDynamicCastGet( visSurvey::FaultStickSetDisplay*,ftssdspl,so );
    mDynamicCastGet(visSurvey::PolygonBodyDisplay*,plg,so);
    if ( ftdspl || ftssdspl || plg )
	addGroup( new uiMarkerStyleGrp( tabstack_->tabGroup(), survobj_ ) );

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,so);
    if ( pdd )
	addGroup( new uiVisPlaneDataDisplayDragProp(tabstack_->tabGroup(),pdd));

    if ( plg )
	addGroup( new uiVisPolygonSurfBezierDlg(tabstack_->tabGroup(),plg) );

    if ( GetEnvVarYN("USE_FAULT_RETRIANGULATION") )
    {
	mDynamicCastGet(visSurvey::FaultDisplay*,flt,so);
	if ( flt )
	    addGroup( new uiFaultDisplayOptGrp(tabstack_->tabGroup(),flt) );
    }

    setCancelText( uiString::emptyString() );
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


bool uiLineStyleGrp::rejectOK( CallBacker* )
{
    survobj_->setLineStyle( backup_ );
    return true;
}

#define cDefMaxMarkerSize 18
//uiMarkerStyleGrp
uiMarkerStyleGrp::uiMarkerStyleGrp( uiParent* p, visSurvey::SurveyObject* so )
    : uiDlgGroup(p,tr("Marker style"))
    , survobj_( so )

{
    visSurvey::StickSetDisplay* ssd = getDisplay();
    mDynamicCastGet(visSurvey::PolygonBodyDisplay*,pbd,so)
    if ( ssd || pbd )
    {
	MarkerStyle3D::Type excludetype = MarkerStyle3D::None;
	stylefld_ = new uiMarkerStyle3D( this, true,
			Interval<int>(1,cDefMaxMarkerSize), 1, &excludetype );

	const MarkerStyle3D* ms = nullptr;
	if ( ssd )
	    ms = ssd->markerStyle();
	if ( pbd )
	    ms = pbd->markerStyle();

	if ( ms )
	    stylefld_->setMarkerStyle( *ms );

	stylefld_->typeSel()->notify( mCB(this,uiMarkerStyleGrp,typeSel) );
	stylefld_->sliderMove()->notify( mCB(this,uiMarkerStyleGrp,sizeChg) );
	stylefld_->colSel()->notify( mCB(this,uiMarkerStyleGrp,colSel) );
	stylefld_->enableColorSelection( pbd );
    }

}


void uiMarkerStyleGrp::sizeChg( CallBacker* cb )
{
    typeSel( cb );
}


void uiMarkerStyleGrp::typeSel( CallBacker* )
{
    MarkerStyle3D ms;
    stylefld_->getMarkerStyle( ms );
    visSurvey::StickSetDisplay* ssd = getDisplay();
    if ( ssd )
	return ssd->setMarkerStyle( ms );

    mDynamicCastGet(visSurvey::PolygonBodyDisplay*,pbd,survobj_)
    if ( pbd )
	pbd->setMarkerStyle( ms );
}


void uiMarkerStyleGrp::colSel( CallBacker* cb )
{
    typeSel( cb );
}


visSurvey::StickSetDisplay* uiMarkerStyleGrp::getDisplay()
{
    mDynamicCastGet(visSurvey::FaultDisplay*,fd,survobj_)
    if ( fd )
	return dCast(visSurvey::StickSetDisplay*,fd);

    mDynamicCastGet(visSurvey::FaultStickSetDisplay*,fssd,survobj_)
    if ( fssd )
	return dCast(visSurvey::StickSetDisplay*,fssd);

    return nullptr;
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
    textclasssify_->valuechanged.notify(
	    mCB(this,uiTextureInterpolateGrp,chgIntpCB) );
}


void uiTextureInterpolateGrp::chgIntpCB( CallBacker* cb )
{
    if ( survobj_ && textclasssify_ )
	survobj_->enableTextureInterpolation( textclasssify_->getBoolValue() );
}


// uiMaterialGrp

#define mFinalize( sldr, fn ) \
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
    , ambslider_(nullptr)
    , diffslider_(nullptr)
    , specslider_(nullptr)
    , emisslider_(nullptr)
    , shineslider_(nullptr)
    , transslider_(nullptr)
    , colinp_(nullptr)
    , prevobj_(nullptr)
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

    mFinalize( ambslider_, getAmbience );
    mFinalize( diffslider_, getDiffIntensity );
    mFinalize( specslider_, getSpecIntensity );
    mFinalize( emisslider_, getEmmIntensity );
    mFinalize( shineslider_, getShininess );
    mFinalize( transslider_, getTransparency );
}


void uiMaterialGrp::createSlider( bool domk, uiSlider*& slider,
				  const uiString& lbltxt )
{
    if ( !domk ) return;

    uiSlider::Setup ss( lbltxt ); ss.withedit(true);
    slider = new uiSlider( this, ss,
	    BufferString( lbltxt.getFullString(), "slider").buf() );
    slider->valueChanged.notify( mCB(this,uiMaterialGrp,sliderMove) );
    if ( prevobj_ ) slider->attach( alignedBelow, prevobj_ );
    prevobj_ = slider;
}



void uiMaterialGrp::sliderMove( CallBacker* cb )
{
    mDynamicCastGet(uiSlider*,sldr,cb)
    if ( !sldr ) return;

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

void uiMaterialGrp::colorChangeCB(CallBacker*)
{ if ( colinp_ ) survobj_->setColor( colinp_->color() ); }
