/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Satyaki Maitra
 Date:		Feb 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uihorizonshiftdlg.cc,v 1.1 2009-03-10 06:53:21 cvssatyaki Exp $";

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


uiHorizonShiftDialog::uiHorizonShiftDialog( uiParent* p, const Setup& setup,
					    const Attrib::DescSet& descset )
    : uiDialog(p,uiDialog::Setup("Horizon shift",mNoDlgTitle,mNoHelpID).
	    			  modal(false) )
    , shiftrg_(setup.shiftrg_)
    , calcshiftrg_(setup.shiftrg_)
    , emhor3d_(0)
    , curshift_(0.0)
    , curshiftidx_(setup.shiftidx_)
    , emid_(setup.emid_)
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
	    mCB(this,uiHorizonShiftDialog,setSliderRange) );

    BufferString lbl( "Shift ", SI().getZUnitString() );
    slider_ = new uiSliderExtra(
	    this, uiSliderExtra::Setup(lbl).withedit(true), "Horizon slider" );
    slider_->attach( alignedBelow, rangeinpfld_ );
    uiSlider* sldr = slider_->sldr();

    // TODO: Calculate slider range from horizon's z-range
    sldr->setScale( shiftrg_.step, 0 );
    sldr->setInterval( shiftrg_ );
    sldr->valueChanged.notify( mCB(this,uiHorizonShiftDialog,shiftCB) );

    EM::EMObject* emobj = EM::EMM().getObject( setup.emid_ );
    mDynamicCastGet(EM::Horizon3D*,emhor3d,emobj)
    emhor3d_ = emhor3d;
    emhor3d_->ref();

    curshift_ = shiftrg_.start + ( curshiftidx_ * shiftrg_.step );
    slider_->sldr()->setValue( curshift_ );

    attrinpfld_ = new uiAttrSel( this, descset, "Select Attribute" );
    attrinpfld_->attach( alignedBelow, slider_ );
    
    calbut_ = new uiToolButton( this, "Calculate" );
    calbut_->attach( rightTo, attrinpfld_ );
    calbut_->activated.notify( mCB(this,uiHorizonShiftDialog,calcAttrib) );

    storefld_ = new uiCheckBox( this, "Store Horizons on pressing Ok" );
    storefld_->attach( alignedBelow, attrinpfld_ );
    storefld_->setSensitive( false );
    storefld_->setChecked( false );
    
    namefld_ = new uiGenInput( this, "Attribute Basename",
			       StringInpSpec("Attribute Name") );
    namefld_->attach( alignedBelow, storefld_ );
    namefld_->setSensitive( false );
}


uiHorizonShiftDialog::~uiHorizonShiftDialog()
{
    emhor3d_->unRef();
}


bool uiHorizonShiftDialog::doStore() const
{ return storefld_ ? storefld_->isChecked() : false; }


void uiHorizonShiftDialog::setSliderRange( CallBacker* )
{
    const StepInterval<float> intv = rangeinpfld_->getFStepInterval();
    if ( (intv.start == intv.stop) || (intv.nrSteps()==0) )
    {
	rangeinpfld_->setValue( shiftrg_ );
	return;
    }
    shiftrg_ = rangeinpfld_->getFStepInterval();
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
    slider_->sldr()->setScale( shiftrg_.step, 0 );
    slider_->sldr()->setInterval( shiftrg_ );
    rangeinpfld_->setValue( shiftrg_ );
}


void uiHorizonShiftDialog::calcAttrib( CallBacker* )
{
    if ( !attrinpfld_->attribID().isValid() )
	return;
    attribnm_ = attrinpfld_->getInput();
    storefld_->setSensitive( true );
    namefld_->setSensitive( true );
    attrid_ = attrinpfld_->attribID();
    shiftrg_ = rangeinpfld_->getFStepInterval();
    calcshiftrg_ = rangeinpfld_->getFStepInterval();
    curshiftidx_ = shiftrg_.getIndex( slider_->sldr()->getValue() ); 
    calcAttribPushed.trigger();
}


void uiHorizonShiftDialog::shiftCB( CallBacker* )
{
    if ( shiftrg_ != calcshiftrg_ )
	return;
    curshift_ = slider_->sldr()->getValue() / SI().zFactor();
    curshiftidx_ = shiftrg_.getIndex( slider_->sldr()->getValue() ); 
    emhor3d_->geometry().setShift( curshift_ );
    horShifted.trigger();
}


bool uiHorizonShiftDialog::acceptOK( CallBacker* )
{
    slider_->processInput();
    return true;
}
