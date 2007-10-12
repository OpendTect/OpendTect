/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:		$Id: uienergyattrib.cc,v 1.10 2007-10-12 09:12:19 cvssulochana Exp $
________________________________________________________________________

-*/


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


mInitAttribUI(uiEnergyAttrib,Energy,"Energy",sKeyBasicGrp)


uiEnergyAttrib::uiEnergyAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.3")

{
    inpfld_ = getInpFld();

    gatefld_ = new uiGenInput( this, gateLabel(), FloatInpIntervalSpec() );
    gatefld_->attach( alignedBelow, inpfld_ );

    outpfld_ = new uiGenInput( this, "Output", StringListInpSpec(outpstrs) );
    outpfld_->attach( alignedBelow, gatefld_ );
    setHAlignObj( gatefld_ );
}


bool uiEnergyAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),Energy::attribName()) )
	return false;

    mIfGetFloatInterval( Energy::gateStr(), gate, gatefld_->setValue(gate) );
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
    params += EvalParam( timegatestr, Energy::gateStr() );
}
