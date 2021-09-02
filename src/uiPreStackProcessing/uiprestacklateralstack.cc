/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/


#include "uiprestacklateralstack.h"

#include "separstr.h"
#include "uiprestackprocessor.h"
#include "prestacklateralstack.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "od_helpids.h"

namespace PreStack
{

void uiLateralStack::initClass()
{
    SeparString names( LateralStack::sFactoryKeyword(),
	    		FactoryBase::cSeparator() );
    names += "VerticalStack";
    uiPSPD().addCreator( create, names.buf(),
	    		 LateralStack::sFactoryDisplayName() );
}


uiDialog* uiLateralStack::create( uiParent* p, Processor* sgp )
{
    mDynamicCastGet( LateralStack*, sgvs, sgp );
    if ( !sgvs ) return 0;

    return new uiLateralStack( p, sgvs );
}


uiLateralStack::uiLateralStack( uiParent* p, LateralStack* sgvs )
    : uiDialog( p, uiDialog::Setup(tr("Super Gather setup"),mNoDlgTitle,
                                    mODHelpKey(mPreStackVerticalStackHelpID) ) )
    , processor_( sgvs )
{
    stepoutfld_ = new uiGenInput( this, tr("Stepout (inl/crl)"),
		     PositionInpSpec( processor_->getPatternStepout(), false));
    iscrossfld_ = new uiGenInput( this, tr("Shape"),
	BoolInpSpec( processor_->isCross(), tr("Cross"), 
                     uiStrings::sRectangle()) );
    iscrossfld_->attach( alignedBelow, stepoutfld_ );
}


bool uiLateralStack::acceptOK( CallBacker* )
{
    if ( !processor_ ) return true;

    const BinID stepout = stepoutfld_->getBinID();
    if ( mIsUdf(stepout.inl()) || mIsUdf(stepout.crl()) ||
	 stepout.inl()<0 || stepout.crl()<0 ||
	 (!stepout.inl() && !stepout.crl()) )
    {
	uiMSG().error(tr("Stepout is not set to a valid range"));
	return false;
    }

    if ( !processor_->setPattern( stepout, iscrossfld_->getBoolValue() ) )
    {
	uiMSG().error(tr("Could not set stack pattern"));
	return false;
    }

    return true;
}



}; //namespace
