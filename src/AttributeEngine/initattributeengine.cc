/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Satyaki
 Date:          December 2007
 RCS:           $Id: initattributeengine.cc,v 1.1 2007-12-14 05:10:16 cvssatyaki Exp $
        ________________________________________________________________________

	-*/

#include "initatrributeengine.h"
#include "attribstorprovider.h"

void AttributeEngine::initStdClasses()
{
    Attrib::StorageProvider::initClass();
}

