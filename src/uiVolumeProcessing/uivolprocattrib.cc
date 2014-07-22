/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2013
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uivolprocattrib.h"
#include "volprocattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "volproctrans.h"

#include "uiattribfactory.h"
#include "uiioobjsel.h"
#include "uidialog.h" // for mTODOHelpKey ... ugh


mInitAttribUI(uiVolProcAttrib,VolProcAttrib,"VolumeProcessing",sKeyBasicGrp())

uiVolProcAttrib::uiVolProcAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d,mTODOHelpKey)

{
    IOObjContext ctxt = VolProcessingTranslatorGroup::ioContext();
    ctxt.forread = true;
    setupfld_ = new uiIOObjSel( this, ctxt, "Volume Builder setup" );

    setHAlignObj( setupfld_ );
}


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

    mSetString( VolProcAttrib::sKeySetup(), setupfld_->key() );
    return true;
}

