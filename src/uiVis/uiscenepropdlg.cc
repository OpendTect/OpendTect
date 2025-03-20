/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiscenepropdlg.h"

#include "ui3dviewer.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uifont.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
#include "uimsg.h"
#include "uiselsurvranges.h"
#include "uislider.h"

#include "datainpspec.h"
#include "od_helpids.h"
#include "settingsaccess.h"
#include "vispolygonoffset.h"
#include "visscenecoltab.h"
#include "zdomain.h"


bool uiScenePropertyDlg::savestatus_ = true;

uiScenePropertyDlg::uiScenePropertyDlg( uiParent* p,
		const ObjectSet<ui3DViewer>& viewers, int curvwridx )
    : uiDialog(p,uiDialog::Setup(tr("Scene properties"),
				 uiString::emptyString(),
                                 mODHelpKey(mScenePropertyDlgHelpID) ))
    , viewers_(viewers)
    , curvwridx_(curvwridx)
{
    enableSaveButton( tr("Apply to all") );
    setSaveButtonChecked( savestatus_ );

    const ui3DViewer* viewer = viewers_[curvwridx_];
    if ( viewer )
    {
	oldbgcolor_ = viewer->getBackgroundColor();
	hadanimation_ = viewer->isAnimationEnabled();
	wheeldisplaymode_ = viewer->getWheelDisplayMode();

	auto* visscene = const_cast<visBase::Scene*>( viewer->getScene() );
	RefMan<visSurvey::Scene> scene = dCast(visSurvey::Scene*,visscene);
	if ( scene )
	{
	    hadsurveybox_ = scene->isAnnotShown();
	    hadannot_ = scene->isAnnotTextShown();
	    oldfont_ = scene->getAnnotFont();
	    hadannotscale_ = scene->isAnnotScaleShown();
	    hadannotgrid_ = scene->isAnnotGridShown();
	    oldscale_ = scene->getAnnotScale( false );
	    annotcolor_ = scene->getAnnotColor();
	    oldmarkersize_ = scene->getMarkerSize();
	    oldmarkercolor_ = scene->getMarkerColor();
	    oldfactor_ = scene->getPolygonOffset()->getFactor();
	    oldunits_ = scene->getPolygonOffset()->getUnits();
	    scene_ = scene;
	}
    }

    survboxfld_ = new uiCheckBox( this, tr("Survey box") );
    survboxfld_->setChecked( hadsurveybox_ );
    mAttachCB( survboxfld_->activated, uiScenePropertyDlg::updateCB );

    annotfld_ = new uiCheckBox( this, tr("Annotation text") );
    annotfld_->setChecked( hadannot_ );
    annotfld_->attach( alignedBelow, survboxfld_ );
    mAttachCB( annotfld_->activated, uiScenePropertyDlg::updateCB );
    annotfld_->setSensitive( survboxfld_->isChecked() );

    auto* annotfontbut = new uiPushButton(this, tr("Font"),
			mCB(this,uiScenePropertyDlg,selAnnotFontCB), false);
    annotfontbut->attach( rightOf, annotfld_ );

    annotscalefld_ = new uiCheckBox( this, tr("Annotation scale") );
    annotscalefld_->setChecked( hadannotscale_ );
    annotscalefld_->attach( alignedBelow, annotfld_ );
    mAttachCB( annotscalefld_->activated, uiScenePropertyDlg::updateCB );
    annotscalefld_->setSensitive( survboxfld_->isChecked() );

    auto* scalebut = new uiPushButton(this, tr("Set"),
			mCB(this,uiScenePropertyDlg,setAnnotScaleCB), false);
    scalebut->attach( rightOf, annotscalefld_ );

    annotgridfld_ = new uiCheckBox( this, tr("Annotation grid") );
    annotgridfld_->setChecked( hadannotgrid_ );
    annotgridfld_->attach( alignedBelow, annotscalefld_ );
    mAttachCB( annotgridfld_->activated, uiScenePropertyDlg::updateCB );
    annotgridfld_->setSensitive( survboxfld_->isChecked() );

    bgcolfld_ = new uiColorInput( this, uiColorInput::Setup( oldbgcolor_)
					.lbltxt(tr("Background color")) );
    bgcolfld_->attach( alignedBelow, annotgridfld_ );
    mAttachCB( bgcolfld_->colorChanged, uiScenePropertyDlg::updateCB );

    markersizefld_ = new uiSlider( this,
				   uiSlider::Setup(tr("Mouse marker size")),
				   "Marker size slider" );
    markersizefld_->setMinValue( 1 );
    markersizefld_->setMaxValue( 10 );
    markersizefld_->setValue( oldmarkersize_ );
    markersizefld_->attach( alignedBelow, bgcolfld_ );
    mAttachCB( markersizefld_->valueChanged, uiScenePropertyDlg::updateCB );

    markercolfld_ = new uiColorInput( this,
	uiColorInput::Setup(oldmarkercolor_).lbltxt(tr("Mouse marker color")) );
    markercolfld_->attach( alignedBelow, markersizefld_ );
    mAttachCB( markercolfld_->colorChanged, uiScenePropertyDlg::updateCB );

    annotcolfld_ = new uiColorInput( this,
	    uiColorInput::Setup(annotcolor_).lbltxt(tr("Annotation color")) );
    annotcolfld_->attach( alignedBelow, markercolfld_ );
    mAttachCB( annotcolfld_->colorChanged, uiScenePropertyDlg::updateCB );

    auto* ltbutton = new uiPushButton( this, tr("Line/Surface separation"),
			  mCB(this,uiScenePropertyDlg,setOffsetCB ), false );
    ltbutton->attach( alignedBelow, annotcolfld_ );

    animationfld_ = new uiCheckBox( this, tr("Allow spin animation") );
    animationfld_->setChecked( hadanimation_ );
    mAttachCB( animationfld_->activated, uiScenePropertyDlg::updateCB );
    animationfld_->attach( alignedBelow, ltbutton );

    BufferStringSet modes;
    modes.add( "Hide" ).add( "Always show" ).add( "Show on mouse hover" );
    wheelmodefld_ = new uiGenInput( this, tr("Zoom/Rotate Wheels Display"),
				    StringListInpSpec(modes) );
    mAttachCB( wheelmodefld_->valueChanged, uiScenePropertyDlg::updateCB );
    wheelmodefld_->setValue( sCast(int,wheeldisplaymode_) );
    wheelmodefld_->attach( alignedBelow, animationfld_ );
}


uiScenePropertyDlg::~uiScenePropertyDlg()
{
    detachAllNotifiers();
}


void uiScenePropertyDlg::selAnnotFontCB( CallBacker* )
{
    RefMan<visSurvey::Scene> scene = scene_.get();
    if ( !scene )
	return;

    FontData fontdata = scene->getAnnotFont();
    if ( selectFont(fontdata,this) )
	scene->setAnnotFont( fontdata );

    auto* vwr = const_cast<ui3DViewer*>( viewers_[curvwridx_] );
    if ( vwr )
	vwr->setAnnotationFont( fontdata );
}


class uiScaleDlg : public uiDialog
{ mODTextTranslationClass(uiScaleDlg);
public:
uiScaleDlg( uiParent* p, const TrcKeyZSampling& scale,
	    const ZDomain::Info& zinfo )
    : uiDialog(p,Setup(tr("Set Annotation Scale"),mNoDlgTitle,mNoHelpKey))
{
    rangefld_ = new uiSelSubvol( this, true, zinfo );
    rangefld_->setSampling( scale );
}


bool acceptOK( CallBacker * ) override
{
    newscale_ = rangefld_->getSampling();
    return true;
}

TrcKeyZSampling newscale_;
uiSelSubvol*	rangefld_;

};


void uiScenePropertyDlg::setAnnotScaleCB( CallBacker* )
{
    RefMan<visSurvey::Scene> scene = scene_.get();
    if ( !scene )
	return;

    const ZDomain::Info& zdom = scene->zDomainInfo();
    const ZDomain::Info* zinfo = nullptr;
    bool usedefaultscale = true;
    if ( scene->zDomainInfo().isDepth() )
    {
	zinfo = &ZDomain::DefaultDepth();
	usedefaultscale = false;
    }
    else
	zinfo = &zdom;

    uiScaleDlg dlg( this, scene->getAnnotScale(usedefaultscale), *zinfo );
    if ( dlg.go() != uiDialog::Accepted )
	return;

    TrcKeyZSampling& newscale = dlg.newscale_;
    TypeSet<double> zscalefacset( 3, 1. );
    if ( zdom.isDepth() &&
	 ((zdom.isDepthMeter() && zinfo->isDepthFeet()) ||
	  (zdom.isDepthFeet() && !zinfo->isDepthFeet())) )
    {
	if ( zdom.isDepthMeter() )
	    zscalefacset[2] = mToFeetFactorD;
	else if ( zdom.isDepthFeet() )
	    zscalefacset[2] = mFromFeetFactorD;
    }
    else
    {
	if ( usedefaultscale )
	    newscale.zsamp_.scale( zinfo->userFactor() );

	zscalefacset[2] = zinfo->userFactor();
    }

    scene->setAnnotScale( newscale, zscalefacset.arr(), zscalefacset.size() );
}


void uiScenePropertyDlg::updateCB( CallBacker* )
{
    RefMan<visSurvey::Scene> scene = scene_.get();
    if ( scene )
	updateScene( scene.ptr() );

    auto* vwr = const_cast<ui3DViewer*>( viewers_[curvwridx_] );
    if ( vwr )
    {
	vwr->setBackgroundColor( bgcolfld_->color() );
	vwr->setAnnotationColor( annotcolfld_->color() );
	vwr->enableAnimation( animationfld_->isChecked() );
	vwr->setWheelDisplayMode(
		(OD::WheelMode)wheelmodefld_->getIntValue() );
    }

    annotfld_->setSensitive( survboxfld_->isChecked() );
    annotscalefld_->setSensitive( survboxfld_->isChecked() );
    annotgridfld_->setSensitive( survboxfld_->isChecked()  );
}


void uiScenePropertyDlg::updateScene( visSurvey::Scene* scene )
{
    if ( !scene )
	return;

    scene->showAnnot( survboxfld_->isChecked() );
    scene->showAnnotScale( annotscalefld_->isChecked() );
    scene->showAnnotGrid( annotgridfld_->isChecked() );
    scene->showAnnotText( annotfld_->isChecked() );

    scene->setMarkerSize( markersizefld_->getFValue() );
    scene->setMarkerColor( markercolfld_->color() );
    scene->setAnnotColor( annotcolfld_->color() );
    scene->getSceneColTab()->setLegendColor( annotcolfld_->color() );
    if ( separationdlg_ )
    {
	scene->getPolygonOffset()->setFactor( separationdlg_->getFValue(0) );
	scene->getPolygonOffset()->setUnits( separationdlg_->getFValue(1) );
    }
}


bool uiScenePropertyDlg::rejectOK( CallBacker* )
{
    RefMan<visSurvey::Scene> scene = scene_.get();
    if ( scene )
    {
	scene->showAnnot( hadsurveybox_ );
	scene->showAnnotScale( hadannotscale_ );
	scene->setAnnotFont( oldfont_ );
	scene->showAnnotText( hadannot_ );
	scene->showAnnotGrid( hadannotgrid_ );
	scene->setAnnotScale( oldscale_ );
	scene->setMarkerSize( oldmarkersize_ );
	scene->setMarkerColor( oldmarkercolor_ );
	scene->setAnnotColor( annotcolor_ );
	scene->getSceneColTab()->setLegendColor( annotcolor_ );
	scene->getPolygonOffset()->setUnits( oldunits_ );
	scene->getPolygonOffset()->setFactor( oldfactor_ );
    }

    auto* vwr = const_cast<ui3DViewer*>( viewers_[curvwridx_] );
    if ( vwr )
    {
	vwr->setBackgroundColor( oldbgcolor_ );
	vwr->setAnnotationColor( annotcolor_ );
	vwr->setAnnotationFont( oldfont_ );
	vwr->enableAnimation( hadanimation_ );
	vwr->setWheelDisplayMode( wheeldisplaymode_ );
    }

    return true;
}


void uiScenePropertyDlg::setOffsetCB( CallBacker* )
{
    RefMan<visSurvey::Scene> scene = scene_.get();
    if ( !separationdlg_ && scene )
    {
	auto* entries = new ObjectSet<uiGenInputDlgEntry>;
	entries->add( new uiGenInputDlgEntry( uiStrings::sFactor(),
		new FloatInpSpec( scene->getPolygonOffset()->getFactor() )) );
	entries->add( new uiGenInputDlgEntry( uiStrings::sUnit(mPlural),
		new FloatInpSpec( scene->getPolygonOffset()->getUnits() )) );

	separationdlg_ =
	    new uiGenInputDlg( this, tr("Line/Surface separation"), entries );
	separationdlg_->setHelpKey(
            mODHelpKey(mScenePropertyDlgLineSurfSepHelpID) );
    }

    while ( true )
    {
	if ( !separationdlg_->go() )
	{
	    separationdlg_->getFld(0)->setValue( oldfactor_ );
	    separationdlg_->getFld(1)->setValue( oldunits_ );
	}

	const float factor = separationdlg_->getFValue(0);
	const float units = separationdlg_->getFValue(1);

	if ( !mIsUdf(factor) && factor>=1 &&
	     !mIsUdf(units) && units>=1 )
	{
	    updateCB(nullptr);
	    break;
	}

	uiMSG().error(tr("Both Factor and Units must be defined and "
			 "more than 1.") );
    }
}


bool uiScenePropertyDlg::acceptOK( CallBacker* )
{
    RefMan<visSurvey::Scene> curscene = scene_.get();
    if ( curscene )
    {
	curscene->savePropertySettings();
	FontList().get( FontData::Graphics3D )
		  .setFontData( curscene->getAnnotFont() );
	Settings& setts = Settings::common();
	FontList().update( setts );
    }

    if ( viewers_[curvwridx_] )
	viewers_[curvwridx_]->savePropertySettings();

    savestatus_ = saveButtonChecked();
    if ( !savestatus_ )
	return true;

    for ( int idx=0; idx<viewers_.size(); idx++ )
    {
	auto* vwr = const_cast<ui3DViewer*>( viewers_[idx] );
	if ( !vwr )
	    break;

	auto* visscene = const_cast<visBase::Scene*>( vwr->getScene() );
	RefMan<visSurvey::Scene> scene = dCast(visSurvey::Scene*,visscene);
	updateScene( scene.ptr() );

	vwr->setBackgroundColor( bgcolfld_->color() );
	vwr->setAnnotationColor( annotcolfld_->color() );
	vwr->enableAnimation( animationfld_->isChecked() );
	if ( scene )
	    vwr->setAnnotationFont( scene->getAnnotFont() );

	vwr->setWheelDisplayMode(
		    (OD::WheelMode)wheelmodefld_->getIntValue() );
    }

    return true;
}
