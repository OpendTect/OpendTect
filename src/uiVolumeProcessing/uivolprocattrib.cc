/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2013
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

using namespace Attrib;

uiWord sDispName()
{
    return od_static_tr("sDispName","Volume Processing");
}

mInitAttribUI(uiVolProcAttrib,VolProcAttrib,sDispName(),sBasicGrp())

uiVolProcAttrib::uiVolProcAttrib( uiParent* p, bool is2d )
    : uiAttrDescEd(p, is2d, mODHelpKey(mVolProcAttribHelpID))

{
    IOObjContext ctxt = is2d ? VolProcessing2DTranslatorGroup::ioContext()
			     : VolProcessingTranslatorGroup::ioContext();
    ctxt.forread_ = true;
    setupfld_ = new uiIOObjSel( this, ctxt, tr("Volume Builder setup") );

    setHAlignObj( setupfld_ );
}


bool uiVolProcAttrib::setParameters( const Desc& desc )
{
    if ( desc.attribName() != VolProcAttrib::attribName() )
	return false;

    const ValParam* par = desc.getValParam( VolProcAttrib::sKeySetup() );
    if ( !par )
	return false;

    setupfld_->setInput( DBKey(par->getStringValue(0)) );
    return true;
}


bool uiVolProcAttrib::getParameters( Desc& desc )
{
    if ( desc.attribName() != VolProcAttrib::attribName() )
	return false;

    mSetString( VolProcAttrib::sKeySetup(), setupfld_->key().toString() );
    return true;
}
