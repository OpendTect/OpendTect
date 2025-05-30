/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uihorizonshiftdlg.h"

#include "commondefs.h"
#include "emhorizon.h"
#include "emmanager.h"
#include "emobject.h"
#include "od_helpids.h"
#include "ranges.h"
#include "survinfo.h"

#include "uiattrsel.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uislider.h"


const char* uiHorizonShiftDialog::sDefaultAttribName()
{ return "Attribute Name"; }


uiHorizonShiftDialog::uiHorizonShiftDialog( uiParent* p,
					    const EM::ObjectID& emid,
					    const VisID& visid,
					    const Attrib::DescSet& descset,
					    float initialshift,
					    bool cancalcattrib )
    : uiDialog(p,Setup(tr("%1 shift").arg(uiStrings::sHorizon()),
		       mODHelpKey(mHorizonShiftDialogHelpID)).modal(false))
    , calcAttribPushed(this)
    , horShifted(this)
    , calcshiftrg_(mUdf(float),mUdf(float),mUdf(float))
    , emid_(emid)
    , visid_(visid)
{
    const float curshift =
		initialshift * sCast(float,SI().zDomain().userFactor());
    shiftrg_.set( curshift-100, curshift+100, 10 );

    uiString lbl = tr("Shift Range %1").arg(SI().getUiZUnitString());
    rangeinpfld_ = new uiGenInput( this, lbl, FloatInpIntervalSpec(shiftrg_) );
    mAttachCB( rangeinpfld_->valueChanged, uiHorizonShiftDialog::rangeChangeCB);

    lbl = tr("Shift %1").arg(SI().getUiZUnitString());
    slider_ = new uiSlider(
	    this, uiSlider::Setup(lbl).withedit(true), "Horizon slider" );
    slider_->attach( alignedBelow, rangeinpfld_ );

    // TODO: Calculate slider range from horizon's z-range
    slider_->setScale( shiftrg_.step_, shiftrg_.start_ );
    slider_->setInterval( shiftrg_ );
    slider_->setValue( curshift );
    mAttachCB( slider_->valueChanged, uiHorizonShiftDialog::shiftCB );

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    horizon_ = dCast(EM::Horizon*,emobj);
    if ( horizon_ )
    {
	uiString title = toUiString("%1 - %2").arg(setup().wintitle_)
					      .arg(horizon_->name());
	setCaption( title );
    }

    if ( cancalcattrib )
    {
	attrinpfld_ = new uiAttrSel( this, descset,
			uiStrings::phrSelect(uiStrings::sAttribute()) );
	attrinpfld_->attach( alignedBelow, slider_ );
	mAttachCB( attrinpfld_->selectionDone,
		   uiHorizonShiftDialog::attribChangeCB );

	calbut_ = new uiPushButton( this, uiStrings::sCalculate(), false );
	calbut_->attach( rightTo, attrinpfld_ );
	mAttachCB( calbut_->activated,
		   uiHorizonShiftDialog::calcAttrib );

	storefld_ = new uiCheckBox( this,
		tr("Save attributes as Horizon Data on pressing OK") );
	storefld_->attach( alignedBelow, attrinpfld_ );
	storefld_->setChecked( false );
	mAttachCB( storefld_->activated,
		   uiHorizonShiftDialog::setNameFldSensitive );

	namefld_ = new uiGenInput( this, tr("Attribute Basename"),
				   StringInpSpec() );
	namefld_->attach( alignedBelow, storefld_ );
	attribChangeCB( 0 );
    }
}


uiHorizonShiftDialog::~uiHorizonShiftDialog()
{
    detachAllNotifiers();
}


void uiHorizonShiftDialog::setDescSet( const Attrib::DescSet* ds )
{
    if ( attrinpfld_ )
	attrinpfld_->setDescSet( ds );
}


void uiHorizonShiftDialog::setNLAModel( const NLAModel* nlamodel )
{
    if ( attrinpfld_ )
	attrinpfld_->setNLAModel( nlamodel );
}


int uiHorizonShiftDialog::nrSteps() const
{
    return shiftrg_.nrSteps()+1;
}


StepInterval<float> uiHorizonShiftDialog::shiftRg() const
{
    StepInterval<float> res = shiftrg_;
    res.scale( sCast(float,1.f/SI().zDomain().userFactor()) );
    return res;
}


bool uiHorizonShiftDialog::doStore() const
{
    return storefld_ ? storefld_->isChecked() : false;
}


void uiHorizonShiftDialog::setNameFldSensitive( CallBacker* )
{
    namefld_->setSensitive( storefld_->isChecked() &&
			    attrinpfld_ && attrinpfld_->attribID().isValid() );
}


void uiHorizonShiftDialog::attribChangeCB( CallBacker* )
{
    const bool isok = attrinpfld_->attribID().isValid();
    calbut_->setSensitive( isok );
    storefld_->setSensitive( isok );
    namefld_->setSensitive( isok && storefld_->isChecked() );
    namefld_->setText( attrinpfld_->getAttrName() );

    if ( !isok )
	return;

    if ( uiMSG().question(tr("Calculate now?")) )
	calcAttrib( 0 );
}


void uiHorizonShiftDialog::rangeChangeCB( CallBacker* )
{
    StepInterval<float> intv = rangeinpfld_->getFStepInterval();
    intv.stop_ = intv.snap( intv.stop_ );

    if ( shiftrg_ == intv )
	return;

    if ( (intv.start_ == intv.stop_) || (intv.nrSteps()==0) )
    {
	rangeinpfld_->setValue( shiftrg_ );
	return;
    }

    shiftrg_ = intv;

    rangeinpfld_->setValue( shiftrg_ );

    if ( (calcshiftrg_ != shiftrg_) && (!mIsUdf(calcshiftrg_.start_) &&
					!mIsUdf(calcshiftrg_.stop_)) )
    {
	if ( uiMSG().askGoOn(tr("Do you want to recalculate the range?")) )
	{
	    calcshiftrg_ = rangeinpfld_->getFStepInterval();
	    calcAttrib( 0 );
	}
    }

    slider_->setScale( shiftrg_.step_, shiftrg_.start_ );
    slider_->setInterval( shiftrg_ );
    rangeinpfld_->setValue( shiftrg_ );
}


void uiHorizonShiftDialog::calcAttrib( CallBacker* )
{
    if ( !attrinpfld_->attribID().isValid() )
	return;

    calcshiftrg_ = shiftrg_;
    calcAttribPushed.trigger();
}


Attrib::DescID uiHorizonShiftDialog::attribID() const
{ return attrinpfld_->attribID(); }



float uiHorizonShiftDialog::getShift() const
{
    float curshift = slider_->editValue();
    if ( mIsUdf(curshift) )
	curshift = slider_->getFValue();

    return curshift / sCast(float,SI().zDomain().userFactor());
}


void uiHorizonShiftDialog::shiftCB( CallBacker* )
{
    horShifted.trigger();
}


int uiHorizonShiftDialog::curShiftIdx() const
{
    const float curshift =
		getShift() * sCast(float,SI().zDomain().userFactor());
    const int curshiftidx = shiftrg_.getIndex( curshift );
    if ( curshiftidx<0 || curshiftidx>=nrSteps() )
	return mUdf(int);

    return curshiftidx;
}


const char* uiHorizonShiftDialog::getAttribName() const
{
    return attrinpfld_->getInput();
}


const char* uiHorizonShiftDialog::getAttribBaseName() const
{
    StringView res;
    if ( storefld_->isChecked() )
       res = namefld_->text();

    if ( res.isEmpty() || res==sDefaultAttribName() )
	res = attrinpfld_->getInput();

    return res.str();
}


bool uiHorizonShiftDialog::acceptOK( CallBacker* )
{
    if ( storefld_ && storefld_->isChecked() )
    {
	const StringView nm = namefld_->text();
	if ( nm.isEmpty() || nm==sDefaultAttribName() )
	{
	    uiMSG().error(tr("No basename defined"));
	    return false;
	}
    }

    shiftCB( 0 );
    return true;
}
