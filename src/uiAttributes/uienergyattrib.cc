/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uienergyattrib.h"
#include "energyattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "od_helpids.h"

using namespace Attrib;

static const char* outpstrs[] =
{
    "Energy",
    "Sqrt ( Energy )",
    "Ln ( Energy )",
    0
};


mInitAttribUI(uiEnergyAttrib,Energy,"Energy",sKeyBasicGrp())


uiEnergyAttrib::uiEnergyAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d, mODHelpKey(mEnergyAttribHelpID) )

{
    inpfld_ = createInpFld( is2d );

    gatefld_ = new uiGenInput( this, gateLabel(),
	    		FloatInpIntervalSpec().setName("Z start",0)
					      .setName("Z stop",1) );
    gatefld_->attach( alignedBelow, inpfld_ );

    gradientfld_ = new uiGenInput( this, tr("Compute gradient"), 
                                            BoolInpSpec(true));
    gradientfld_->attach( alignedBelow, gatefld_ );

    outpfld_ = new uiGenInput( this, uiStrings::sOutput(), 
                              StringListInpSpec(outpstrs) );
    outpfld_->attach( alignedBelow, gradientfld_ );
    setHAlignObj( gatefld_ );
}


uiEnergyAttrib::~uiEnergyAttrib()
{}


bool uiEnergyAttrib::setParameters( const Desc& desc )
{
    if ( desc.attribName() != Energy::attribName() )
	return false;

    mIfGetFloatInterval( Energy::gateStr(), gate, gatefld_->setValue(gate) );
    mIfGetBool( Energy::dogradStr(), dograd, gradientfld_->setValue(dograd) );
    return true;
}


bool uiEnergyAttrib::setInput( const Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    return true;
}


bool uiEnergyAttrib::setOutput( const Attrib::Desc& desc )
{
    outpfld_->setValue( desc.selectedOutput() );
    return true;
}


bool uiEnergyAttrib::getParameters( Desc& desc )
{
    if ( desc.attribName() != Energy::attribName() )
	return false;

    mSetFloatInterval( Energy::gateStr(), gatefld_->getFInterval() );
    mSetBool( Energy::dogradStr(), gradientfld_->getBoolValue() );
    return true;
}


bool uiEnergyAttrib::getInput( Desc& desc )
{
    fillInp( inpfld_, desc, 0 );
    return true;
}


bool uiEnergyAttrib::getOutput( Attrib::Desc& desc )
{
    fillOutput( desc, outpfld_->getIntValue() );
    return true;
}


void uiEnergyAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( zGateLabel(), Energy::gateStr() );
}
