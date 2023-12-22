/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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
    return sgvs ? new uiLateralStack( p, sgvs ) : nullptr;
}


uiLateralStack::uiLateralStack( uiParent* p, LateralStack* sgvs )
    : uiDialog( p, uiDialog::Setup(tr("Super Gather setup"),mNoDlgTitle,
                                    mODHelpKey(mPreStackVerticalStackHelpID) ) )
    , processor_(sgvs)
{
    const bool is3d = ::is3D( processor_->getGeomSystem() );
    const BinID& stepout = processor_->getPatternStepout();
    if ( is3d )
    {
	stepoutfld_ = new uiGenInput( this, tr("Stepout (inl/crl)"),
				      PositionInpSpec( stepout, false));
	iscrossfld_ = new uiGenInput( this, tr("Shape"),
				BoolInpSpec( processor_->isCross(),
				tr("Cross"), uiStrings::sRectangle()) );
	iscrossfld_->attach( alignedBelow, stepoutfld_ );
    }
    else
    {
	stepoutfld_ = new uiGenInput( this, tr("Stepout"),
				      PositionInpSpec( stepout.trcNr() ) );
    }
}


uiLateralStack::~uiLateralStack()
{}


bool uiLateralStack::acceptOK( CallBacker* )
{
    if ( !processor_ )
	return true;

    const bool is3d = ::is3D( processor_->getGeomSystem() );
    BinID stepout = stepoutfld_->getBinID();
    if ( !is3d )
	stepout.inl() = 0;

    if ( mIsUdf(stepout.crl()) || stepout.crl()<0 ||
	 (is3d && (mIsUdf(stepout.inl()) || stepout.inl()<0 ||
		   (!stepout.inl() && !stepout.crl()))) )
    {
	uiMSG().error(tr("Stepout is not set to a valid range"));
	return false;
    }

    const bool rectangle = iscrossfld_ ? iscrossfld_->getBoolValue() : true;
    if ( !processor_->setPattern(stepout,rectangle) )
    {
	uiMSG().error( tr("Could not set stack pattern") );
	return false;
    }

    return true;
}



} // namespace PreStack
