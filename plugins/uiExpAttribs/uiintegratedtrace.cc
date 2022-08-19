/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiintegratedtrace.h"
#include "integratedtrace.h"

#include "uiattrsel.h"

using namespace Attrib;


mInitAttribUI(uiIntegratedTrace,IntegratedTrace,
		"Integrated Trace","Experimental")

uiIntegratedTrace::uiIntegratedTrace( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d, mNoHelpKey )
{
    inpfld_ = createInpFld( is2d );
    setHAlignObj( inpfld_ );
}


bool uiIntegratedTrace::setInput( const Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    return true;
}


bool uiIntegratedTrace::getInput( Desc& desc )
{
    fillInp( inpfld_, desc, 0 );
    return true;
}
