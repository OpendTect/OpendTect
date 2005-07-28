/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          July 2005
 RCS:		$Id: uireferenceattrib.cc,v 1.1 2005-07-28 10:53:50 cvshelene Exp $
________________________________________________________________________

-*/


#include "uireferenceattrib.h"
#include "referenceattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "uiattrsel.h"
#include "uigeninput.h"

using namespace Attrib;

static const char* outpstrs[] =
{
    "X",
    "Y",
    "Z",
    "Inl",
    "Crl",
    "SampleNR",
    "Index Inl",
    "Index Crl",
    "Index Z",
    0
};

uiReferenceAttrib::uiReferenceAttrib( uiParent* p )
	: uiAttrDescEd(p)
{
    outpfld = new uiGenInput( this, "Desired Output",
				   StringListInpSpec(outpstrs) );
    setHAlignObj( outpfld );
}


bool uiReferenceAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),Reference::attribName()) )
	return false;

    return true;
}


bool uiReferenceAttrib::setOutput( const Desc& desc )
{
    outpfld->setValue( desc.selectedOutput() );
    return true;
}


bool uiReferenceAttrib::getOutput( Desc& desc )
{
    fillOutput( desc, outpfld->getIntValue() );
    return true;
}
