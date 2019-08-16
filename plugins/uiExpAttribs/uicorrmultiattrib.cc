/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:       Rahul Gogia
 Date:	       June 2019
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


uiWord sDispName()
{
    return od_static_tr("sDispName","Multi-Target Correlation");
}

mInitAttribUI(uiCorrMultiAttrib,CorrMultiAttrib,sDispName(),sExperimentalGrp())


uiCorrMultiAttrib::uiCorrMultiAttrib(uiParent* p,bool is2d)
		  : uiAttrDescEd(p,is2d,HelpKey("tut","attrib"))
{
    inpfld_ = createInpFld( is2d, tr("Reference Attribute") );
    inpfld2_ = createInpFld( is2d, uiStrings::sSelAttrib() );
    gatefld_ = new uiGenInput( this, gateLabel(),
	       FloatInpIntervalSpec().setName("Z start",-28)
	       .setName("Z stop",28) );
    inpfld2_->attach( alignedBelow, inpfld_ );
    gatefld_->attach( alignedBelow, inpfld2_ );

    setHAlignObj(inpfld_);
}


uiCorrMultiAttrib::~uiCorrMultiAttrib()
{}


bool uiCorrMultiAttrib::setParameters(const Desc& desc)
{
    if ( desc.attribName() != CorrMultiAttrib::attribName() )
	return false;

    mIfGetFloatInterval(CorrMultiAttrib::gateStr(),gate,
			gatefld_->setValue(gate));
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

    mSetFloatInterval( CorrMultiAttrib::gateStr(), gatefld_->getFInterval() );
    return true;
}


uiRetVal uiCorrMultiAttrib::getInput(Desc& desc)
{
    uiRetVal uirv = fillInp( inpfld_, desc, 0 );
    uirv.add( fillInp(inpfld2_,desc,1) );
    return uirv;
}
