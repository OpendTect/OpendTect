/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2005
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";



#include "uienergyattrib.h"
#include "energyattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"

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
	: uiAttrDescEd(p,is2d,"101.0.3")

{
    inpfld_ = createInpFld( is2d );

    gatefld_ = new uiGenInput( this, gateLabel(),
	    		FloatInpIntervalSpec().setName("Z start",0)
					      .setName("Z stop",1) );
    gatefld_->attach( alignedBelow, inpfld_ );

    gradientfld_ = new uiGenInput( this, "Compute gradient", BoolInpSpec(true));
    gradientfld_->attach( alignedBelow, gatefld_ );

    outpfld_ = new uiGenInput( this, "Output", StringListInpSpec(outpstrs) );
    outpfld_->attach( alignedBelow, gradientfld_ );
    setHAlignObj( gatefld_ );
}


bool uiEnergyAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),Energy::attribName()) )
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
    if ( strcmp(desc.attribName(),Energy::attribName()) )
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
    params += EvalParam( timegatestr(), Energy::gateStr() );
}
