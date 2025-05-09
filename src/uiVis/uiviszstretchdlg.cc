/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiviszstretchdlg.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "uipixmap.h"
#include "uislider.h"
#include "uivispartserv.h"

#include "iopar.h"
#include "mousecursor.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "zdomain.h"


uiZStretchDlg::uiZStretchDlg( uiParent* p, uiVisPartServer* vispartserv )
    : uiDialog(p,Setup(tr("Z Scaling"),tr("Set Z scaling factor"),
		       mODHelpKey(mZScaleDlgHelpID))
		    .canceltext(uiString::emptyString()))
    , vispartserv_(vispartserv)
{
    vispartserv->getSceneIds( sceneids_ );
    if ( sceneids_.isEmpty() )
    {
	new uiLabel( this, tr("No scenes available") );
	return;
    }

    RefMan<visSurvey::Scene> scene = getSceneByIdx( 0 );
    if ( !scene )
	return;

    const float initslval =
			scene->getFixedZStretch()*scene->getTempZStretch();
    zstretches_ += initslval;

    if ( sceneids_.size()>1 )
    {
	uiStringSet scenenms;
	scenenms.add( uiStrings::sAll() );
	for ( int idx=0; idx<sceneids_.size(); idx++ )
	{
	    ConstRefMan<visSurvey::Scene> thescene = getSceneByIdx( idx );
	    scenenms.add( thescene->uiName() );
	    if ( idx>0 )
	    {
		const float inival =
		    thescene->getFixedZStretch()*thescene->getTempZStretch();
		zstretches_ += inival;
	    }
	}

	scenefld_ = new uiLabeledComboBox( this, scenenms,
					   tr("Apply scaling to") );
	scenefld_->box()->setCurrentItem( 1 );
	mAttachCB( scenefld_->box()->selectionChanged, uiZStretchDlg::sceneSel);
    }

    sliderfld_ = new uiSlider( this, uiSlider::Setup(sZStretch())
				     .withedit(true).nrdec(4).logscale(true),
					"Z stretch slider" );
    mAttachCB( sliderfld_->valueChanged,uiZStretchDlg::sliderMove );
    if ( scenefld_ )
	sliderfld_->attach( alignedBelow, scenefld_ );

    mAttachCB( preFinalize(), uiZStretchDlg::doFinalize );
    initzstretches_ = zstretches_;
}


uiZStretchDlg::~uiZStretchDlg()
{
    detachAllNotifiers();
}


void uiZStretchDlg::doFinalize( CallBacker* )
{
    sceneSel( 0 );

    if ( !vwallcb.willCall() && !homecb.willCall() )
	return;

    auto* grp = new uiGroup( this, "icons" );
    if ( vwallcb.willCall() )
    {
	uiPixmap vwallpm( "view_all" );
	vwallbut_ = new uiPushButton( grp, tr("Fit to scene"), vwallpm, true );
	mAttachCB( vwallbut_->activated, uiZStretchDlg::butPush );
    }
    if ( homecb.willCall() )
    {
	uiPixmap homepm( "home" );
	auto* homebut = new uiPushButton( grp, tr("To Home"), homepm, true );
	mAttachCB( homebut->activated, uiZStretchDlg::butPush );
	if ( vwallbut_ )
	    homebut->attach( rightOf, vwallbut_ );
    }

    grp->attach( centeredBelow, sliderfld_ );
    savefld_ = new uiCheckBox( this, uiStrings::sSaveAsDefault() );
    savefld_->setChecked( true );
    savefld_->attach( alignedBelow, grp );
}


void uiZStretchDlg::sceneSel( CallBacker* )
{
    int sceneidx = scenefld_ ? scenefld_->box()->currentItem()-1 : 0;
    if ( sceneidx<0 )
	sceneidx = 0;

    updateSliderValues( sceneidx );
}


void uiZStretchDlg::updateSliderValues( int sceneidx )
{
    if ( sceneidx>=sceneids_.size() )
	return;

    uiString label = sZStretch();
    float initslval = 1.0f;
    float uifactor = 1.0f;
    int nrdec = 0;
    if ( !sceneids_.isEmpty() )
    {
	if ( sceneidx < 0 )
	    sceneidx = 0;

	ConstRefMan<visSurvey::Scene> scene = getSceneByIdx( sceneidx );
	if ( !scene )
	    return;

	initslval = scene->getFixedZStretch()*scene->getTempZStretch();
	uifactor = scene->getApparentVelocity( initslval )/initslval;
	zstretches_[sceneidx] = initslval;

	if ( scene->zDomainInfo().def_.isTime() )
	    label = tr( "Apparent velocity %1")
		    .arg( UnitOfMeasure::surveyDefVelUnitAnnot(true,true) );

	if ( !scene->zDomainInfo().def_.isTime() )
	    nrdec = 2;
    }

    sliderfld_->label()->setText( label );

    sliderfld_->setMinValue( 0.04f*initslval*uifactor );
    sliderfld_->setMaxValue( 25*initslval*uifactor );
    sliderfld_->setValue( initslval*uifactor );
    sliderfld_->setNrDecimalsEditFld( nrdec );
}


bool uiZStretchDlg::acceptOK( CallBacker* )
{
    if ( !sliderfld_ )
	return true;

    int sceneidx = scenefld_ ? scenefld_->box()->currentItem()-1 : 0;
    if ( sceneidx<0 )
	sceneidx = 0;

    const bool stretchall = scenefld_ && scenefld_->box()->currentItem()==0;
    if ( stretchall )
	setOneZStretchToAllScenes( zstretches_[sceneidx], true );
    else
	setZStretchesToScenes( zstretches_, true );

    SI().getPars().removeWithKey("Z Scale"); //Old setting
    SI().savePars();

    return true;
}



void uiZStretchDlg::setOneZStretchToAllScenes( float zstretch, bool permanent )
{
    for ( int idx=0; idx<sceneids_.size(); idx++ )
    {
	RefMan<visSurvey::Scene> scene = getSceneByIdx( idx );
	if ( scene )
	    setZStretch( scene.ptr(), zstretch, permanent);
    }
}


void uiZStretchDlg::setZStretchesToScenes( TypeSet<float>& zstretches,
					   bool permanent )
{
    if ( zstretches.size() != sceneids_.size() )
	return;

    for ( int idx=0; idx<zstretches.size(); idx++ )
    {
	RefMan<visSurvey::Scene> scene = getSceneByIdx( idx );
	if ( scene )
	    setZStretch( scene.ptr(), zstretches[idx], permanent);
    }
}


void uiZStretchDlg::setZStretch( visSurvey::Scene* scene, float zstretch,
				 bool permanent )
{
    if ( !scene )
	return;

    if ( permanent )
    {
	MouseCursorChanger cursorchanger( MouseCursor::Busy );
	scene->setTempZStretch( 1.f );
	scene->setFixedZStretch( zstretch );
    }
    else
    {
	scene->setTempZStretch( zstretch/scene->getFixedZStretch() );
    }

    if ( savefld_ && savefld_->isChecked() )
	SI().getPars().set(
	    IOPar::compKey("Z Scale",scene->zDomainInfo().key()), zstretch );

    const int idx = sceneids_.indexOf( scene->getID() );
    if ( zstretches_.validIdx(idx) )
	zstretches_[idx] = zstretch;
}


bool uiZStretchDlg::rejectOK( CallBacker* )
{
    setZStretchesToScenes( initzstretches_, false );
    return true;
}


void uiZStretchDlg::sliderMove( CallBacker* )
{
    const bool stretchall = scenefld_ && scenefld_->box()->currentItem()==0;
    const float uifactor = getSelectedSceneUiFactor();
    const float slval = sliderfld_->getFValue()/uifactor;

    RefMan<visSurvey::Scene> scene = getSelectedScene();
    const int idx = scene ? sceneids_.indexOf( scene->getID() ) : -1;
    if ( scene && zstretches_.validIdx(idx) )
	zstretches_[idx] = slval;

    if ( stretchall )
	setOneZStretchToAllScenes( slval, true );
    else
	setZStretch( scene.ptr(), slval, false);

}


RefMan<visSurvey::Scene> uiZStretchDlg::getSceneByIdx( int idx ) const
{
    if ( !vispartserv_ || !sceneids_.validIdx(idx) )
	return nullptr;

    return vispartserv_->getScene( sceneids_[idx] );
}


RefMan<visSurvey::Scene> uiZStretchDlg::getSelectedScene() const
{
    int sceneidx = scenefld_ ? scenefld_->box()->currentItem()-1 : 0;
    if ( sceneidx<0 )
	sceneidx = 0;

    return getSceneByIdx( sceneidx );
}


float uiZStretchDlg::getSelectedSceneZStretch() const
{
    ConstRefMan<visSurvey::Scene> scene = getSelectedScene();
    if ( !scene )
	return -1.0f;

    return scene->getFixedZStretch()*scene->getTempZStretch();
}


float uiZStretchDlg::getSelectedSceneUiFactor() const
{
    ConstRefMan<visSurvey::Scene> scene = getSelectedScene();
    if ( !scene )
	return -1.0f;

    const float zstretch = getSelectedSceneZStretch();
    return scene->getApparentVelocity(zstretch)/zstretch;
}


void uiZStretchDlg::butPush( CallBacker* b )
{
    if ( b == vwallbut_ )
	vwallcb.doCall( this );
    else
	homecb.doCall( this );
}
