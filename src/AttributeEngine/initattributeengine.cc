/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initattributeengine.cc,v 1.6 2011/08/23 14:51:33 cvsbert Exp $";

#include "moddepmgr.h"
#include "attribstorprovider.h"

mDefModInitFn(AttributeEngine)
{
    mIfNotFirstTime( return );

    Attrib::StorageProvider::initClass();
}
