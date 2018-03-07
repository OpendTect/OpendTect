/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct  2006
________________________________________________________________________

-*/

#include "uieventfreqattrib.h"
#include "eventfreqattrib.h"
#include "attribdesc.h"
#include "attribparam.h"
#include "uiattrsel.h"
#include "uigeninput.h"

using namespace Attrib;

static uiWord sDispName()
{
    return od_static_tr("sDispName","Event Frequency");
}

mInitGrpDefAttribUI(uiEventFreqAttrib,EventFreq,sDispName(),sExperimentalGrp())

uiEventFreqAttrib::uiEventFreqAttrib( uiParent* p, bool is2d )
: uiAttrDescEd( p, is2d, mODHelpKey( mEventFreqAttribHelpID ))
{
    inpfld_ = createInpFld( is2d );
    typfld_ = new uiGenInput( this, uiStrings::sOutput(),
			      BoolInpSpec(true,uiStrings::sFrequency(),
                                          uiStrings::sPhase(false)));
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


uiRetVal uiEventFreqAttrib::getInput( Attrib::Desc& desc )
{
    return fillInp( inpfld_, desc, 0 );
}


bool uiEventFreqAttrib::getOutput( Attrib::Desc& desc )
{
    fillOutput( desc, typfld_->getBoolValue() ? 0 : 1 );
    return true;
}
