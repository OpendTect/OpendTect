/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Payraudeau
 Date:          July 2005
________________________________________________________________________

-*/

static const char* rcsID = "$Id$";



#include "uireferenceattrib.h"
#include "referenceattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"

using namespace Attrib;

static const char* outpstrs3d[] =
{
    "X",
    "Y",
    "Z",
    "Inline nr",
    "Crossline nr",
    "Sample nr",
    "Inline index",
    "Crossline index",
    "Z index",
    0
};

static const char* outpstrs2d[] =
{
    "X",
    "Y",
    "Z",
    "Trace nr",
    "Sample nr",
    "Trace index",
    "Z index",
    0
};


mInitAttribUI(uiReferenceAttrib,Reference,"Reference",sKeyPositionGrp())


uiReferenceAttrib::uiReferenceAttrib( uiParent* p, bool is2d )
    : uiAttrDescEd(p,is2d,"101.0.11")
    
{
    inpfld = createInpFld( is2d );
    
    outpfld3d = new uiGenInput( this, "Desired Output",
				   StringListInpSpec(outpstrs3d) );
    outpfld3d->attach( alignedBelow, inpfld );
    outpfld3d->display( !is2d_ );

    outpfld2d = new uiGenInput( this, "Desired Output",
				   StringListInpSpec(outpstrs2d) );
    outpfld2d->attach( alignedBelow, inpfld );
    outpfld2d->display( is2d_ );

    setHAlignObj( outpfld3d );
}


bool uiReferenceAttrib::setParameters( const Attrib::Desc& desc )
{
    return !strcmp(desc.attribName(),Reference::attribName());
}


bool uiReferenceAttrib::setInput( const Attrib::Desc& desc )
{
    putInp( inpfld, desc, 0 );
    return true;
}


bool uiReferenceAttrib::setOutput( const Desc& desc )
{
    is2d_ ? outpfld2d->setValue( desc.selectedOutput() ) :
	    outpfld3d->setValue( desc.selectedOutput() );
    return true;
}


bool uiReferenceAttrib::getOutput( Desc& desc )
{
    is2d_ ? fillOutput( desc, outpfld2d->getIntValue() ) :
	    fillOutput( desc, outpfld3d->getIntValue() );
    
    return true;
}


bool uiReferenceAttrib::getParameters( Attrib::Desc& desc )
{
    return !strcmp(desc.attribName(),Reference::attribName());
}


bool uiReferenceAttrib::getInput( Attrib::Desc& desc )
{
    fillInp( inpfld, desc, 0 );
    return true;
}
