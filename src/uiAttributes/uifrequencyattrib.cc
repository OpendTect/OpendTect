/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2001
 RCS:           $Id: uifrequencyattrib.cc,v 1.14 2007-12-06 11:08:21 cvshelene Exp $
________________________________________________________________________

-*/

#include "uifrequencyattrib.h"
#include "frequencyattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uiwindowfunctionsel.h"

using namespace Attrib;

static const char* outpstrs[] =
{
	"Dominant frequency",
	"Average frequency",
	"Median frequency",
	"Average frequency Squared",
	"Maximum spectral amplitude",
	"Spectral Area beyond dominant frequency",
	"Frequency Slope Fall",
	"Absorption Quality Factor",
	0
};


mInitAttribUI(uiFrequencyAttrib,Frequency,"Frequency",sKeyFreqGrp)


uiFrequencyAttrib::uiFrequencyAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.6")

{
    inpfld = getImagInpFld();

    gatefld = new uiGenInput( this, gateLabel(), FloatInpIntervalSpec() );
    gatefld->attach( alignedBelow, inpfld );

    normfld = new uiGenInput( this, "Normalize", BoolInpSpec(false) );
    normfld->attach( alignedBelow, gatefld );

    winfld = new uiWindowFunctionSel( this, "Window/Taper" );
    winfld->attach( alignedBelow, normfld ); 

    outpfld = new uiGenInput( this, "Output", StringListInpSpec(outpstrs) );
    outpfld->setElemSzPol( uiObject::WideVar );
    outpfld->attach( alignedBelow, winfld );

    setHAlignObj( inpfld );
}


bool uiFrequencyAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),Frequency::attribName()) )
	return false;

    mIfGetFloatInterval( Frequency::gateStr(), gate, gatefld->setValue(gate) );
    mIfGetBool( Frequency::normalizeStr(), normalize,
	        normfld->setValue(normalize) );
    mIfGetString( Frequency::windowStr(), window,
	        winfld->setWindowName(window) );
    mIfGetFloat( Frequency::paramvalStr(), variable,
	         winfld->setWindowParamValue(variable) );

    return true;
}


bool uiFrequencyAttrib::setInput( const Attrib::Desc& desc )
{
    putInp( inpfld, desc, 0 );
    return true;
}


bool uiFrequencyAttrib::setOutput( const Attrib::Desc& desc )
{
    outpfld->setValue( desc.selectedOutput() );
    return true;
}


bool uiFrequencyAttrib::getParameters( Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),Frequency::attribName()) )
	return false;

    mSetFloatInterval( Frequency::gateStr(), gatefld->getFInterval() );
    mSetBool( Frequency::normalizeStr(), normfld->getBoolValue() );
    mSetString( Frequency::windowStr(), winfld->windowName() );
    mSetFloat( Frequency::paramvalStr(), winfld->windowParamValue() );

    return true;
}


bool uiFrequencyAttrib::getInput( Attrib::Desc& desc )
{
    fillInp( inpfld, desc, 0 );
    return true;
}


bool uiFrequencyAttrib::getOutput( Attrib::Desc& desc )
{
    fillOutput( desc, outpfld->getIntValue() );
    return true;
}


void uiFrequencyAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( timegatestr, Frequency::gateStr() );
}
