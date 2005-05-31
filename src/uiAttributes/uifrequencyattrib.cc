/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2001
 RCS:           $Id: uifrequencyattrib.cc,v 1.1 2005-05-31 12:33:55 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uifrequencyattrib.h"
#include "frequencyattrib.h"
#include "attribdesc.h"
#include "uispecattrsel.h"
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


uiFrequencyAttrDescEd::uiFrequencyAttrDescEd( uiParent* p )
	: uiAttrDescEd(p)
{
    inpfld = getImagInpFld();

    gatefld = new uiGenInput( this, gateLabel(), FloatInpIntervalSpec() );
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


bool uiFrequency::setParameters( const Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),Frequency::attribName()) )
	return false;

    mIfGetFloatInterval( Frequency::gateStr(), gate, gatefld->setValue(gate) );
    mIfGetBool( Frequency::normalizeStr(), normalize,
	        normfld->setValue(normalize) );
    mIfGetEnum( Frequency::windowStr(), window, winfld->setValue(window) );

    return true;
}


bool uiFrequency::setInput( const Attrib::Desc& desc )
{
    putInp( inpfld, desc, 0 );
    return true;
}


bool uiFrequency::setOutput( const Attrib::Desc& desc )
{
    outpfld->setValue( desc.selectedOutput() );
    return true;
}


bool uiFrequency::getParameters( Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),Frequency::attribName()) )
	return false;

    mSetFloatInterval( Frequency::gateStr(), gatefld->getFInterval() );
    mSetBool( Frequency::normalizeStr(), normfld->getIntValue() );
    mSetEnum( Frequency::windowStr(), winfld->getIntValue() );

    return true;
}


bool uiFrequency::getInput( Attrib::Desc& desc )
{
    fillInp( inpfld, desc, 0 );
    return true;
}


bool uiFrequency::getOutput( Attrib::Desc& desc )
{
    mChgTrackGetSet(chtr,ad,selectedOutput,selectOutput,outpfld->getIntValue());
    return true;
}
