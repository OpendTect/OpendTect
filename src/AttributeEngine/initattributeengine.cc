/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initattributeengine.cc,v 1.5 2011-08-23 06:54:11 cvsbert Exp $";

#include "initattributeengine.h"
#include "attribstorprovider.h"

void AttributeEngine::initStdClasses()
{
    mIfNotFirstTime( return );

    Attrib::StorageProvider::initClass();
}

