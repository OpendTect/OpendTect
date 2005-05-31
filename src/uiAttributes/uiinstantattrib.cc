/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2001
 RCS:           $Id: uiinstantattrib.cc,v 1.1 2005-05-31 12:33:55 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiinstantattrib.h"
#include "instantattrib.h"
#include "attribdesc.h"
#include "uispecattrsel.h"
#include "uigeninput.h"

using namespace Attrib;


const char* uiInstantAttrib::outstrs[] =
{
	"Amplitude",
	"Phase",
	"Frequency",
    	"Hilbert",
	"Amplitude 1st derivative",
	"Amplitude 2nd derivative",
	"Cosine phase",
	"Envelope weighted phase",
	"Envelope weighted frequency",
	"Phase acceleration",
	"Thin bed indicator",
	"Bandwidth",
	"Q factor",
	0
};


uiInstantAttrib::uiInstantAttrib( uiParent* p )
	: uiAttrDescEd(p)
{
    inpfld = getImagInpFld();

    outpfld = new uiGenInput( this, "Output", StringListInpSpec(outstrs) );
    outpfld->setElemSzPol( uiObject::medvar );
    outpfld->attach( alignedBelow, inpfld );

    setHAlignObj( inpfld );
}


bool uiInstantAttrib::setParameters( const Desc& desc )
{
    return !strcmp(desc.attribName(),Instantaneous::attribName());
}


bool uiInstantAttrib::setInput( const Desc& desc )
{
    putInp( inpfld, desc, 0 );
    return true;
}


bool uiInstantAttrib::setOutput( const Desc& desc )
{
    outpfld->setValue( desc.selectedOutput() );
}


bool uiInstantAttrib::getParameters( Desc& desc )
{
    return !strcmp(desc.attribName(),Instantaneous::attribName());
}


bool uiInstantAttrib::getInput( Desc& desc )
{
    fillInp( inpfld, desc, 0 );
    return true;
}


bool uiInstantAttrib::getOutput( Desc& desc )
{
    fillOutput( desc, outpfld->getIntValue() );
    return true;
}
