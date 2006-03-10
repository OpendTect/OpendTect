/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2001
 RCS:           $Id: uiinstantattrib.cc,v 1.3 2006-03-10 13:34:02 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiinstantattrib.h"
#include "instantattrib.h"
#include "attribdesc.h"
#include "uiattrsel.h"
#include "uigeninput.h"

using namespace Attrib;


const char* uiInstantaneousAttrib::outstrs[] =
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


uiInstantaneousAttrib::uiInstantaneousAttrib( uiParent* p )
	: uiAttrDescEd(p)
{
    inpfld = getImagInpFld();

    outpfld = new uiGenInput( this, "Output", StringListInpSpec(outstrs) );
    outpfld->setElemSzPol( uiObject::MedVar );
    outpfld->attach( alignedBelow, inpfld );

    setHAlignObj( inpfld );
}


bool uiInstantaneousAttrib::setParameters( const Desc& desc )
{
    return !strcmp(desc.attribName(),Instantaneous::attribName());
}


bool uiInstantaneousAttrib::setInput( const Desc& desc )
{
    putInp( inpfld, desc, 0 );
    return true;
}


bool uiInstantaneousAttrib::setOutput( const Desc& desc )
{
    outpfld->setValue( desc.selectedOutput() );
    return true;
}


bool uiInstantaneousAttrib::getParameters( Desc& desc )
{
    return !strcmp(desc.attribName(),Instantaneous::attribName());
}


bool uiInstantaneousAttrib::getInput( Desc& desc )
{
    fillInp( inpfld, desc, 0 );
    return true;
}


bool uiInstantaneousAttrib::getOutput( Desc& desc )
{
    fillOutput( desc, outpfld->getIntValue() );
    return true;
}
