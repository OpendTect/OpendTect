/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          November 2002
 RCS:           $Id: uipositionattrib.cc,v 1.4 2006-09-11 06:59:31 cvsnanne Exp $
________________________________________________________________________

-*/

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


mInitUI( uiPositionAttrib, "Position" )

uiPositionAttrib::uiPositionAttrib( uiParent* p )
	: uiAttrDescEd(p)
{
    inpfld = getInpFld( "Input attribute" );

    stepoutfld = new uiStepOutSel( this );
    stepoutfld->attach( alignedBelow, inpfld );

    gatefld = new uiGenInput( this, gateLabel(), FloatInpIntervalSpec() );
    gatefld->attach( alignedBelow, stepoutfld );

    steerfld = new uiSteeringSel( this, 0 );
    steerfld->attach( alignedBelow, gatefld );

    operfld = new uiGenInput( this, "Operator", StringListInpSpec(opstrs) );
    operfld->attach( alignedBelow, steerfld );

    outfld = getInpFld( "Output attribute" );
    outfld->attach( alignedBelow, operfld );

    setHAlignObj( inpfld );
}


const char* uiPositionAttrib::getAttribName() const
{ return Position::attribName(); }


void uiPositionAttrib::set2D( bool yn )
{
    inpfld->set2D( yn );
    outfld->set2D( yn );
    stepoutfld->set2D( yn );
    steerfld->set2D( yn );
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
    mSetBinID( Position::stepoutStr(), stepoutfld->binID() );
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
    params += EvalParam( timegatestr, Position::gateStr() );
    params += EvalParam( stepoutstr, Position::stepoutStr() );
}
