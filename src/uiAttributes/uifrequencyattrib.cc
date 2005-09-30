/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2001
 RCS:           $Id: uifrequencyattrib.cc,v 1.5 2005-09-30 15:45:13 cvshelene Exp $
________________________________________________________________________

-*/

#include "uifrequencyattrib.h"
#include "frequencyattrib.h"
#include "attribdesc.h"
#include "attribparam.h"
#include "uiattrsel.h"
#include "uigeninput.h"

using namespace Attrib;

static const char* winstrs[] =
{
	"None", 
	"Hamming", 
	"Hanning", 
	"Blackman", 
	"Bartlett",
	"CosTaper5", 
	"CosTaper10", 
	"CosTaper20", 
	0 
};

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


uiFrequencyAttrib::uiFrequencyAttrib( uiParent* p )
	: uiAttrDescEd(p)
{
    inpfld = getImagInpFld();

    gatefld = new uiGenInput( this, gateLabel(), FloatInpIntervalSpec() );
    gatefld->setValues( -28, 28 );
    gatefld->attach( alignedBelow, inpfld );

    normfld = new uiGenInput( this, "Normalize", BoolInpSpec("Yes","No") );
    normfld->attach( alignedBelow, gatefld );

    winfld = new uiGenInput( this, "Window/Taper", StringListInpSpec(winstrs) );
    winfld->setElemSzPol( uiObject::medvar );
    winfld->attach( alignedBelow, normfld ); 

    outpfld = new uiGenInput( this, "Output", StringListInpSpec(outpstrs) );
    outpfld->setElemSzPol( uiObject::widevar );
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
    mIfGetEnum( Frequency::windowStr(), window, winfld->setValue(window) );

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
    mSetEnum( Frequency::windowStr(), winfld->getIntValue() );

    return true;
}


bool uiFrequencyAttrib::getInput( Attrib::Desc& desc )
{
    fillInp( inpfld, desc, 0 );
    return true;
}


bool uiFrequencyAttrib::getOutput( Attrib::Desc& desc )
{
    mChgTrackGetSet( chtr, (&desc), selectedOutput,
	    	selectOutput, outpfld->getIntValue() );
    return true;
}


void uiFrequencyAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( timegatestr, Frequency::gateStr() );
}
