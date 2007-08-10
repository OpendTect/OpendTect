/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Oct  2006
 RCS:           $Id: uieventfreqattrib.cc,v 1.2 2007-08-10 11:57:19 cvsbert Exp $
________________________________________________________________________

-*/

#include "uieventfreqattrib.h"
#include "eventfreqattrib.h"
#include "attribdesc.h"
#include "attribparam.h"
#include "uiattrsel.h"
#include "uigeninput.h"

using namespace Attrib;

mInitAttribUI(uiEventFreqAttrib,EventFreq,"Event Frequency",sKeyFreqGrp)

uiEventFreqAttrib::uiEventFreqAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d)
{
    inpfld_ = getInpFld( "Input" );
    typfld_ = new uiGenInput( this, "Output",
	    		      BoolInpSpec(true,"Frequency","Phase") );
    typfld_->attach( alignedBelow, inpfld_ );
    setHAlignObj( inpfld_ );
}


bool uiEventFreqAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),EventFreq::attribName()) )
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
    if ( strcmp(desc.attribName(),EventFreq::attribName()) )
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
