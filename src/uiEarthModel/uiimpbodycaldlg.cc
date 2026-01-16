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

#include "hiddenparam.h"


class uiImplBodyCalcHPGrp
{
public:

uiImplBodyCalcHPGrp( uiImplBodyCalDlg& dlg )
    : calcPushed(&dlg)
{
    topgrp_ = new uiGroup( &dlg, "Top Group" );
}

~uiImplBodyCalcHPGrp()
{
}

uiGroup* topGroup()
{
    return topgrp_;
}


double getVolume() const
{
    return volumeinm3_;
}


void setVolume( double volm3 )
{
    volumeinm3_ = volm3;
}

    Notifier<uiImplBodyCalDlg> calcPushed;

private:

    uiGroup*    topgrp_;
    double	volumeinm3_	= mUdf(double);

};

static HiddenParam<uiImplBodyCalDlg,uiImplBodyCalcHPGrp*>
				uiimplbodygrphpmgr_(nullptr);

mStartAllowDeprecatedSection

uiImplBodyCalDlg::uiImplBodyCalDlg( uiParent* p, const EM::Body& eb )
    : uiDialog(p,Setup(tr("Calculate Geobody Volume"),
		       mODHelpKey(mImplBodyCalDlgHelpID)))
    , embody_(eb)
    , velfld_(nullptr)
    , volfld_(nullptr)
    , impbody_(nullptr)
{
    setCtrlStyle( CloseOnly );
    auto* hpgrp = new uiImplBodyCalcHPGrp( *this );
    uiimplbodygrphpmgr_.setParam ( this, hpgrp );

    uiGroup* topgrp = hpgrp->topGroup();
    if ( SI().zIsTime() )
    {
	velfld_ = new uiGenInput( topgrp,
		    tr("%1 %2")
		    .arg(VelocityDesc::getVelVolumeLabel())
		    .arg(UnitOfMeasure::surveyDefVelUnitAnnot(true,true)),
			 FloatInpSpec(Vel::getGUIDefaultVelocity()) );
	velfld_->setStretch( 0, 0 );
    }

    auto* calcvolbut = new uiPushButton( topgrp, tr("Calculate Volume"),
					 "math", true );
    calcvolbut->setStretch( 0, 0 );
    mAttachCB( calcvolbut->activated, uiImplBodyCalDlg::calcCB );
    if ( velfld_ )
	calcvolbut->attach( alignedBelow, velfld_ );

    volfld_ = new uiGenInput( topgrp, tr("Gross Volume") );
    volfld_->setReadOnly( true );
    volfld_->attach( alignedBelow, calcvolbut );
    volfld_->setStretch( 0, 0 );
    topgrp->setHAlignObj( volfld_ );

    uiUnitSel::Setup unitsetup( Mnemonic::Vol );
    unitsetup.variableszpol( true );
    unitfld_ = new uiUnitSel( topgrp, unitsetup );
    unitfld_->setUnit( UoMR().get(SI().depthsInFeet() ? "ft3" : "m3") );
    mAttachCB( unitfld_->selChange, uiImplBodyCalDlg::unitChgCB );
    unitfld_->attach( rightOf, volfld_ );
    unitfld_->setStretch( 0, 0 );

    instanceCreated().trigger( this );
}

mStopAllowDeprecatedSection


uiImplBodyCalDlg::~uiImplBodyCalDlg()
{
    detachAllNotifiers();
    delete impbody_;
    uiimplbodygrphpmgr_.removeAndDeleteParam( this );
}


uiGroup* uiImplBodyCalDlg::topGroup()
{
    return uiimplbodygrphpmgr_.getParam( this )->topGroup();
}


Notifier<uiImplBodyCalDlg>& uiImplBodyCalDlg::calcPushed()
{
    return uiimplbodygrphpmgr_.getParam( this )->calcPushed;
}


double uiImplBodyCalDlg::getVolume() const
{
    return uiimplbodygrphpmgr_.getParam( this )->getVolume();
}


void uiImplBodyCalDlg::setVolumeD( double volm3 )
{
    uiimplbodygrphpmgr_.getParam( this )->setVolume( volm3 );
}


uiObject* uiImplBodyCalDlg::attachObject()
{
    return topGroup()->attachObj();
}


Notifier<uiImplBodyCalDlg>& uiImplBodyCalDlg::instanceCreated()
{
    mDefineStaticLocalObject( Notifier<uiImplBodyCalDlg>, thenotif, = nullptr );
    return thenotif;
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

    setVolumeD( mUdf(double) );
    uiTaskRunner taskrunner(this);
    BodyVolumeCalculator bc( impbody_->tkzs_, *impbody_->arr_,
			     impbody_->threshold_, vel );
    if ( !taskrunner.execute(bc) )
	return;

    setVolumeD( bc.getVolume() );
    unitChgCB( nullptr );
    calcPushed().trigger();
}


void uiImplBodyCalDlg::unitChgCB( CallBacker* )
{
    const double volumeinm3 = getVolume();
    if ( mIsUdf(volumeinm3) )
    {
	volfld_->setEmpty();
	return;
    }

    const UnitOfMeasure* uom = unitfld_->getUnit();
    const float grossvol =
		    mCast(float,getConvertedValue( volumeinm3, nullptr, uom ) );
    volfld_->setValue( grossvol );
}


void uiImplBodyCalDlg::getImpBody()
{
    delete impbody_;

    uiTaskRunner taskrunner(this);
    impbody_ = embody_.createImplicitBody(&taskrunner,false);
}
