/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initattributeengine.cc,v 1.7 2012-05-01 13:59:38 cvskris Exp $";

#include "moddepmgr.h"
#include "attribstorprovider.h"
#include "attribdescsettr.h"


mDefModInitFn(AttributeEngine)
{
    mIfNotFirstTime( return );

    AttribDescSetTranslatorGroup::initClass();
    dgbAttribDescSetTranslator::initClass();
    
    Attrib::StorageProvider::initClass();
}
