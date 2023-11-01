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

class HP_uiImplBodyCalDlg
{
mOD_DisableCopy(HP_uiImplBodyCalDlg)
public:
HP_uiImplBodyCalDlg()	{}
~HP_uiImplBodyCalDlg()	{}

    uiUnitSel*		unitfld_		= nullptr;
    float		volumeinm3_		= mUdf(float);
};

static HiddenParam<uiImplBodyCalDlg,HP_uiImplBodyCalDlg*> hp( nullptr );


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
		    tr("%1 %2")
		    .arg(VelocityDesc::getVelVolumeLabel())
		    .arg(UnitOfMeasure::surveyDefVelUnitAnnot(true,true)),
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

    hp.setParam( this, new HP_uiImplBodyCalDlg() );

    auto* unitfld = new uiUnitSel( this, Mnemonic::Vol );
    unitfld->setUnit( UoMR().get(SI().depthsInFeet() ? "ft3" : "m3") );
    mAttachCB( unitfld->selChange, uiImplBodyCalDlg::unitChgCB );
    unitfld->attach( rightOf, volfld_ );
    hp.getParam(this)->unitfld_ = unitfld;
}


uiImplBodyCalDlg::~uiImplBodyCalDlg()
{
    detachAllNotifiers();
    delete impbody_;
    hp.removeAndDeleteParam( this );
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

    float& volumeinm3 = hp.getParam(this)->volumeinm3_;
    volumeinm3 = mUdf(float);
    uiTaskRunner taskrunner(this);
    BodyVolumeCalculator bc( impbody_->tkzs_, *impbody_->arr_,
			     impbody_->threshold_, vel );
    if ( !TaskRunner::execute(&taskrunner,bc) )
	return;

    volumeinm3 = bc.getVolume();
    unitChgCB( nullptr );
}


void uiImplBodyCalDlg::unitChgCB( CallBacker* )
{
    const auto* extra = hp.getParam(this);
    BufferString txt;
    const UnitOfMeasure* uom = extra->unitfld_->getUnit();
    if ( uom && !mIsUdf(extra->volumeinm3_) )
    {
	const float newval = uom->getUserValueFromSI( extra->volumeinm3_ );
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
