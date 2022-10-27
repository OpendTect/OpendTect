/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiimpbodycaldlg.h"

#include "bodyvolumecalc.h"
#include "embody.h"
#include "od_helpids.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "veldesc.h"

#include "uibutton.h"
#include "uiconstvel.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "uiunitsel.h"


uiImplBodyCalDlg::uiImplBodyCalDlg( uiParent* p, const EM::Body& eb )
    : uiDialog(p,Setup(tr("Calculate Geobody Volume"),mNoDlgTitle,
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
			 FloatInpSpec(Vel::getGUIDefaultVelocity()) );
    }

    auto* calcbut = new uiPushButton( this, uiStrings::sCalculate(), true );
    calcbut->setIcon( "downarrow" );
    mAttachCB( calcbut->activated, uiImplBodyCalDlg::calcCB );
    if ( velfld_ )
	calcbut->attach( alignedBelow, velfld_ );

    volfld_ = new uiGenInput( this, uiStrings::sVolume() );
    volfld_->setReadOnly( true );
    volfld_->attach( alignedBelow, calcbut );

    unitfld_ = new uiUnitSel( this, Mnemonic::Vol );
    unitfld_->setUnit( UoMR().get(SI().depthsInFeet() ? "ft3" : "m3") );
    mAttachCB( unitfld_->selChange, uiImplBodyCalDlg::unitChgCB );
    unitfld_->attach( rightOf, volfld_ );
}


uiImplBodyCalDlg::~uiImplBodyCalDlg()
{
    detachAllNotifiers();
    delete impbody_;
}


#define mErrRet(s) { uiMSG().error(s); return; }

void uiImplBodyCalDlg::calcCB( CallBacker* )
{
    if ( !impbody_ )
    {
	getImpBody();
	if ( !impbody_ || !impbody_->arr_ )
	    mErrRet(tr("Checking geobody failed"));
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

    volumeinm3_ = mUdf(float);
    uiTaskRunner taskrunner(this);
    BodyVolumeCalculator bc( impbody_->tkzs_, *impbody_->arr_,
			     impbody_->threshold_, vel );
    if ( !TaskRunner::execute(&taskrunner,bc) )
	return;

    volumeinm3_ = bc.getVolume();
    unitChgCB( nullptr );
}


void uiImplBodyCalDlg::unitChgCB( CallBacker* )
{
    BufferString txt;
    const UnitOfMeasure* uom = unitfld_->getUnit();
    if ( uom && !mIsUdf(volumeinm3_) )
    {
	const float newval = uom->getUserValueFromSI( volumeinm3_ );
	txt.set( newval, 4 );
    }

    volfld_->setText( txt );
}


void uiImplBodyCalDlg::getImpBody()
{
    delete impbody_;

    uiTaskRunner taskrunner(this);
    impbody_ = embody_.createImplicitBody(&taskrunner,false);
}
