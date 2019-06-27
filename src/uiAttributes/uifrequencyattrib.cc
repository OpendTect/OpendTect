/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2001
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
#include "od_helpids.h"

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
	nullptr
};


mInitAttribUI(uiFrequencyAttrib,Frequency,uiStrings::sFrequency(),sFreqGrp())


uiFrequencyAttrib::uiFrequencyAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d, mODHelpKey(mFrequencyAttribHelpID) )

{
    inpfld_ = createImagInpFld( is2d );

    gatefld_ = new uiGenInput( this, gateLabel(),
			      FloatInpIntervalSpec().setName("Z start",0)
						    .setName("Z stop",1) );
    gatefld_->attach( alignedBelow, inpfld_ );

    normfld_ = new uiGenInput( this, uiStrings::sNormalize(),
				BoolInpSpec(false) );
    normfld_->attach( alignedBelow, gatefld_ );

    uiWindowFunctionSel::Setup su; su.label_ = "Window/Taper";
    su.winname_ = "CosTaper"; su.winparam_ = .05f;
    winfld_ = new uiWindowFunctionSel( this, su );
    winfld_->attach( alignedBelow, normfld_ );

    smoothspectrumfld_ = new uiGenInput( this, tr("Smooth Spectrum"),
					 BoolInpSpec(false) );
    smoothspectrumfld_->attach( alignedBelow, winfld_ );

    outpfld_ = new uiGenInput( this, uiStrings::sOutput(),
			      StringListInpSpec(outpstrs) );
    outpfld_->setElemSzPol( uiObject::WideVar );
    outpfld_->attach( alignedBelow, smoothspectrumfld_ );

    setHAlignObj( inpfld_ );
}


bool uiFrequencyAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( desc.attribName() != Frequency::attribName() )
	return false;

    mIfGetFloatInterval( Frequency::gateStr(), gate, gatefld_->setValue(gate) );
    mIfGetBool( Frequency::normalizeStr(), normalize,
		normfld_->setValue(normalize) );
    mIfGetString( Frequency::windowStr(), window,
		winfld_->setWindowName(window) );
    mIfGetFloat( Frequency::paramvalStr(), variable,
	   const float resvar = float( mNINT32((1-variable)*1000) )/1000.0f;
	   winfld_->setWindowParamValue(resvar) );

    mIfGetBool( Frequency::smoothspectrumStr(), sspec,
		smoothspectrumfld_->setValue(sspec) );

    return true;
}


bool uiFrequencyAttrib::setInput( const Attrib::Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    return true;
}


bool uiFrequencyAttrib::setOutput( const Attrib::Desc& desc )
{
    outpfld_->setValue( desc.selectedOutput() );
    return true;
}


bool uiFrequencyAttrib::getParameters( Attrib::Desc& desc )
{
    if ( desc.attribName() != Frequency::attribName() )
	return false;

    mSetFloatInterval( Frequency::gateStr(), gatefld_->getFInterval() );
    mSetBool( Frequency::normalizeStr(), normfld_->getBoolValue() );
    mSetBool( Frequency::smoothspectrumStr(),
	      smoothspectrumfld_->getBoolValue() );
    mSetString( Frequency::windowStr(), winfld_->windowName() );
    const float resvar =
		float( mNINT32( (1-winfld_->windowParamValue())*1000) )/1000.0f;
    mSetFloat( Frequency::paramvalStr(), resvar );

    return true;
}


uiRetVal uiFrequencyAttrib::getInput( Attrib::Desc& desc )
{
    return fillInp( inpfld_, desc, 0 );
}


bool uiFrequencyAttrib::getOutput( Attrib::Desc& desc )
{
    fillOutput( desc, outpfld_->getIntValue() );
    return true;
}


void uiFrequencyAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    params += EvalParam( timegatestr(), Frequency::gateStr() );
}


uiRetVal uiFrequencyAttrib::areUIParsOK()
{
    if ( FixedString(winfld_->windowName()) == "CosTaper" )
    {
	float paramval = winfld_->windowParamValue();
	if ( paramval<0 || paramval>1  )
	    return uiRetVal( tr("Variable 'Taper length' is not\n"
			   "within the allowed range: 0 to 100 (%).") );
    }

    return uiRetVal::OK();
}
