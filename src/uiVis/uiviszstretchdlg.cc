/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
________________________________________________________________________

-*/

#include "uiviszstretchdlg.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "uipixmap.h"
#include "uislider.h"
#include "uimsg.h"

#include "iopar.h"
#include "od_helpids.h"
#include "survinfo.h"
#include "veldesc.h"

#include "visdataman.h"
#include "vistransmgr.h"
#include "vissurvscene.h"

#include <typeinfo>


uiZStretchDlg::uiZStretchDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Z Scaling"),tr("Set Z scaling factor"),
				 mODHelpKey(mZScaleDlgHelpID)))
    , vwallbut_(0)
    , scenefld_(0)
    , sliderfld_(0)
    , savefld_(0)
{
    visBase::DM().getIDs( typeid(visSurvey::Scene), sceneids_ );
    if ( sceneids_.size() == 0 )
    {
	new uiLabel( this, tr("No scenes available") );
	return;
    }

    mDynamicCastGet(visSurvey::Scene*,scene,
	    visBase::DM().getObject(sceneids_[0]) );
    if ( !scene ) return;

    const float initslval =
	scene->getFixedZStretch()*scene->getTempZStretch();
    zstretches_ +=  initslval;

    if ( sceneids_.size()>1 )
    {
	uiStringSet scenenms;
	scenenms.add( uiStrings::sAll() );
	for ( int idx=0; idx<sceneids_.size(); idx++ )
	{
	    mDynamicCastGet(visSurvey::Scene*,thescene,
			    visBase::DM().getObject(sceneids_[idx]))
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
				     .withedit(true).nrdec(3).logscale(true),
					"Z stretch slider" );
    mAttachCB( sliderfld_->valueChanged,uiZStretchDlg::sliderMove );
    if ( scenefld_ )
	sliderfld_->attach( alignedBelow, scenefld_ );

    mAttachCB( preFinalise(), uiZStretchDlg::doFinalise );

    initzstretches_ = zstretches_;
}


uiZStretchDlg::~uiZStretchDlg()
{
    detachAllNotifiers();
}


void uiZStretchDlg::doFinalise( CallBacker* )
{
    sceneSel( 0 );

    if ( !vwallcb.willCall() && !homecb.willCall() )
	return;

    uiGroup* grp = new uiGroup( this, "icons" );
    if ( vwallcb.willCall() )
    {
	uiPixmap vwallpm( "view_all" );
	vwallbut_ = new uiPushButton( grp, tr("Fit to scene"), vwallpm, true );
	mAttachCB( vwallbut_->activated, uiZStretchDlg::butPush );
    }
    if ( homecb.willCall() )
    {
	uiPixmap homepm( "home" );
	uiButton* homebut =
		new uiPushButton( grp, tr("To Home"), homepm, true );
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


void uiZStretchDlg::updateSliderValues(  int sceneidx )
{
    if ( sceneidx>=sceneids_.size() )
	return;

    uiString label = sZStretch();
    float initslval = 1.0f;
    float uifactor = 1.0f;
    if ( sceneids_.size() )
    {
	if ( sceneidx < 0 ) sceneidx = 0;
	mDynamicCastGet(visSurvey::Scene*, scene,
			visBase::DM().getObject(sceneids_[sceneidx]));
	if ( !scene ) return;

	initslval = scene->getFixedZStretch()*scene->getTempZStretch();
	uifactor = scene->getApparentVelocity( initslval )/initslval;
	zstretches_[sceneidx] = initslval;

	if ( scene->zDomainInfo().def_.isTime() )
	{
	    label = tr("Apparent velocity")
			.withUnit( VelocityDesc::getVelUnit(true) );
	}
    }

    sliderfld_->label()->setText( label );

    sliderfld_->setMinValue( 0.04f*initslval*uifactor );
    sliderfld_->setMaxValue( 25*initslval*uifactor );
    sliderfld_->setValue( initslval*uifactor );
}


bool uiZStretchDlg::acceptOK()
{
    if ( !sliderfld_ )
	return true;

    int sceneidx = scenefld_ ? scenefld_->box()->currentItem()-1 : 0;
    if ( sceneidx<0 ) sceneidx=0;

    const bool stretchall = scenefld_ && scenefld_->box()->currentItem()==0;
    if ( stretchall )
	setOneZStretchToAllScenes( zstretches_[sceneidx], true );
    else
	setZStretchesToScenes( zstretches_, true );

    SI().removeKeyFromDefaultPars( "Z Scale", true ); //Old setting
    return true;
}



void uiZStretchDlg::setOneZStretchToAllScenes( float zstretch, bool permanent )
{
    for ( int idx=0; idx<sceneids_.size(); idx++ )
    {
	mDynamicCastGet( visSurvey::Scene*, scene,
	    visBase::DM().getObject(sceneids_[idx]) );
	if ( !scene ) continue;
	setZStretch( scene, zstretch, permanent );
    }
}


void uiZStretchDlg::setZStretchesToScenes( TypeSet<float>& zstretches,
					   bool permanent )
{
    if ( zstretches.size() != sceneids_.size() )
	return;

    for ( int idx = 0; idx<zstretches.size(); idx++ )
    {
	mDynamicCastGet( visSurvey::Scene*,scene,
	    visBase::DM().getObject(sceneids_[idx]) );
	if ( !scene ) continue;
	setZStretch( scene, zstretches[idx], permanent );
    }
}


void uiZStretchDlg::setZStretch( visSurvey::Scene* scene, float zstretch,
				 bool permanent )
{
    if ( !scene )
	return;

    uiUserShowWait usw( this, uiStrings::sUpdatingDisplay() );
    if ( permanent )
    {
	scene->setTempZStretch( 1.f );
	scene->setFixedZStretch( zstretch );
    }
    else
    {
	scene->setTempZStretch( zstretch/scene->getFixedZStretch() );
    }

    if ( savefld_ && savefld_->isChecked() )
	SI().setDefaultPar(
		IOPar::compKey("Z Scale",scene->zDomainInfo().key()),
		toString(zstretch), true );

    const int id = sceneids_.indexOf( scene->id() );
    zstretches_[id] = zstretch;
}


bool uiZStretchDlg::rejectOK()
{
    setZStretchesToScenes( initzstretches_, false );
    return true;
}


void uiZStretchDlg::sliderMove( CallBacker* )
{
    const bool stretchall = scenefld_ && scenefld_->box()->currentItem()==0;
    const float uifactor = getSelectedSceneUiFactor();
    const float slval = sliderfld_->getValue()/uifactor;

    visSurvey::Scene* scene =  getSelectedScene();
    const int id = sceneids_.indexOf( scene->id() );
    if ( scene )
	zstretches_[id] = slval;

    if ( stretchall )
	setOneZStretchToAllScenes( slval, true );
    else
	setZStretch( scene, slval, false );

}


visSurvey::Scene* uiZStretchDlg::getSelectedScene() const
{
    int sceneidx = scenefld_ ? scenefld_->box()->currentItem()-1 : 0;
    if ( sceneidx<0 ) sceneidx = 0;
    mDynamicCastGet (visSurvey::Scene*, scene,
	visBase::DM().getObject(sceneids_[sceneidx]) );
    return scene;
}


float uiZStretchDlg::getSelectedSceneZStretch() const
{
    const visSurvey::Scene* scene = getSelectedScene();
    if ( !scene )
	return -1.0f;
    return scene->getFixedZStretch()*scene->getTempZStretch();
}


float uiZStretchDlg::getSelectedSceneUiFactor() const
{
    const visSurvey::Scene* scene = getSelectedScene();
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
