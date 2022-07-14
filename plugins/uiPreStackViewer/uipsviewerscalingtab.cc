/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          May 2008
________________________________________________________________________

-*/

#include "uipsviewerscalingtab.h"

#include "iopar.h"
#include "flatview.h"
#include "settings.h"
#include "uibutton.h"
#include "uiflatviewproptabs.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uipsviewermanager.h"
#include "visflatviewer.h"
#include "visprestackdisplay.h"


namespace PreStackView
{

uiViewer3DScalingTab::uiViewer3DScalingTab( uiParent* p,
	visSurvey::PreStackDisplay& psv, uiViewer3DMgr& mgr )
    : uiFlatViewDataDispPropTab( p, *psv.flatViewer(), uiStrings::sScaling(),
				 false)
    , applyall_( false )
    , savedefault_( false )
    , mgr_( mgr )
{
    applybut_ = uiButton::getStd( this, OD::Apply,
		   mCB(this,uiViewer3DScalingTab,applyButPushedCB), true );
    applybut_->attach( alignedBelow, lastcommonfld_ );

    mDynamicCastGet(visBase::FlatViewer*,vwr,&vwr_);
    if ( vwr )
	mAttachCB( vwr->dispParsChanged, uiViewer3DScalingTab::dispChgCB );
    putToScreen();
}


uiViewer3DScalingTab::~uiViewer3DScalingTab()
{
    detachAllNotifiers();
}


void uiViewer3DScalingTab::dispChgCB( CallBacker* )
{
    putToScreen();
}


BufferString uiViewer3DScalingTab::dataName() const
{
    ConstRefMan<FlatDataPack> dp = vwr_.getPack( false ).get();
    return BufferString( dp ? dp->name().buf() : "" );
}


void uiViewer3DScalingTab::putToScreen()
{
    putCommonToScreen();
}


FlatView::DataDispPars::Common& uiViewer3DScalingTab::commonPars()
{
    return ddpars_.vd_;
}


bool uiViewer3DScalingTab::acceptOK()
{
    if ( !apply() )
	return false;

    if ( useclipfld_->getIntValue() && saveAsDefault() )
	ddpars_.vd_.mappersetup_.range_ = vwr_.getDataRange(false);

    IOPar flatviewpar;
    ddpars_.fillPar( flatviewpar );
    if ( saveAsDefault() )
    {
	Settings& settings = Settings::fetch( uiViewer3DMgr::sSettings3DKey() );
	settings.mergeComp( flatviewpar, uiViewer3DMgr::sKeyFlatviewPars() );
	if ( !settings.write() )
	{
	    uiMSG().error(tr("Cannot write  default settings"));
	    return false;
	}
    }

    return true;
}


void uiViewer3DScalingTab::applyButPushedCB( CallBacker* cb )
{
    apply();
}


bool uiViewer3DScalingTab::apply()
{
    if ( !settingCheck() )
	return false;

    if ( !uiFlatViewDataDispPropTab::acceptOK() )
	return false;

    IOPar par;
    ddpars_.fillPar( par );

    vwr_.appearance().ddpars_.usePar( par );
    vwr_.handleChange( FlatView::Viewer::DisplayPars );

    if ( !applyall_ )
	return true;

    for ( int idx=0; idx<mgr_.get3DViewers().size(); idx++ )
    {
	visBase::FlatViewer* fvwr = mgr_.get3DViewers()[idx]->flatViewer();
	if ( fvwr==&vwr_)
	    continue;

	fvwr->appearance().ddpars_.usePar( par );
	fvwr->handleChange( FlatView::Viewer::DisplayPars );
    }

    return true;
}


bool uiViewer3DScalingTab::settingCheck()
{
    const int clip = useclipfld_->getIntValue();
    if ( !clip )
    {
	if ( mIsUdf(rgfld_->getFInterval().start) ||
	     mIsUdf(rgfld_->getFInterval().stop) )
	{
	    uiMSG().error( tr("Range is not set") );
	    return false;
	}
    }
    else if ( clip==1 )
    {
	const float symv =symclipratiofld_->getFValue();
	if ( mIsUdf(symv) )
	{
	    uiMSG().error( tr("Clipping rate is not set") );
	    return false;
	}

	if ( !mIsUdf(symv) && (symv>100 || symv<0) )
	{
	    uiMSG().error( tr("Clip percentage should between 0 and 100") );
	    return false;
	}

	if ( usemidvalfld_->getBoolValue() &&
	     mIsUdf(symmidvalfld_->getFValue()) )
	{
	    uiMSG().error( tr("Midvalue is not set") );
	    return false;
	}
    }
    else
    {
	const Interval<float> asymv = assymclipratiofld_->getFInterval();
	if ( !mIsUdf(asymv.start) && !mIsUdf(asymv.stop) )
	{
	    if ( asymv.start>100 || asymv.start<0 ||
		 asymv.stop>100 || asymv.stop<0 )
	    {
		uiMSG().error( tr("Clip percentage should between 0 and 100") );
		return false;
	    }

	    if ( asymv.start+asymv.stop>100 )
	    {
		uiMSG().error( tr("Clip percentage sum is between 0 and 100") );
		return false;
	    }
	}

	if ( mIsUdf(asymv.start) || mIsUdf(asymv.stop) )
	{
	    uiMSG().error( tr("Clipping rate is not set") );
	    return false;
	}
    }

    return true;
}

} // namespace PreStackView
