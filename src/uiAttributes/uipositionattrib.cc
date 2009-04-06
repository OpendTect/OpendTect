/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          November 2002
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uipositionattrib.cc,v 1.15 2009-04-06 09:32:24 cvsranojay Exp $";


#include "uipositionattrib.h"
#include "positionattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uisteeringsel.h"
#include "uistepoutsel.h"

using namespace Attrib;

static const char* opstrs[] =
{
	"Min",
	"Max",
	"Median",
	0
};

mInitAttribUI(uiPositionAttrib,Position,"Position",sKeyPositionGrp())


uiPositionAttrib::uiPositionAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.10")
	
{
    inpfld = getInpFld(  is2d, "Input attribute" );

    stepoutfld = new uiStepOutSel( this, is2d );
    stepoutfld->setFieldNames( "Inl Stepout", "Crl Stepout" );
    stepoutfld->attach( alignedBelow, inpfld );

    gatefld = new uiGenInput( this, gateLabel(),
			      FloatInpIntervalSpec().setName("Z start",0)
						    .setName("Z stop",1) );
    gatefld->attach( alignedBelow, stepoutfld );

    steerfld = new uiSteeringSel( this, 0, is2d );
    steerfld->attach( alignedBelow, gatefld );

    operfld = new uiGenInput( this, "Operator", StringListInpSpec(opstrs) );
    operfld->attach( alignedBelow, steerfld );

    outfld = getInpFld( is2d, "Output attribute" );
    outfld->attach( alignedBelow, operfld );

    setHAlignObj( inpfld );
}


bool uiPositionAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),Position::attribName()) )
	return false;

    mIfGetFloatInterval( Position::gateStr(), gate, gatefld->setValue(gate) );
    mIfGetBinID( Position::stepoutStr(), stepout,
	    	 stepoutfld->setBinID(stepout) );
    mIfGetEnum( Position::operStr(), oper, operfld->setValue(oper) );

    return true;
}


bool uiPositionAttrib::setInput( const Desc& desc )
{
    putInp( inpfld, desc, 0 );
    putInp( outfld, desc, 1 );
    putInp( steerfld, desc, 2 );

    return true;
}


bool uiPositionAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),Position::attribName()) )
	return false;

    mSetFloatInterval( Position::gateStr(), gatefld->getFInterval() );
    mSetBinID( Position::stepoutStr(), stepoutfld->getBinID() );
    mSetEnum( Position::operStr(), operfld->getIntValue() );
    mSetBool( Position::steeringStr(), steerfld->willSteer() );

    return true;
}


bool uiPositionAttrib::getInput( Desc& desc )
{
    fillInp( inpfld, desc, 0 );
    fillInp( outfld, desc, 1 );
    fillInp( steerfld, desc, 2 );

    return true;
}


void uiPositionAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( timegatestr(), Position::gateStr() );
    params += EvalParam( stepoutstr(), Position::stepoutStr() );
}
