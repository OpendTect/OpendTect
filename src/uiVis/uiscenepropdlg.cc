/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          February 2006
________________________________________________________________________

-*/

#include "uiscenepropdlg.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uifont.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
#include "uimsg.h"
#include "uiselsurvranges.h"
#include "uislider.h"
#include "ui3dviewer.h"

#include "vispolygonoffset.h"
#include "visscenecoltab.h"
#include "vissurvscene.h"

#include "datainpspec.h"
#include "fontdata.h"
#include "od_helpids.h"
#include "settingsaccess.h"


bool uiScenePropertyDlg::savestatus_ = true;

uiScenePropertyDlg::uiScenePropertyDlg( uiParent* p,
		const ObjectSet<ui3DViewer>& viewers, int curvwridx )
    : uiDialog(p,uiDialog::Setup(tr("Scene properties"),
				 uiString::emptyString(),
                                 mODHelpKey(mScenePropertyDlgHelpID) ))
    , hadsurveybox_(true)
    , hadannot_(true)
    , hadannotscale_(true)
    , hadannotgrid_(true)
    , oldbgcolor_(Color::Anthracite())
    , annotcolor_(Color::White())
    , oldmarkersize_(5)
    , oldmarkercolor_(Color::White())
    , hadanimation_(true)
    , viewers_(viewers)
    , curvwridx_(curvwridx)
    , scene_(0)
    , separationdlg_(0)
{
    enableSaveButton( tr("Apply to all") );
    setSaveButtonChecked( savestatus_ );

    if ( viewers_[curvwridx_] )
    {
	oldbgcolor_ = viewers_[curvwridx_]->getBackgroundColor();
	hadanimation_ = viewers_[curvwridx_]->isAnimationEnabled();
	wheeldisplaymode_ = viewers_[curvwridx_]->getWheelDisplayMode();

	mDynamicCast(visSurvey::Scene*, scene_, const_cast <visBase::Scene*>
			(viewers_[curvwridx_]->getScene()));
	if ( scene_ )
	{
	    hadsurveybox_ = scene_->isAnnotShown();
	    hadannot_ = scene_->isAnnotTextShown();
	    oldfont_ = scene_->getAnnotFont();
	    hadannotscale_ = scene_->isAnnotScaleShown();
	    hadannotgrid_ = scene_->isAnnotGridShown();
	    oldscale_ = scene_->getAnnotScale();
	    annotcolor_ = scene_->getAnnotColor();
	    oldmarkersize_ = scene_->getMarkerSize();
	    oldmarkercolor_ = scene_->getMarkerColor();
	    oldfactor_ = scene_->getPolygonOffset()->getFactor();
	    oldunits_ = scene_->getPolygonOffset()->getUnits();
	}
    }

    survboxfld_ = new uiCheckBox( this, tr("Survey box") );
    survboxfld_->setChecked( hadsurveybox_ );
    survboxfld_->activated.notify( mCB(this,uiScenePropertyDlg,updateCB) );

    annotfld_ = new uiCheckBox( this, tr("Annotation text") );
    annotfld_->setChecked( hadannot_ );
    annotfld_->attach( alignedBelow, survboxfld_ );
    annotfld_->activated.notify( mCB(this,uiScenePropertyDlg,updateCB) );
    annotfld_->setSensitive( survboxfld_->isChecked() );

    uiPushButton* annotfontbut = new uiPushButton(this, tr("Font"),
			mCB(this,uiScenePropertyDlg,selAnnotFontCB), false);
    annotfontbut->attach( rightOf, annotfld_ );

    annotscalefld_ = new uiCheckBox( this, tr("Annotation scale") );
    annotscalefld_->setChecked( hadannotscale_ );
    annotscalefld_->attach( alignedBelow, annotfld_ );
    annotscalefld_->activated.notify( mCB(this,uiScenePropertyDlg,updateCB) );
    annotscalefld_->setSensitive( survboxfld_->isChecked() );

    uiPushButton* scalebut = new uiPushButton(this, tr("Set"),
			mCB(this,uiScenePropertyDlg,setAnnotScaleCB), false);
    scalebut->attach( rightOf, annotscalefld_ );

    annotgridfld_ = new uiCheckBox( this, tr("Annotation grid") );
    annotgridfld_->setChecked( hadannotgrid_ );
    annotgridfld_->attach( alignedBelow, annotscalefld_ );
    annotgridfld_->activated.notify(mCB(this,uiScenePropertyDlg,updateCB) );
    annotgridfld_->setSensitive( survboxfld_->isChecked() );

    bgcolfld_ = new uiColorInput( this, uiColorInput::Setup( oldbgcolor_)
					.lbltxt(tr("Background color")) );
    bgcolfld_->attach( alignedBelow, annotgridfld_ );
    bgcolfld_->colorChanged.notify( mCB(this,uiScenePropertyDlg,updateCB) );

    markersizefld_ = new uiSlider( this,
				   uiSlider::Setup(tr("Mouse marker size")),
				   "Marker size slider" );
    markersizefld_->setMinValue( 1 );
    markersizefld_->setMaxValue( 10 );
    markersizefld_->setValue( oldmarkersize_ );
    markersizefld_->attach( alignedBelow, bgcolfld_ );
    markersizefld_->valueChanged.notify(
					mCB(this,uiScenePropertyDlg,updateCB) );

    markercolfld_ = new uiColorInput( this,
	uiColorInput::Setup(oldmarkercolor_).lbltxt(tr("Mouse marker color")) );
    markercolfld_->attach( alignedBelow, markersizefld_ );
    markercolfld_->colorChanged.notify( mCB(this,uiScenePropertyDlg,updateCB) );

    annotcolfld_ = new uiColorInput( this,
	    uiColorInput::Setup(annotcolor_).lbltxt(tr("Annotation color")) );
    annotcolfld_->attach( alignedBelow, markercolfld_ );
    annotcolfld_->colorChanged.notify( mCB(this,uiScenePropertyDlg,updateCB) );

    uiPushButton* ltbutton =
	new uiPushButton( this, tr("Line/Surface separation"),
			  mCB(this,uiScenePropertyDlg,setOffsetCB ), false );
    ltbutton->attach( alignedBelow, annotcolfld_ );

    animationfld_ = new uiCheckBox( this, tr("Allow spin animation") );
    animationfld_->setChecked( hadanimation_ );
    animationfld_->activated.notify( mCB(this,uiScenePropertyDlg,updateCB) );
    animationfld_->attach( alignedBelow, ltbutton );

    BufferStringSet modes;
    modes.add( "Hide" ).add( "Always show" ).add( "Show on mouse hover" );
    wheelmodefld_ = new uiGenInput( this, tr("Zoom/Rotate Wheels Display"),
				    StringListInpSpec(modes) );
    wheelmodefld_->valuechanged.notify( mCB(this,uiScenePropertyDlg,updateCB) );
    wheelmodefld_->setValue( wheeldisplaymode_ );
    wheelmodefld_->attach( alignedBelow, animationfld_ );
}


uiScenePropertyDlg::~uiScenePropertyDlg()
{
}


void uiScenePropertyDlg::selAnnotFontCB( CallBacker* )
{
    if ( !scene_ )
	return;

    FontData fontdata = scene_->getAnnotFont();
    if ( selectFont(fontdata,this) )
	scene_->setAnnotFont( fontdata );

    ui3DViewer* vwr = const_cast<ui3DViewer*> (viewers_[curvwridx_]);
    if ( vwr )
	vwr->setAnnotationFont( fontdata );
}


struct uiScaleDlg : public uiDialog
{ mODTextTranslationClass(uiScaleDlg);
public:
uiScaleDlg( uiParent* p, const TrcKeyZSampling& scale, const char* zdomkey )
    : uiDialog(p,Setup(tr("Set Annotation Scale"),mNoDlgTitle,mNoHelpKey))
{
    rangefld_ = new uiSelSubvol( this, true, zdomkey );
    rangefld_->setSampling( scale );
}


bool acceptOK(CallBacker *)
{
    newscale_ = rangefld_->getSampling();
    return true;
}

TrcKeyZSampling newscale_;
uiSelSubvol*	rangefld_;

};


void uiScenePropertyDlg::setAnnotScaleCB( CallBacker* )
{
    uiScaleDlg dlg( this, scene_->getAnnotScale(), scene_->zDomainKey() );
    if ( !dlg.go() ) return;

    scene_->setAnnotScale( dlg.newscale_ );
}


void uiScenePropertyDlg::updateCB( CallBacker* )
{
    if ( scene_ )
        updateScene( scene_ );

    ui3DViewer* vwr = const_cast<ui3DViewer*> (viewers_[curvwridx_]);
    if ( vwr )
    {
	vwr->setBackgroundColor( bgcolfld_->color() );
	vwr->setAnnotationColor( annotcolfld_->color() );
	vwr->enableAnimation( animationfld_->isChecked() );
	vwr->setWheelDisplayMode(
		(ui3DViewer::WheelMode)wheelmodefld_->getIntValue() );
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
	scene_->getPolygonOffset()->setFactor( separationdlg_->getFValue(0) );
	scene_->getPolygonOffset()->setUnits( separationdlg_->getFValue(1) );
    }
}


bool uiScenePropertyDlg::rejectOK( CallBacker* )
{
    if ( scene_ )
    {
        scene_->showAnnot( hadsurveybox_ );
	scene_->showAnnotScale( hadannotscale_ );
	scene_->setAnnotFont( oldfont_ );
	scene_->showAnnotText( hadannot_ );
	scene_->showAnnotGrid( hadannotgrid_ );
	scene_->setAnnotScale( oldscale_ );
	scene_->setMarkerSize( oldmarkersize_ );
	scene_->setMarkerColor( oldmarkercolor_ );
        scene_->setAnnotColor( annotcolor_ );
	scene_->getSceneColTab()->setLegendColor( annotcolor_ );
	scene_->getPolygonOffset()->setUnits( oldunits_ );
	scene_->getPolygonOffset()->setFactor( oldfactor_ );
    }

    ui3DViewer* vwr = const_cast<ui3DViewer*> (viewers_[curvwridx_]);
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
    if ( !separationdlg_ )
    {
	ObjectSet<uiGenInputDlgEntry>* entries =
	    new ObjectSet<uiGenInputDlgEntry>;
	(*entries) += new uiGenInputDlgEntry( uiStrings::sFactor(),
		new FloatInpSpec( scene_->getPolygonOffset()->getFactor() ));
	(*entries) += new uiGenInputDlgEntry( uiStrings::sUnit(mPlural),
		new FloatInpSpec( scene_->getPolygonOffset()->getUnits() ));

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
	    updateCB(0);
	    break;
	}

	uiMSG().error(tr("Both Factor and Units must be defined and "
			 "more than 1.") );
    }
}


bool uiScenePropertyDlg::acceptOK( CallBacker* )
{
    if ( scene_ )
    {
	scene_->savePropertySettings();
	FontList().get( FontData::Graphics3D ).setFontData(
						       scene_->getAnnotFont() );
	Settings& setts = Settings::common();
	FontList().update( setts );
    }

    if ( viewers_[curvwridx_] )
	viewers_[curvwridx_]->savePropertySettings();

    savestatus_ = saveButtonChecked();
    if ( !savestatus_ )
	return true;

    for ( int idx=0; idx<viewers_.size() && viewers_[idx]; idx++ )
    {
        mDynamicCastGet(visSurvey::Scene*, scene,const_cast <visBase::Scene*>
                        (viewers_[idx]->getScene()));
        updateScene( scene );

	ui3DViewer* vwr = const_cast<ui3DViewer*> (viewers_[idx]);
	if ( vwr )
	{
	    vwr->setBackgroundColor( bgcolfld_->color() );
	    vwr->setAnnotationColor( annotcolfld_->color() );
	    vwr->enableAnimation( animationfld_->isChecked() );
	    vwr->setAnnotationFont( scene->getAnnotFont() );
	    vwr->setWheelDisplayMode(
			(ui3DViewer::WheelMode)wheelmodefld_->getIntValue() );
	}
    }

    return true;
}
