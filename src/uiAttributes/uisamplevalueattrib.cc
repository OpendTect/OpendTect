/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2012
________________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id: uisamplevalueattrib.cc,v 1.1 2012/09/05 14:11:07 cvsbert Exp $";


#include "uisamplevalueattrib.h"
#include "samplevalueattrib.h"

#include "uiattrsel.h"
#include "uiattribfactory.h"

using namespace Attrib;


mInitAttribUI(uiSampleValueAttrib,SampleValue,"Sample value",sKeyBasicGrp())

uiSampleValueAttrib::uiSampleValueAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,mTODOHelpID)
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
