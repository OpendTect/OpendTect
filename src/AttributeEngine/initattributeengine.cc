/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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
