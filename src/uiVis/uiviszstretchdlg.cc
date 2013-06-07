/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uiviszstretchdlg.h"

#include "iopar.h"
#include "pixmap.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "uislider.h"
#include "visdataman.h"
#include "vistransmgr.h"
#include "vissurvscene.h"


uiZStretchDlg::uiZStretchDlg( uiParent* p )
    : uiDialog(p,
	       uiDialog::Setup("Z Scaling","Set scaling factor","50.0.7")
	       .canceltext(""))
    , valchgd(false)
    , vwallbut(0)
    , scenefld(0)
    , sliderfld(0)
{
    visBase::DM().getIds( typeid(visSurvey::Scene), sceneids );
    if ( sceneids.size() == 0 )
    {
	uiLabel* lbl = new uiLabel( this, "No scenes available" );
	return;
    }

    if ( sceneids.size() > 1 )
    {
	BufferStringSet scenenms;
	scenenms.add( "All" );
	for ( int idx=0; idx<sceneids.size(); idx++ )
	{
	    mDynamicCastGet(visSurvey::Scene*,scene,
		    	    visBase::DM().getObject(sceneids[idx]))
	    scenenms.add( scene->name() );
	}

	scenefld = new uiLabeledComboBox( this, scenenms, "Apply scaling to" );
	scenefld->box()->setCurrentItem( 1 );
	scenefld->box()->selectionChanged.notify( 
					mCB(this,uiZStretchDlg,sceneSel) );
    }

    sliderfld = new uiSliderExtra( this, uiSliderExtra::Setup("Z stretch")
				     .withedit(true).nrdec(3).logscale(true),
	   				"Z stretch slider" );
    sliderfld->sldr()->valueChanged.notify( mCB(this,uiZStretchDlg,sliderMove) );
    if ( scenefld )
	sliderfld->attach( alignedBelow, scenefld );

    preFinalise().notify( mCB(this,uiZStretchDlg,doFinalise) );
}


void uiZStretchDlg::doFinalise( CallBacker* )
{
    updateSliderValues();

    if ( !vwallcb.willCall() && !homecb.willCall() )
	return;

    uiGroup* grp = new uiGroup( this, "icons" );
    if ( vwallcb.willCall() )
    {
	ioPixmap vwallpm( "view_all.png" );
	vwallbut = new uiPushButton( grp, "&Fit to scene", vwallpm, true );
	vwallbut->activated.notify( mCB(this,uiZStretchDlg,butPush) );
    }
    if ( homecb.willCall() )
    {
	ioPixmap homepm( "home.png" );
	uiButton* homebut = new uiPushButton( grp, "To &Home", homepm, true );
	homebut->activated.notify( mCB(this,uiZStretchDlg,butPush) );
	if ( vwallbut )
	    homebut->attach( rightOf, vwallbut );
    }

    grp->attach( centeredBelow, sliderfld );
    savefld = new uiCheckBox( this, "Save as default" );
    savefld->attach( alignedBelow, grp );
}


void uiZStretchDlg::sceneSel( CallBacker* )
{
    updateSliderValues();
}


void uiZStretchDlg::updateSliderValues()
{
    initslval = getCurrentZStretch();
    sliderfld->sldr()->setMinValue( 0.04*initslval );
    sliderfld->sldr()->setMaxValue( 25*initslval );
    sliderfld->sldr()->setValue( initslval );
}


float uiZStretchDlg::getCurrentZStretch() const
{
    if ( sceneids.size() == 0 )
	return visSurvey::STM().defZStretch();

    int sceneidx = scenefld ? scenefld->box()->currentItem()-1 : 0;
    if ( sceneidx < 0 ) sceneidx = 0;
    mDynamicCastGet(visSurvey::Scene*,scene,
		    visBase::DM().getObject(sceneids[sceneidx]))
    return scene->getZStretch();
}


void uiZStretchDlg::setZStretch( float zstretch )
{
    const bool stretchall = scenefld && scenefld->box()->currentItem()==0;
    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	bool dostretch = !scenefld || stretchall ||
		       idx == scenefld->box()->currentItem()-1;
	if ( !dostretch ) continue;

	mDynamicCastGet(visSurvey::Scene*,scene,
			visBase::DM().getObject(sceneids[idx]))
	scene->setZStretch( zstretch );
    }
}


bool uiZStretchDlg::acceptOK( CallBacker* )
{
    if ( !sliderfld )
	return true;

    sliderfld->processInput();
    const float slval = sliderfld->sldr()->getValue();
    setZStretch( slval );

    if ( savefld->isChecked() )
    {
	SI().getPars().set( visSurvey::STM().zStretchStr(), slval );
	SI().savePars();
    }

    if ( slval != initslval )
	valchgd = true;
    return true;
}


void uiZStretchDlg::sliderMove( CallBacker* )
{
    const float slval = sliderfld->sldr()->getValue();
    setZStretch( slval );
}


void uiZStretchDlg::butPush( CallBacker* b )
{
    if ( b == vwallbut )
	vwallcb.doCall( this );
    else
	homecb.doCall( this );
}
