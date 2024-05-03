/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uieventfreqattrib.h"
#include "eventfreqattrib.h"
#include "attribdesc.h"
#include "attribparam.h"
#include "uiattrsel.h"
#include "uigeninput.h"

using namespace Attrib;

mInitAttribUI(uiEventFreqAttrib,EventFreq,"Event Frequency","Experimental" )

uiEventFreqAttrib::uiEventFreqAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d, mODHelpKey(mEventFreqAttribHelpID))
{
    inpfld_ = createInpFld( is2d, uiStrings::sInput() );
    typfld_ = new uiGenInput( this, uiStrings::sOutput(),
			      BoolInpSpec(true,uiStrings::sFrequency(),
					  uiStrings::sPhase()) );
    typfld_->attach( alignedBelow, inpfld_ );
    setHAlignObj( inpfld_ );
}


bool uiEventFreqAttrib::setParameters( const Attrib::Desc& desc )
{
    if( desc.attribName() != EventFreq::attribName() )
	return false;
    return true;
}


bool uiEventFreqAttrib::setInput( const Attrib::Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    return true;
}


bool uiEventFreqAttrib::setOutput( const Attrib::Desc& desc )
{
    typfld_->setValue( desc.selectedOutput() == 0 );
    return true;
}


bool uiEventFreqAttrib::getParameters( Attrib::Desc& desc )
{
    if( desc.attribName() != EventFreq::attribName() )
	return false;
    return true;
}


bool uiEventFreqAttrib::getInput( Attrib::Desc& desc )
{
    inpfld_->processInput();
    fillInp( inpfld_, desc, 0 );
    return true;
}


bool uiEventFreqAttrib::getOutput( Attrib::Desc& desc )
{
    fillOutput( desc, typfld_->getBoolValue() ? 0 : 1 );
    return true;
}
