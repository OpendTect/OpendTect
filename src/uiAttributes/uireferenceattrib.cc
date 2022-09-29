/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uireferenceattrib.h"
#include "referenceattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "survinfo.h"
#include "uiattribfactory.h"
#include "uigeninput.h"
#include "od_helpids.h"

using namespace Attrib;

static void getOutputNames( uiStringSet& strs3d, uiStringSet& strs2d )
{
    uiString zstr;
    if ( SI().zIsTime() )
	zstr = toUiString("%1 (s)").arg( uiStrings::sZ() );
    else
	zstr = uiStrings::sZ();

    strs3d.add( uiStrings::sX() ).add( uiStrings::sY() ).add( zstr )
	  .add( uiStrings::phrInline( od_static_tr("getOutputNames","number")) )
	  .add( uiStrings::phrCrossline( od_static_tr("getOutputNames",
						      "number")) )
	  .add( od_static_tr("getOutputNames", "Sample number") )
	  .add( uiStrings::phrInline( od_static_tr("getOutputNames", "index") ))
	  .add( uiStrings::phrCrossline( od_static_tr("getOutputNames",
						      "index" )) )
	  .add( od_static_tr("getOutputNames","Z index") );
    strs2d.add( uiStrings::sX() ).add( uiStrings::sY() ).add( zstr )
	  .add( uiStrings::sTraceNumber() )
	  .add( od_static_tr("getOutputNames", "Sample number") )
	  .add( od_static_tr("getOutputNames", "Trace index") );
}


mInitAttribUI(uiReferenceAttrib,Reference,"Reference",sKeyPositionGrp())


uiReferenceAttrib::uiReferenceAttrib( uiParent* p, bool is2d )
    : uiAttrDescEd(p,is2d,mODHelpKey(mReferenceAttribHelpID))
{
    uiStringSet outpstrs3d, outpstrs2d;
    getOutputNames( outpstrs3d, outpstrs2d );

    if ( is2d )
    {
	outpfld2d_ = new uiGenInput( this, tr("Desired Output"),
				     StringListInpSpec(outpstrs2d) );
	setHAlignObj( outpfld2d_ );
    }
    else
    {
	outpfld3d_ = new uiGenInput( this, tr("Desired Output"),
				     StringListInpSpec(outpstrs3d) );
	setHAlignObj( outpfld3d_ );
    }
}


uiReferenceAttrib::~uiReferenceAttrib()
{}


bool uiReferenceAttrib::setParameters( const Attrib::Desc& desc )
{
    return desc.attribName() == Reference::attribName();
}


bool uiReferenceAttrib::setInput( const Attrib::Desc& )
{
    return true;
}


bool uiReferenceAttrib::setOutput( const Desc& desc )
{
    uiGenInput* fld = is2d_ ? outpfld2d_ : outpfld3d_;
    if ( fld )
	fld->setValue( desc.selectedOutput() );

    return true;
}


bool uiReferenceAttrib::getOutput( Desc& desc )
{
    uiGenInput* fld = is2d_ ? outpfld2d_ : outpfld3d_;
    if ( fld )
	fillOutput( desc, fld->getIntValue() );

    return true;
}


bool uiReferenceAttrib::getParameters( Attrib::Desc& desc )
{
    return desc.attribName() == Reference::attribName();
}


bool uiReferenceAttrib::getInput( Attrib::Desc& )
{
    return true;
}
