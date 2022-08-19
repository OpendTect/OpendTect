/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicorrmultiattrib.h"
#include "corrmultiattrib.h"
#include "uibutton.h"
#include "attribdesc.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "survinfo.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uisteeringsel.h"
#include "uistepoutsel.h"


using namespace Attrib;


mInitAttribUI(uiCorrMultiAttrib,CorrMultiAttrib,
	      "Attribute-Attribute Correlation","Experimental")


uiCorrMultiAttrib::uiCorrMultiAttrib(uiParent* p,bool is2d)
		  : uiAttrDescEd(p,is2d,HelpKey("tut","attrib"))
{
    inpfld_ = createInpFld( is2d, "Reference Attribute" );
    inpfld2_ = createInpFld( is2d, "Select Attribute" );
    gatefld_ = new uiGenInput( this, gateLabel(),
		FloatInpIntervalSpec().setName("Z start",-28)
				      .setName("Z stop",28) );
    inpfld2_->attach( alignedBelow, inpfld_ );
    gatefld_->attach( alignedBelow, inpfld2_ );

    setHAlignObj(inpfld_);
}


uiCorrMultiAttrib::~uiCorrMultiAttrib()
{}


bool uiCorrMultiAttrib::setParameters( const Desc& desc )
{
    if ( desc.attribName() != CorrMultiAttrib::attribName() )
	return false;

    mIfGetFloatInterval(CorrMultiAttrib::gateStr(),gate,
			gatefld_->setValue(gate))
    return true;
}


bool uiCorrMultiAttrib::setInput(const Desc& desc)
{
    putInp( inpfld_, desc, 0 );
    putInp( inpfld2_, desc, 1 );
    return true;
}


bool uiCorrMultiAttrib::getParameters(Desc& desc)
{
    if ( desc.attribName() != CorrMultiAttrib::attribName() )
	return false;

    mSetFloatInterval( CorrMultiAttrib::gateStr(), gatefld_->getFInterval() )
    return true;
}


bool uiCorrMultiAttrib::getInput(Desc& desc)
{
    fillInp( inpfld_, desc, 0 );
    fillInp( inpfld2_, desc, 1 );
    return true;
}
