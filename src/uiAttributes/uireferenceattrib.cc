/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Payraudeau
 Date:          July 2005
________________________________________________________________________

-*/




#include "uireferenceattrib.h"
#include "referenceattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "survinfo.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
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
    : uiAttrDescEd(p,is2d, mODHelpKey(mReferenceAttribHelpID) )

{
    uiStringSet outpstrs3d, outpstrs2d;
    getOutputNames( outpstrs3d, outpstrs2d );
    inpfld = createInpFld( is2d );

    outpfld3d = new uiGenInput( this, tr("Desired Output"),
                                StringListInpSpec(outpstrs3d) );
    outpfld3d->attach( alignedBelow, inpfld );
    outpfld3d->display( !is2d_ );

    outpfld2d = new uiGenInput( this, tr("Desired Output"),
				StringListInpSpec(outpstrs2d) );
    outpfld2d->attach( alignedBelow, inpfld );
    outpfld2d->display( is2d_ );

    setHAlignObj( outpfld3d );
}


bool uiReferenceAttrib::setParameters( const Attrib::Desc& desc )
{
    return desc.attribName() == Reference::attribName();
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
    return desc.attribName() == Reference::attribName();
}


bool uiReferenceAttrib::getInput( Attrib::Desc& desc )
{
    fillInp( inpfld, desc, 0 );
    return true;
}
