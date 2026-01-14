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
    : uiDialog(p,Setup(tr("Calculate Geobody Volume"),
		       mODHelpKey(mImplBodyCalDlgHelpID)))
    , calcPushed(this)
    , embody_(eb)
{
    setCtrlStyle( CloseOnly );

    topgrp_ = new uiGroup( this, "Top Group" );
    if ( SI().zIsTime() )
    {
	velfld_ = new uiGenInput( topgrp_,
		    tr("%1 %2")
		    .arg(VelocityDesc::getVelVolumeLabel())
		    .arg(UnitOfMeasure::surveyDefVelUnitAnnot(true,true)),
			 FloatInpSpec(Vel::getGUIDefaultVelocity()) );
	velfld_->setStretch( 0, 0 );
    }

    auto* calcvolbut = new uiPushButton( topgrp_, tr("Calculate Volume"),
					 "math", true );
    calcvolbut->setStretch( 0, 0 );
    mAttachCB( calcvolbut->activated, uiImplBodyCalDlg::calcCB );
    if ( velfld_ )
	calcvolbut->attach( alignedBelow, velfld_ );

    grossvolfld_ = new uiGenInput( topgrp_, tr("Gross Volume") );
    grossvolfld_->setReadOnly( true );
    grossvolfld_->attach( alignedBelow, calcvolbut );
    grossvolfld_->setStretch( 0, 0 );
    topgrp_->setHAlignObj( grossvolfld_ );

    uiUnitSel::Setup unitsetup( Mnemonic::Vol );
    unitsetup.variableszpol( true );
    unitfld_ = new uiUnitSel( topgrp_, unitsetup );
    unitfld_->setUnit( UoMR().get(SI().depthsInFeet() ? "ft3" : "m3") );
    mAttachCB( unitfld_->selChange, uiImplBodyCalDlg::unitChgCB );
    unitfld_->attach( rightOf, grossvolfld_ );
    unitfld_->setStretch( 0, 0 );

    instanceCreated().trigger( this );
}


uiImplBodyCalDlg::~uiImplBodyCalDlg()
{
    detachAllNotifiers();
    delete impbody_;
}


double uiImplBodyCalDlg::getVolume() const
{
    return volumeinm3_;
}


uiObject* uiImplBodyCalDlg::attachObject()
{
    return topgrp_->attachObj();
}


Notifier<uiImplBodyCalDlg>& uiImplBodyCalDlg::instanceCreated()
{
    mDefineStaticLocalObject( Notifier<uiImplBodyCalDlg>, theNotif, (0) );
    return theNotif;
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
    if ( !taskrunner.execute(bc) )
	return;

    volumeinm3_ = bc.getVolume();
    unitChgCB( nullptr );
    calcPushed.trigger();
}


void uiImplBodyCalDlg::unitChgCB( CallBacker* )
{
    if ( mIsUdf(volumeinm3_) )
    {
	grossvolfld_->setEmpty();
	return;
    }

    const UnitOfMeasure* uom = unitfld_->getUnit();
    const double grossvol = getConvertedValue( volumeinm3_, nullptr, uom );
    grossvolfld_->setValue( grossvol );
}


void uiImplBodyCalDlg::getImpBody()
{
    delete impbody_;

    uiTaskRunner taskrunner(this);
    impbody_ = embody_.createImplicitBody(&taskrunner,false);
}
