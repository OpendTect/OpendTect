/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "moddepmgr.h"
#include "attribstorprovider.h"

mDefModInitFn(AttributeEngine)
{
    mIfNotFirstTime( return );

    Attrib::StorageProvider::initClass();
}
