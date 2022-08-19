/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisamplevalueattrib.h"
#include "samplevalueattrib.h"

#include "uiattrsel.h"
#include "uiattribfactory.h"
#include "od_helpids.h"

using namespace Attrib;


mInitAttribUI(uiSampleValueAttrib,SampleValue,"Sample value",sKeyBasicGrp())

uiSampleValueAttrib::uiSampleValueAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d, mODHelpKey(mSampleValueAttribHelpID) )
{
    inpfld_ = createInpFld( is2d );
    setHAlignObj( inpfld_ );
}


bool uiSampleValueAttrib::setInput( const Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    return true;
}


bool uiSampleValueAttrib::getInput( Desc& desc )
{
    fillInp( inpfld_, desc, 0 );
    return true;
}
