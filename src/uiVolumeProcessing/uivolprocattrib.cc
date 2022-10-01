/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivolprocattrib.h"
#include "volprocattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "od_helpids.h"
#include "volproctrans.h"

#include "uiattribfactory.h"
#include "uiioobjsel.h"


mInitAttribUI(uiVolProcAttrib,VolProcAttrib,"VolumeProcessing",sKeyBasicGrp())

uiVolProcAttrib::uiVolProcAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,mODHelpKey(mVolProcAttribHelpID))

{
    IOObjContext ctxt = is2d ? VolProcessing2DTranslatorGroup::ioContext()
			     : VolProcessingTranslatorGroup::ioContext();
    ctxt.forread_ = true;
    setupfld_ = new uiIOObjSel( this, ctxt, tr("Volume Builder setup") );

    setHAlignObj( setupfld_ );
}


uiVolProcAttrib::~uiVolProcAttrib()
{}


bool uiVolProcAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( desc.attribName() != VolProcAttrib::attribName() )
	return false;

    const ValParam* par = desc.getValParam( VolProcAttrib::sKeySetup() );
    if ( !par ) return false;

    const MultiID mid( par->getStringValue(0) );
    setupfld_->setInput( mid );

    return true;
}


bool uiVolProcAttrib::getParameters( Attrib::Desc& desc )
{
    if ( desc.attribName() != VolProcAttrib::attribName() )
	return false;

    mSetMultiID( VolProcAttrib::sKeySetup(), setupfld_->key() );
    return true;
}
