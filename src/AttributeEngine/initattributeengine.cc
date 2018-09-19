/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          December 2007
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "attribstorprovider.h"
#include "attribdescsettr.h"
#include "attribprobelayer.h"

namespace Attrib
{
    extern void Make_Global_DescSet_Manager();
}


mDefModInitFn(AttributeEngine)
{
    mIfNotFirstTime( return );

    AttribDescSetTranslatorGroup::initClass();
    dgbAttribDescSetTranslator::initClass();

    Attrib::StorageProvider::initClass();
    AttribProbeLayer::initClass();

    Attrib::Make_Global_DescSet_Manager();
}
