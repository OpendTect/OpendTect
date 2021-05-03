/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2008
________________________________________________________________________

-*/


#include "uiimpbodycaldlg.h"

#include "bodyvolumecalc.h"
#include "embody.h"
#include "executor.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "veldesc.h"
#include "od_helpids.h"


uiImplBodyCalDlg::uiImplBodyCalDlg( uiParent* p, const EM::Body& eb )
    : uiDialog(p,Setup(tr("Calculate volume"),tr("Body volume estimation"),
                        mODHelpKey(mImplBodyCalDlgHelpID) ))
    , embody_(eb)
    , velfld_(0)
    , volfld_(0)
    , impbody_(0)
{
    setCtrlStyle( CloseOnly );

    if ( SI().zIsTime() )
    {
	velfld_ = new uiGenInput( this,
			 VelocityDesc::getVelVolumeLabel(),
			 FloatInpSpec( SI().depthsInFeet()?10000.0f:3000.0f) );
    }

    uiPushButton* calcbut =
	new uiPushButton( this, tr("Estimate volume"), true );
    calcbut->activated.notify( mCB(this,uiImplBodyCalDlg,calcCB) );
    if ( velfld_ )
	calcbut->attach( alignedBelow, velfld_ );

    volfld_ = new uiGenInput( this, uiStrings::sVolume() );
    volfld_->setElemSzPol( uiObject::WideMax );
    volfld_->setReadOnly( true );
    volfld_->attach( alignedBelow, calcbut );
}


uiImplBodyCalDlg::~uiImplBodyCalDlg()
{ delete impbody_; }


#define mErrRet(s) { uiMSG().error(s); return; }

static BufferString dispText( float m3, bool inft )
{
    const float bblconv = 6.2898108;
    const float ft3conv = 35.314667;

    if ( mIsUdf(m3) )
	return "";

    float dispval = m3;
    if ( inft ) dispval *= ft3conv;
    bool mega = false;
    if ( fabs(dispval) > 1e6 )
	{ mega = true; dispval /= 1e6; }

    BufferString txt;
    txt = dispval; txt += mega ? "M " : " ";
    txt += inft ? "ft^3" : "m^3";
    txt += " (";
    dispval *= bblconv;
    if ( inft ) dispval /= ft3conv;
    if ( dispval > 1e6 )
	{ mega = true; dispval /= 1e6; }
    txt += dispval; if ( mega ) txt += "M";
    txt += " bbl)";

    return txt;
}


void uiImplBodyCalDlg::calcCB( CallBacker* )
{
    if ( !impbody_ )
    {
	getImpBody();
    	if ( !impbody_ || !impbody_->arr_ )
    	    mErrRet(tr("Checking body failed"));
    }

    float vel = 1;
    if ( velfld_ )
    {
	vel = velfld_->getFValue();
	if ( mIsUdf(vel) || vel < 0.1 )
	    mErrRet(tr("Please provide the velocity"))
	if ( SI().depthsInFeet() )
	    vel *= mFromFeetFactorF;
    }

    uiTaskRunner taskrunner(this);
    BodyVolumeCalculator bc( impbody_->tkzs_, *impbody_->arr_,
	    impbody_->threshold_, vel );
    TaskRunner::execute( &taskrunner, bc );

    const float m3 = bc.getVolume();
    const BufferString txt = dispText( m3, SI().xyInFeet() );
    volfld_->setText( txt.buf() );
}


void uiImplBodyCalDlg::getImpBody()
{
    delete impbody_;

    uiTaskRunner taskrunner(this);
    impbody_ = embody_.createImplicitBody(&taskrunner,false);
}
