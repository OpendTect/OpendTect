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
    FileMultiString keys( LateralStack::sFactoryKeyword() );
    keys += "VerticalStack";
    uiProcessorManager::factory().addCreator( create, keys,
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
    iscrossfld_ = new uiGenInput( this, uiStrings::sShape(),
	BoolInpSpec( processor_->isCross(), tr("Cross"),
                     uiStrings::sRectangle()) );
    iscrossfld_->attach( alignedBelow, stepoutfld_ );
}


bool uiLateralStack::acceptOK()
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
	uiMSG().error(tr("Cannot set stack pattern"));
	return false;
    }

    return true;
}



}; //namespace
