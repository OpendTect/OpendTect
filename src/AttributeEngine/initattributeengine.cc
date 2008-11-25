/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: initattributeengine.cc,v 1.3 2008-11-25 15:35:21 cvsbert Exp $";

#include "initattributeengine.h"
#include "attribstorprovider.h"

void AttributeEngine::initStdClasses()
{
    Attrib::StorageProvider::initClass();
}

