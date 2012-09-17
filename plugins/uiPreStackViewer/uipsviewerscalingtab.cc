/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          May 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uipsviewerscalingtab.cc,v 1.3 2011/02/10 06:29:54 cvssatyaki Exp $";

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
#include "visprestackviewer.h"


namespace PreStackView
{
uiViewer3DScalingTab::uiViewer3DScalingTab( uiParent* p, 
	PreStackView::Viewer3D& psv, uiViewer3DMgr& mgr )
    : uiFlatViewDataDispPropTab( p, *psv.flatViewer(), "Scaling", false)
    , applyall_( false )
    , savedefault_( false )
    , mgr_( mgr )					    
{
    applybut_ = new uiPushButton( this, "Apply", true );
    applybut_->activated.notify( 
	    mCB(this,uiViewer3DScalingTab,applyButPushedCB) );
    applybut_->attach( alignedBelow, symmidvalfld_ );
    putToScreen();
}


const char* uiViewer3DScalingTab::dataName() const
{
    return vwr_.pack(false) ? vwr_.pack(false)->name().buf() : "";
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
    if ( !applyButPushedCB(0) )
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
	    uiMSG().error("Cannot write  default settings");
	    return false;
	}
    }

    return true;
}


bool uiViewer3DScalingTab::applyButPushedCB( CallBacker* cb )
{
    if ( !settingCheck() )
	return false;

    if ( !uiFlatViewDataDispPropTab::acceptOK() )
	return false;

    IOPar par;
    ddpars_.fillPar( par );

    vwr_.appearance().ddpars_.usePar( par );
    vwr_.handleChange( FlatView::Viewer::VDPars );

    if ( !applyall_ )
	return true;
    
    for ( int idx=0; idx<mgr_.get3DViewers().size(); idx++ )
    {
	visBase::FlatViewer* fvwr = mgr_.get3DViewers()[idx]->flatViewer();
	if ( fvwr==&vwr_)
	    continue;

	fvwr->appearance().ddpars_.usePar( par );
	fvwr->handleChange( FlatView::Viewer::VDPars );
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
	    uiMSG().error( "Range is not set" );
	    return false;
	}
    }
    else if ( clip==1 )
    {
	const float symv =symclipratiofld_->getfValue();
	if ( mIsUdf(symv) )
	{
	    uiMSG().error( "Clipping rate is not set" );
	    return false;
	}

	if ( !mIsUdf(symv) && (symv>100 || symv<0) )
	{
	    uiMSG().error( "Clip percentage should between 0 and 100" );
    	    return false;
	}

	if ( usemidvalfld_->getBoolValue() && 
	     mIsUdf(symmidvalfld_->getfValue()) )
	{
	    uiMSG().error( "Midvalue is not set" );
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
    		uiMSG().error( "Clip percentage should between 0 and 100" );
    		return false;
	    }

	    if ( asymv.start+asymv.stop>100 )
	    {
		uiMSG().error( "Clip percentage sum is between 0 and 100" );
	      	return false;
	    }
	}

	if ( mIsUdf(asymv.start) || mIsUdf(asymv.stop) )
	{
	    uiMSG().error( "Clipping rate is not set" );
	    return false;
	}
    }

    return true;
}



}; //namespace

