/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Satyaki Maitra
 Date:		Feb 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uihorizonshiftdlg.cc,v 1.5 2009-06-23 21:16:41 cvskris Exp $";

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
					    bool cancalcattrib )
    : uiDialog(p,uiDialog::Setup("Horizon shift",mNoDlgTitle,mNoHelpID).
	    			  modal(false) )
    , shiftrg_(-100,100,10)
    , calcshiftrg_(mUdf(float),mUdf(float),mUdf(float))
    , emhor3d_(0)
    , curshift_(0.0)
    , curshiftidx_(10)
    , emid_(emid)
    , storefld_(0)
    , namefld_(0)
    , calbut_(0)
    , attrinpfld_(0)
    , calcAttribPushed(this)
    , horShifted(this)
{
    rangeinpfld_ = new uiGenInput( this, "Shift Range",
	    			   FloatInpIntervalSpec(shiftrg_) );
    rangeinpfld_->valuechanged.notify(
	    mCB(this,uiHorizonShiftDialog,rangeChangeCB) );

    BufferString lbl( "Shift ", SI().getZUnitString() );
    slider_ = new uiSliderExtra(
	    this, uiSliderExtra::Setup(lbl).withedit(true), "Horizon slider" );
    slider_->attach( alignedBelow, rangeinpfld_ );
    uiSlider* sldr = slider_->sldr();

    // TODO: Calculate slider range from horizon's z-range
    sldr->setScale( shiftrg_.step, 0 );
    sldr->setInterval( shiftrg_ );
    sldr->valueChanged.notify( mCB(this,uiHorizonShiftDialog,shiftCB) );

    EM::EMObject* emobj = EM::EMM().getObject( emid_ );
    mDynamicCastGet(EM::Horizon3D*,emhor3d,emobj)
    emhor3d_ = emhor3d;
    emhor3d_->ref();

    curshift_ = shiftrg_.start + ( curshiftidx_ * shiftrg_.step );
    slider_->sldr()->setValue( curshift_ );

    if ( cancalcattrib )
    {
	attrinpfld_ = new uiAttrSel( this, descset, "Select Attribute" );
	attrinpfld_->attach( alignedBelow, slider_ );
	
	calbut_ = new uiPushButton( this, "Calculate", false );
	calbut_->attach( rightTo, attrinpfld_ );
	calbut_->activated.notify( mCB(this,uiHorizonShiftDialog,calcAttrib) );

	storefld_ = new uiCheckBox( this, "Store Horizons on pressing Ok" );
	storefld_->attach( alignedBelow, attrinpfld_ );
	storefld_->setSensitive( false );
	storefld_->setChecked( false );
	storefld_->activated.notify(
		mCB(this,uiHorizonShiftDialog,setNameFldSensitive) );
	
	namefld_ = new uiGenInput( this, "Attribute Basename",
				   StringInpSpec(sDefaultAttribName()) );
	namefld_->attach( alignedBelow, storefld_ );
	namefld_->setSensitive( false );
    }
}


uiHorizonShiftDialog::~uiHorizonShiftDialog()
{
    emhor3d_->unRef();
}


bool uiHorizonShiftDialog::doStore() const
{ return storefld_ ? storefld_->isChecked() : false; }


void uiHorizonShiftDialog::setNameFldSensitive( CallBacker* )
{
    namefld_->setSensitive( storefld_->isChecked() );
}


void uiHorizonShiftDialog::rangeChangeCB( CallBacker* )
{
    const StepInterval<float> intv = rangeinpfld_->getFStepInterval();
    if ( shiftrg_ == intv )
	return;

    if ( (intv.start == intv.stop) || (intv.nrSteps()==0) )
    {
	rangeinpfld_->setValue( shiftrg_ );
	return;
    }

    shiftrg_ = intv;

    if ( !mIsZero((int)shiftrg_.start%(int)shiftrg_.step,mDefEps) )
    {
	const int fac = (int)shiftrg_.start/(int)shiftrg_.step;
	shiftrg_.start = shiftrg_.step * fac;
    }
    if ( !mIsZero((int)shiftrg_.stop%(int)shiftrg_.step,mDefEps) )
    {
	const int fac = (int)shiftrg_.stop/(int)shiftrg_.step;
	shiftrg_.stop = shiftrg_.step * fac;
    }

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

    slider_->sldr()->setScale( shiftrg_.step, 0 );
    slider_->sldr()->setInterval( shiftrg_ );
    rangeinpfld_->setValue( shiftrg_ );
}


void uiHorizonShiftDialog::calcAttrib( CallBacker* )
{
    if ( !attrinpfld_->attribID().isValid() )
	return;

    storefld_->setSensitive( true );
    calcshiftrg_ = shiftrg_;
    curshiftidx_ = shiftrg_.getIndex( slider_->sldr()->getValue() ); 
    calcAttribPushed.trigger();
}


Attrib::DescID uiHorizonShiftDialog::attribID() const
{ return attrinpfld_->attribID(); }



void uiHorizonShiftDialog::shiftCB( CallBacker* )
{
    curshift_ = slider_->sldr()->getValue() / SI().zFactor();
    curshiftidx_ = shiftrg_.getIndex( slider_->sldr()->getValue() ); 
    emhor3d_->geometry().setShift( curshift_ );
    horShifted.trigger();
}


const char* uiHorizonShiftDialog::getAttribName() const
{ return attrinpfld_->getInput(); }


const char* uiHorizonShiftDialog::getAttribBaseName() const
{
    FixedString res = namefld_->text();
    if ( res.isEmpty() || res==sDefaultAttribName() )
	res = attrinpfld_->getInput();

    return res.buf();
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

    slider_->processInput();
    shiftCB( 0 );

    return true;
}
