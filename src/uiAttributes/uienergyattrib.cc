/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:		$Id: uienergyattrib.cc,v 1.6 2006-09-11 06:59:31 cvsnanne Exp $
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


mInitUI( uiEnergyAttrib, "Energy" )

uiEnergyAttrib::uiEnergyAttrib( uiParent* p )
	: uiAttrDescEd(p)
{
    inpfld = getInpFld();

    gatefld = new uiGenInput( this, gateLabel(), FloatInpIntervalSpec() );
    gatefld->attach( alignedBelow, inpfld );
    setHAlignObj( gatefld );
}


const char* uiEnergyAttrib::getAttribName() const
{ return Energy::attribName(); }


bool uiEnergyAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),Energy::attribName()) )
	return false;

    mIfGetFloatInterval( Energy::gateStr(), gate, gatefld->setValue(gate) );
    return true;
}


bool uiEnergyAttrib::setInput( const Desc& desc )
{
    putInp( inpfld, desc, 0 );
    return true;
}


bool uiEnergyAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),Energy::attribName()) )
	return false;

    mSetFloatInterval( Energy::gateStr(), gatefld->getFInterval() );
    return true;
}


bool uiEnergyAttrib::getInput( Desc& desc )
{
    fillInp( inpfld, desc, 0 );
    return true;
}


void uiEnergyAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( timegatestr, Energy::gateStr() );
}
