/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		Feb 2009
________________________________________________________________________

-*/

#include "uihorizonshiftdlg.h"

#include "commondefs.h"
#include "emmanager.h"
#include "emobject.h"
#include "emsurfaceauxdata.h"
#include "emhorizon3d.h"
#include "ranges.h"
#include "survinfo.h"
#include "uiattrsel.h"
#include "uicombobox.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uislider.h"
#include "uimsg.h"
#include "od_helpids.h"


const char* uiHorizonShiftDialog::sDefaultAttribName()
{ return "Attribute Name"; }


uiHorizonShiftDialog::uiHorizonShiftDialog( uiParent* p,
					    const EM::ObjectID& emid,
					    int visid,
					    const Attrib::DescSet& descset,
					    float initialshift,
					    bool cancalcattrib )
    : uiDialog(p,uiDialog::Setup(tr("%1 shift").arg(uiStrings::sHorizon()),
				mNoDlgTitle,
				mODHelpKey(mHorizonShiftDialogHelpID) )
				.modal(false) )
    , calcAttribPushed(this)
    , horShifted(this)
    , calcshiftrg_(mUdf(float),mUdf(float),mUdf(float))
    , emid_(emid)
    , visid_(visid)
{
    const float curshift = initialshift*SI().zDomain().userFactor();
    shiftrg_.set( curshift-100, curshift+100, 10 );

    uiString lbl = tr("Shift Range %1").arg(SI().getUiZUnitString());
    rangeinpfld_ = new uiGenInput( this, lbl, FloatInpIntervalSpec(shiftrg_) );
    rangeinpfld_->valuechanged.notify(
	    mCB(this,uiHorizonShiftDialog,rangeChangeCB) );

    lbl = tr("Shift %1").arg(SI().getUiZUnitString());
    slider_ = new uiSlider(
	    this, uiSlider::Setup(lbl).withedit(true), "Horizon slider" );
    slider_->attach( alignedBelow, rangeinpfld_ );

    // TODO: Calculate slider range from horizon's z-range
    slider_->setScale( shiftrg_.step, shiftrg_.start );
    slider_->setInterval( shiftrg_ );
    slider_->setValue( curshift );
    slider_->valueChanged.notify( mCB(this,uiHorizonShiftDialog,shiftCB) );

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    mDynamicCast(EM::Horizon*,horizon_,emobj)
    if ( horizon_ )
    {
	horizon_->ref();
	uiString title = toUiString("%1 - %2").arg(setup().wintitle_)
					      .arg(horizon_->name());
	setCaption( title );
    }

    if ( cancalcattrib )
    {
	attrinpfld_ = new uiAttrSel( this, descset, "Select Attribute" );
	attrinpfld_->attach( alignedBelow, slider_ );
	attrinpfld_->selectionDone.notify(
		mCB(this,uiHorizonShiftDialog,attribChangeCB) );

	calbut_ = new uiPushButton( this, uiStrings::sCalculate(), false );
	calbut_->attach( rightTo, attrinpfld_ );
	calbut_->activated.notify( mCB(this,uiHorizonShiftDialog,calcAttrib) );

	storefld_ = new uiCheckBox( this,
		tr("Save attributes as Horizon Data on pressing OK") );
	storefld_->attach( alignedBelow, attrinpfld_ );
	storefld_->setChecked( false );
	storefld_->activated.notify(
		mCB(this,uiHorizonShiftDialog,setNameFldSensitive) );

	namefld_ = new uiGenInput( this, tr("Attribute Basename"),
				   StringInpSpec() );
	namefld_->attach( alignedBelow, storefld_ );
	attribChangeCB( 0 );
    }
}


uiHorizonShiftDialog::~uiHorizonShiftDialog()
{
    if ( horizon_ )
	horizon_->unRef();
}


int uiHorizonShiftDialog::nrSteps() const
{ return shiftrg_.nrSteps()+1; }


StepInterval<float> uiHorizonShiftDialog::shiftRg() const
{
    StepInterval<float> res = shiftrg_;
    res.start /= SI().zDomain().userFactor();
    res.stop /= SI().zDomain().userFactor();
    res.step /= SI().zDomain().userFactor();

    return res;
}


bool uiHorizonShiftDialog::doStore() const
{ return storefld_ ? storefld_->isChecked() : false; }


void uiHorizonShiftDialog::setNameFldSensitive( CallBacker* )
{
    namefld_->setSensitive( storefld_->isChecked() &&
		       (attrinpfld_ || attrinpfld_->attribID().isValid() ) );
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
    intv.stop = intv.snap( intv.stop );

    if ( shiftrg_ == intv )
	return;

    if ( (intv.start == intv.stop) || (intv.nrSteps()==0) )
    {
	rangeinpfld_->setValue( shiftrg_ );
	return;
    }

    shiftrg_ = intv;

    rangeinpfld_->setValue( shiftrg_ );

    if ( (calcshiftrg_ != shiftrg_) && (!mIsUdf(calcshiftrg_.start) &&
				        !mIsUdf(calcshiftrg_.stop)) )
    {
	if ( uiMSG().askGoOn(tr("Do you want to recalculate the range?")) )
	{
	    calcshiftrg_ = rangeinpfld_->getFStepInterval();
	    calcAttrib( 0 );
	}
    }

    slider_->setScale( shiftrg_.step, shiftrg_.start );
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

    return curshift / SI().zDomain().userFactor();
}


void uiHorizonShiftDialog::shiftCB( CallBacker* )
{
    horShifted.trigger();
}


int uiHorizonShiftDialog::curShiftIdx() const
{
    const float curshift = getShift() * SI().zDomain().userFactor();
    const int curshiftidx = shiftrg_.getIndex( curshift );
    if ( curshiftidx<0 || curshiftidx>=nrSteps() )
	return mUdf(int);

    return curshiftidx;
}


const char* uiHorizonShiftDialog::getAttribName() const
{ return attrinpfld_->getInput(); }


const char* uiHorizonShiftDialog::getAttribBaseName() const
{
    FixedString res;
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
	const FixedString nm = namefld_->text();
	if ( nm.isEmpty() || nm==sDefaultAttribName() )
	{
	    uiMSG().error(tr("No basename defined"));
	    return false;
	}
    }

    shiftCB( 0 );

    return true;
}
