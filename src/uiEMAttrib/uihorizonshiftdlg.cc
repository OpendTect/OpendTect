/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		Feb 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

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


const char* uiHorizonShiftDialog::sDefaultAttribName()
{ return "Attribute Name"; }
    

uiHorizonShiftDialog::uiHorizonShiftDialog( uiParent* p,
					    const EM::ObjectID& emid,
					    const Attrib::DescSet& descset,
					    float initialshift,
					    bool cancalcattrib )
    : uiDialog(p,uiDialog::Setup("Horizon shift",mNoDlgTitle,"104.0.15").
	    			  modal(false) )
    , calcshiftrg_(mUdf(float),mUdf(float),mUdf(float))
    , emhor3d_(0)
    , emid_(emid)
    , storefld_(0)
    , namefld_(0)
    , calbut_(0)
    , attrinpfld_(0)
    , calcAttribPushed(this)
    , horShifted(this)
{
    const float curshift = initialshift*SI().zFactor();
    shiftrg_ = StepInterval<float> (curshift-100,curshift+100,10);

    BufferString lbl( "Shift Range ", SI().getZUnitString() );
    rangeinpfld_ = new uiGenInput( this, lbl, FloatInpIntervalSpec(shiftrg_) );
    rangeinpfld_->valuechanged.notify(
	    mCB(this,uiHorizonShiftDialog,rangeChangeCB) );

    lbl = BufferString( "Shift ", SI().getZUnitString() );
    slider_ = new uiSliderExtra(
	    this, uiSliderExtra::Setup(lbl).withedit(true), "Horizon slider" );
    slider_->attach( alignedBelow, rangeinpfld_ );
    uiSlider* sldr = slider_->sldr();

    // TODO: Calculate slider range from horizon's z-range
    slider_->sldr()->setScale( shiftrg_.step, shiftrg_.start );
    slider_->sldr()->setInterval( shiftrg_ );
    slider_->sldr()->setValue( curshift );
    sldr->valueChanged.notify( mCB(this,uiHorizonShiftDialog,shiftCB) );

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    mDynamicCastGet(EM::Horizon3D*,emhor3d,emobj)
    emhor3d_ = emhor3d;
    emhor3d_->ref();

    if ( cancalcattrib )
    {
	attrinpfld_ = new uiAttrSel( this, descset, "Select Attribute" );
	attrinpfld_->attach( alignedBelow, slider_ );
	attrinpfld_->selectionDone.notify(
		mCB(this,uiHorizonShiftDialog,attribChangeCB) );
	
	calbut_ = new uiPushButton( this, "Calculate", false );
	calbut_->attach( rightTo, attrinpfld_ );
	calbut_->activated.notify( mCB(this,uiHorizonShiftDialog,calcAttrib) );

	storefld_ = new uiCheckBox( this, "Store Horizons on pressing Ok" );
	storefld_->attach( alignedBelow, attrinpfld_ );
	storefld_->setChecked( false );
	storefld_->activated.notify(
		mCB(this,uiHorizonShiftDialog,setNameFldSensitive) );
	
	namefld_ = new uiGenInput( this, "Attribute Basename",
				   StringInpSpec(sDefaultAttribName()) );
	namefld_->attach( alignedBelow, storefld_ );
	attribChangeCB( 0 );
    }
}


uiHorizonShiftDialog::~uiHorizonShiftDialog()
{
    emhor3d_->unRef();
}


int uiHorizonShiftDialog::nrSteps() const
{ return shiftrg_.nrSteps()+1; }


StepInterval<float> uiHorizonShiftDialog::shiftRg() const
{
    StepInterval<float> res = shiftrg_;
    res.start /= SI().zFactor();
    res.stop /= SI().zFactor();
    res.step /= SI().zFactor();

    return res;
}


bool uiHorizonShiftDialog::doStore() const
{ return storefld_ ? storefld_->isChecked() : false; }


void uiHorizonShiftDialog::setNameFldSensitive( CallBacker* )
{
    namefld_->display( storefld_->isChecked() &&
		       (attrinpfld_ || attrinpfld_->attribID().isValid() ) );
}


void uiHorizonShiftDialog::attribChangeCB( CallBacker* )
{
    const bool isok = attrinpfld_->attribID().isValid();
    calbut_->display( isok );
    storefld_->display( isok );
    namefld_->display( isok && storefld_->isChecked() );
     
    if ( !isok )
 	return;

    if ( uiMSG().question("Calculate now?" ) )
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
	if ( uiMSG().askGoOn("Do you want to recalculate on the give range") )
	{
	    calcshiftrg_ = rangeinpfld_->getFStepInterval();
	    calcAttrib( 0 );
	}
    }

    slider_->sldr()->setScale( shiftrg_.step, shiftrg_.start );
    slider_->sldr()->setInterval( shiftrg_ );
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
	curshift = slider_->sldr()->getValue();
    
    return curshift / SI().zFactor();
}


void uiHorizonShiftDialog::shiftCB( CallBacker* )
{
    const float curshift = getShift();
    horShifted.trigger();
}


int uiHorizonShiftDialog::curShiftIdx() const
{
    const float curshift = getShift() * SI().zFactor();
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
	    uiMSG().error("No basename defined");
	    return false;
	}
    }

    shiftCB( 0 );

    return true;
}
