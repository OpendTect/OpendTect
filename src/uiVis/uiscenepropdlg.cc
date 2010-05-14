/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          February 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiscenepropdlg.cc,v 1.14 2010-05-14 13:30:33 cvskarthika Exp $";

#include "uiscenepropdlg.h"

#include "vissurvscene.h"
#include "visscenecoltab.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uislider.h"
#include "uisoviewer.h"

bool uiScenePropertyDlg::savestatus = true;


uiScenePropertyDlg::uiScenePropertyDlg( uiParent* p, 
		const ObjectSet<uiSoViewer>& viewers, int curvwridx )
    : uiDialog( p, uiDialog::Setup("Scene","Set scene_ options","50.0.5") )
    , hadsurveybox_( true )
    , hadannot_( true )
    , hadannotscale_( true )
    , oldbgcolor_( Color::Black() )
    , annotcolor_( Color::White() )
    , oldmarkersize_( 5 )
    , oldmarkercolor_( Color::White() )
    , markersizefld_( 0 )
    , annotfld_( 0 )
    , annotscalefld_( 0 )
    , survboxfld_( 0 )
    , bgcolfld_( 0 )
    , viewers_( viewers )
    , curvwridx_( curvwridx )
    , scene_( NULL )
{
    enableSaveButton( "Apply to all scenes");
    setSaveButtonChecked( savestatus );

    if ( viewers_[curvwridx_] )
    {
	oldbgcolor_ = viewers_[curvwridx_]->getBackgroundColor();

        mDynamicCast(visSurvey::Scene*, scene_, const_cast <visBase::Scene*> 
			(viewers_[curvwridx_]->getScene()));
	if ( scene_ )
	{
	    hadsurveybox_ = scene_->isAnnotShown();
	    hadannot_ = scene_->isAnnotTextShown();
	    hadannotscale_ = scene_->isAnnotScaleShown();
	    annotcolor_ = scene_->getAnnotColor();
	    oldmarkersize_ = scene_->getMarkerSize();
	    oldmarkercolor_ = scene_->getMarkerColor();
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

    bgcolfld_ = new uiColorInput( this, uiColorInput::Setup( oldbgcolor_)
					.lbltxt("Background color") );
    bgcolfld_->attach( alignedBelow, annotscalefld_ );
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
}


void uiScenePropertyDlg::updateCB( CallBacker* cb )
{
    if ( scene_ )
        updateScene( scene_ );

    uiSoViewer* vwr = const_cast<uiSoViewer*> (viewers_[curvwridx_]);
    if ( vwr )
    {
	vwr->setBackgroundColor( bgcolfld_->color() );
	vwr->setAxisAnnotColor( annotcolfld_->color() );
    }

    annotfld_->setSensitive( survboxfld_->isChecked() );
    annotscalefld_->setSensitive( survboxfld_->isChecked() );
}


void uiScenePropertyDlg::updateScene( visSurvey::Scene* scene_ )
{
    if ( !scene_ )
	return;

    scene_->showAnnot( survboxfld_->isChecked() );
    scene_->showAnnotScale( annotscalefld_->isChecked() );
    scene_->showAnnotText( annotfld_->isChecked() );
    scene_->setMarkerSize( markersizefld_->sldr()->getValue() );
    scene_->setMarkerColor( markercolfld_->color() );
    scene_->setAnnotColor( annotcolfld_->color() );
    scene_->getSceneColTab()->setLegendColor( annotcolfld_->color() );
}


bool uiScenePropertyDlg::rejectOK( CallBacker* )
{
    if ( scene_ )
    {
        scene_->showAnnot( hadsurveybox_ );
	scene_->showAnnotScale( hadannot_ );
	scene_->showAnnotText( hadannotscale_ );
	scene_->setMarkerSize( oldmarkersize_ );
	scene_->setMarkerColor( oldmarkercolor_ );
        scene_->setAnnotColor( annotcolor_ );
    }
    
    if ( viewers_[curvwridx_] )
	const_cast<uiSoViewer*>(viewers_[curvwridx_])->setBackgroundColor( 
		    oldbgcolor_ );
    return true;
}


bool uiScenePropertyDlg::acceptOK( CallBacker* )
{
    savestatus = saveButtonChecked();
    if ( !savestatus )
	return true;

    for ( int idx=0; idx<viewers_.size() && viewers_[idx]; idx++ )
    {
        mDynamicCastGet(visSurvey::Scene*, scene, const_cast <visBase::Scene*> 
			(viewers_[idx]->getScene()));
	updateScene( scene );
	const_cast<uiSoViewer*>(viewers_[idx])->setBackgroundColor(
		bgcolfld_->color() );
	const_cast<uiSoViewer*>(viewers_[idx])->setAxisAnnotColor(
		annotcolfld_->color() );
    }

    return true;
}

