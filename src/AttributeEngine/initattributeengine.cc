/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          December 2007
 RCS:           $Id: initattributeengine.cc,v 1.2 2007-12-14 05:52:55 cvsnanne Exp $
        ________________________________________________________________________

	-*/

#include "initattributeengine.h"
#include "attribstorprovider.h"

void AttributeEngine::initStdClasses()
{
    Attrib::StorageProvider::initClass();
}

