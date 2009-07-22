/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initattributeengine.cc,v 1.4 2009-07-22 16:01:30 cvsbert Exp $";

#include "initattributeengine.h"
#include "attribstorprovider.h"

void AttributeEngine::initStdClasses()
{
    Attrib::StorageProvider::initClass();
}

