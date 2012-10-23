/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          February 2006
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiscenepropdlg.h"

#include "datainpspec.h"
#include "vissurvscene.h"
#include "vispolygonoffset.h"
#include "visscenecoltab.h"
#include "uibutton.h"
#include "uigeninputdlg.h"
#include "uigeninput.h"
#include "uicolor.h"
#include "uislider.h"
#include "ui3dviewer.h"
#include "uimsg.h"

bool uiScenePropertyDlg::savestatus_ = true;


uiScenePropertyDlg::uiScenePropertyDlg( uiParent* p, 
		const ObjectSet<ui3DViewer>& viewers, int curvwridx )
    : uiDialog(p,uiDialog::Setup("Scene properties","","50.0.5"))
    , hadsurveybox_(true)
    , hadannot_(true)
    , hadannotscale_(true)
    , hadannotgrid_(true)
    , oldbgcolor_(Color::Black())
    , annotcolor_(Color::White())
    , oldmarkersize_(5)
    , oldmarkercolor_(Color::White())
    , hadanimation_(true)
    , viewers_(viewers)
    , curvwridx_(curvwridx)
    , scene_(0)
    , separationdlg_(0)
{
    enableSaveButton( "Apply to all scenes");
    setSaveButtonChecked( savestatus_ );

    if ( viewers_[curvwridx_] )
    {
	oldbgcolor_ = viewers_[curvwridx_]->getBackgroundColor();
	hadanimation_ = viewers_[curvwridx_]->isAnimationEnabled();

        mDynamicCast(visSurvey::Scene*, scene_, const_cast <visBase::Scene*> 
			(viewers_[curvwridx_]->getScene()));
	if ( scene_ )
	{
	    hadsurveybox_ = scene_->isAnnotShown();
	    hadannot_ = scene_->isAnnotTextShown();
	    hadannotscale_ = scene_->isAnnotScaleShown();
	    hadannotgrid_ = scene_->isAnnotGridShown();
	    annotcolor_ = scene_->getAnnotColor();
	    oldmarkersize_ = scene_->getMarkerSize();
	    oldmarkercolor_ = scene_->getMarkerColor();
	    oldfactor_ = scene_->getPolygonOffset()->getFactor();
	    oldunits_ = scene_->getPolygonOffset()->getUnits();
    	}
    }

    survboxfld_ = new uiCheckBox( this, "Survey box" );
    survboxfld_->setChecked( hadsurveybox_ );
    survboxfld_->activated.notify( mCB(this,uiScenePropertyDlg,updateCB) );

    annotfld_ = new uiCheckBox( this, "Annotation text" );
    annotfld_->setChecked( hadannot_ );
    annotfld_->attach( alignedBelow, survboxfld_ );
    annotfld_->activated.notify( mCB(this,uiScenePropertyDlg,updateCB) );
    annotfld_->setSensitive( survboxfld_->isChecked() );

    annotscalefld_ = new uiCheckBox( this, "Annotation scale" );
    annotscalefld_->setChecked( hadannotscale_ );
    annotscalefld_->attach( alignedBelow, annotfld_ );
    annotscalefld_->activated.notify( mCB(this,uiScenePropertyDlg,updateCB) );
    annotscalefld_->setSensitive( survboxfld_->isChecked() );

    annotgridfld_ = new uiCheckBox( this, "Annotation grid" );
    annotgridfld_->setChecked( hadannotgrid_ );
    annotgridfld_->attach( alignedBelow, annotscalefld_ );
    annotgridfld_->activated.notify(mCB(this,uiScenePropertyDlg,updateCB) );
    annotgridfld_->setSensitive( survboxfld_->isChecked() );

    bgcolfld_ = new uiColorInput( this, uiColorInput::Setup( oldbgcolor_)
					.lbltxt("Background color") );
    bgcolfld_->attach( alignedBelow, annotgridfld_ );
    bgcolfld_->colorChanged.notify( mCB(this,uiScenePropertyDlg,updateCB) );

    markersizefld_ = new uiSliderExtra( this,
				uiSliderExtra::Setup("Mouse marker size"),
				"Marker size slider" );
    markersizefld_->sldr()->setMinValue( 1 );
    markersizefld_->sldr()->setMaxValue( 10 );
    markersizefld_->sldr()->setValue( oldmarkersize_ );
    markersizefld_->attach( alignedBelow, bgcolfld_ );
    markersizefld_->sldr()->valueChanged.notify(
	    				mCB(this,uiScenePropertyDlg,updateCB) );

    markercolfld_ = new uiColorInput( this,
	    uiColorInput::Setup(oldmarkercolor_).lbltxt("Mouse marker color") );
    markercolfld_->attach( alignedBelow, markersizefld_ );
    markercolfld_->colorChanged.notify( mCB(this,uiScenePropertyDlg,updateCB) );
    
    annotcolfld_ = new uiColorInput( this,
	    uiColorInput::Setup(annotcolor_).lbltxt("Annotation color") );
    annotcolfld_->attach( alignedBelow, markercolfld_ );
    annotcolfld_->colorChanged.notify( mCB(this,uiScenePropertyDlg,updateCB) );

    uiPushButton* ltbutton = new uiPushButton(this, "Line/Surface separation",
	    		mCB(this,uiScenePropertyDlg,setOffsetCB ), false );
    ltbutton->attach( alignedBelow, annotcolfld_ );

    animationfld_ = new uiCheckBox( this, "Allow spin animation" );
    animationfld_->setChecked( hadanimation_ );
    animationfld_->activated.notify( mCB(this,uiScenePropertyDlg,updateCB) );
    animationfld_->attach( alignedBelow, ltbutton );
}


uiScenePropertyDlg::~uiScenePropertyDlg()
{
    delete separationdlg_;
}


void uiScenePropertyDlg::updateCB( CallBacker* )
{
    if ( scene_ )
        updateScene( scene_ );

    ui3DViewer* vwr = const_cast<ui3DViewer*> (viewers_[curvwridx_]);
    if ( vwr )
    {
	vwr->setBackgroundColor( bgcolfld_->color() );
	vwr->setAxisAnnotColor( annotcolfld_->color() );
	vwr->enableAnimation( animationfld_->isChecked() );
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

    scene->setMarkerSize( markersizefld_->sldr()->getValue() );
    scene->setMarkerColor( markercolfld_->color() );
    scene->setAnnotColor( annotcolfld_->color() );
    scene->getSceneColTab()->setLegendColor( annotcolfld_->color() );
    if ( separationdlg_ )
    {
	scene_->getPolygonOffset()->setFactor( separationdlg_->getfValue(0) );
	scene_->getPolygonOffset()->setUnits( separationdlg_->getfValue(1) );
    }
}


bool uiScenePropertyDlg::rejectOK( CallBacker* )
{
    if ( scene_ )
    {
        scene_->showAnnot( hadsurveybox_ );
	scene_->showAnnotScale( hadannot_ );
	scene_->showAnnotText( hadannotscale_ );
	scene_->showAnnotGrid( hadannotgrid_ );
	scene_->setMarkerSize( oldmarkersize_ );
	scene_->setMarkerColor( oldmarkercolor_ );
        scene_->setAnnotColor( annotcolor_ );
	scene_->getPolygonOffset()->setUnits( oldunits_ );
	scene_->getPolygonOffset()->setFactor( oldfactor_ );
    }

    ui3DViewer* vwr = const_cast<ui3DViewer*> (viewers_[curvwridx_]);
    if ( vwr )
    {
	vwr->setBackgroundColor( oldbgcolor_ );
	vwr->enableAnimation( hadanimation_ );
    }

    return true;
}


void uiScenePropertyDlg::setOffsetCB( CallBacker* )
{
    if ( !separationdlg_ )
    {
	ObjectSet<uiGenInputDlgEntry>* entries =
	    new ObjectSet<uiGenInputDlgEntry>;
	(*entries) += new uiGenInputDlgEntry( visBase::Scene::sKeyFactor(),
		new FloatInpSpec( scene_->getPolygonOffset()->getFactor() ));
	(*entries) += new uiGenInputDlgEntry( visBase::Scene::sKeyUnits(),
		new FloatInpSpec( scene_->getPolygonOffset()->getUnits() ));

	separationdlg_ =
	    new uiGenInputDlg( this, "Line/Surface separation", entries );
	separationdlg_->setHelpID( "od:50.0.19");
    }

    while ( true )
    {
	if ( !separationdlg_->go() )
	{
	    separationdlg_->getFld(0)->setValue( oldfactor_ );
	    separationdlg_->getFld(1)->setValue( oldunits_ );
	}

	const float factor = separationdlg_->getfValue(0);
	const float units = separationdlg_->getfValue(1);

	if ( !mIsUdf(factor) && factor>=1 &&
	     !mIsUdf(units) && units>=1 )
	{
	    updateCB(0);
	    break;
	}

	uiMSG().error("Both Factor and Units must be defined and "
		      "more than 1." );
    }
}


bool uiScenePropertyDlg::acceptOK( CallBacker* )
{
    if ( scene_ )
	scene_->savePropertySettings();

    if ( viewers_[curvwridx_] )
	viewers_[curvwridx_]->savePropertySettings();

    savestatus_ = saveButtonChecked();
    if ( !savestatus_ )
	return true;

    for ( int idx=0; idx<viewers_.size() && viewers_[idx]; idx++ )
    {
        mDynamicCastGet(visSurvey::Scene*, scene, const_cast <visBase::Scene*> 
			(viewers_[idx]->getScene()));
	updateScene( scene );
	const_cast<ui3DViewer*>(viewers_[idx])->setBackgroundColor(
		bgcolfld_->color() );
	const_cast<ui3DViewer*>(viewers_[idx])->setAxisAnnotColor(
		annotcolfld_->color() );
    }

    return true;
}

