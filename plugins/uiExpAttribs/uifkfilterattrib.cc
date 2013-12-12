/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          December 2013
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uifkfilterattrib.h"
#include "fkfilterattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "uiattrsel.h"
#include "uigeninput.h"

using namespace Attrib;

mInitAttribUI(uiFKFilterAttrib,FKFilter,"FK Filter","Experimental" )

uiFKFilterAttrib::uiFKFilterAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d)
{
    inpfld_ = createInpFld( is2d, "Input" );

    setHAlignObj( inpfld_ );
}


bool uiFKFilterAttrib::setParameters( const Attrib::Desc& desc )
{
    if( desc.attribName() != FKFilter::attribName() )
	return false;
    return true;
}


bool uiFKFilterAttrib::setInput( const Attrib::Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    return true;
}


bool uiFKFilterAttrib::setOutput( const Attrib::Desc& desc )
{
    return true;
}


bool uiFKFilterAttrib::getParameters( Attrib::Desc& desc )
{
    if( desc.attribName() != FKFilter::attribName() )
	return false;
    return true;
}


bool uiFKFilterAttrib::getInput( Attrib::Desc& desc )
{
    inpfld_->processInput();
    fillInp( inpfld_, desc, 0 );
    return true;
}


bool uiFKFilterAttrib::getOutput( Attrib::Desc& desc )
{
    return true;
}
