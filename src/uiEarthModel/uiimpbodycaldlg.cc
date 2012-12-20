/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2008
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiimpbodycaldlg.h"

#include "bodyvolumecalc.h"
#include "embody.h"
#include "executor.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uitaskrunner.h"


uiImplBodyCalDlg::uiImplBodyCalDlg( uiParent* p, const EM::Body& eb )
    : uiDialog(p,Setup("Calculate volume","Body volume estimation",mNoHelpID))
    , embody_(eb)  
    , velfld_(0)
    , volfld_(0)
    , impbody_(0)  
{
    setCtrlStyle( LeaveOnly );
    
    if ( SI().zIsTime() )
    {
	const bool zinft = SI().depthsInFeetByDefault();
	const char* txt = zinft ? "Velocity (ft/s)" : "Velocity (m/s)";
	velfld_ = new uiGenInput( this, txt, FloatInpSpec(
					    mCast(float,zinft?10000:3000)) );
    }
    
    volfld_ = new uiGenInput( this, "Volume" );
    volfld_->setReadOnly( true );
    if ( velfld_ )
	volfld_->attach( alignedBelow, velfld_ );
    
    uiPushButton* calcbut = new uiPushButton( this, "&Estimate", true );
    calcbut->activated.notify( mCB(this,uiImplBodyCalDlg,calcCB) );
    calcbut->attach( rightTo, volfld_ );
}


uiImplBodyCalDlg::~uiImplBodyCalDlg()
{ delete impbody_; }


#define mErrRet(s) { uiMSG().error(s); return; }

void uiImplBodyCalDlg::calcCB( CallBacker* )
{
    if ( !impbody_ )
    {
	getImpBody();
    	if ( !impbody_ || !impbody_->arr_ )
    	    mErrRet("Checking body failed");
    }

    float vel = 1;
    if ( velfld_ )
    {
	vel = velfld_->getfValue();
	if ( mIsUdf(vel) || vel < 0.1 )
	    mErrRet("Please provide the velocity")
	if ( SI().depthsInFeetByDefault() )
	    vel *= mFromFeetFactorF;
    }

    uiTaskRunner tr(this);
    BodyVolumeCalculator bc( impbody_->cs_, *impbody_->arr_, 
	    impbody_->threshold_, vel );
    TaskRunner::execute( &tr, bc );

    BufferString txt;
    txt += bc.getVolume();
    txt += "m^3";
    volfld_->setText( txt.buf() );
}


void uiImplBodyCalDlg::getImpBody()
{
    delete impbody_;

    uiTaskRunner tr(this);
    impbody_ = embody_.createImplicitBody(&tr,false);
}
