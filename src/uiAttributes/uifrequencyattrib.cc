/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2001
________________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id$";


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


mInitAttribUI(uiFrequencyAttrib,Frequency,"Frequency",sKeyFreqGrp())


uiFrequencyAttrib::uiFrequencyAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,"101.0.6")

{
    inpfld = createImagInpFld( is2d );

    gatefld = new uiGenInput( this, gateLabel(), 
	    		      FloatInpIntervalSpec().setName("Z start",0)
			      			    .setName("Z stop",1) );
    gatefld->attach( alignedBelow, inpfld );

    normfld = new uiGenInput( this, "Normalize", BoolInpSpec(false) );
    normfld->attach( alignedBelow, gatefld );

    uiWindowFunctionSel::Setup su; su.label_ = "Window/Taper";
    su.winname_ = "CosTaper"; su.winparam_ = .05;
    winfld = new uiWindowFunctionSel( this, su );
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
	    	 const float resvar = float( mNINT32((1-variable)*1000) )/1000.0f;
	         winfld->setWindowParamValue(resvar) );

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
    const float resvar =
		float( mNINT32( (1-winfld->windowParamValue())*1000) )/1000.0f;
    mSetFloat( Frequency::paramvalStr(), resvar );

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
    params += EvalParam( timegatestr(), Frequency::gateStr() );
}


bool uiFrequencyAttrib::areUIParsOK()
{
    if ( !strcmp( winfld->windowName(), "CosTaper" ) )
    {
	float paramval = winfld->windowParamValue();
	if ( paramval<0 || paramval>1  )
	{
	    errmsg_ = "Variable 'Taper length' is not\n";
	    errmsg_ += "within the allowed range: 0 to 100 (%).";
	    return false;
	}
    }

    return true;
}
