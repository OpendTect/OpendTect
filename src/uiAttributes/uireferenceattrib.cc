/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          July 2005
 RCS:		$Id: uireferenceattrib.cc,v 1.4 2005-09-26 14:02:51 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uireferenceattrib.h"
#include "referenceattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
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


uiReferenceAttrib::uiReferenceAttrib( uiParent* p )
    : uiAttrDescEd(p)
    , is2d_(false)
{
    outpfld3d = new uiGenInput( this, "Desired Output",
				   StringListInpSpec(outpstrs3d) );

    outpfld2d = new uiGenInput( this, "Desired Output",
				   StringListInpSpec(outpstrs2d) );
    outpfld2d->display(false);

    setHAlignObj( outpfld3d );
}


void uiReferenceAttrib::set2D( bool yn )
{
    outpfld3d->display( !yn );
    outpfld2d->display( yn );
    is2d_ = yn;
}


bool uiReferenceAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( strcmp(desc.attribName(),Reference::attribName()) )
	return false;

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
    mSetBool( Reference::is2DStr(), is2d_ );
    return true;
}
