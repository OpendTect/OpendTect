/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2012
________________________________________________________________________

-*/



#include "uisamplevalueattrib.h"
#include "samplevalueattrib.h"

#include "uiattrsel.h"
#include "uiattribfactory.h"
#include "od_helpids.h"

using namespace Attrib;

static uiWord sDispName()
{
    return od_static_tr("sDispName","Sample Value");
}


mInitAttribUI(uiSampleValueAttrib,SampleValue,sDispName(),sBasicGrp())

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


uiRetVal uiSampleValueAttrib::getInput( Desc& desc )
{
    return fillInp( inpfld_, desc, 0 );
}
