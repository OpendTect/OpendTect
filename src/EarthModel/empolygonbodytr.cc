/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          May 2002
 RCS:           $Id: empolygonbodytr.cc,v 1.1 2008-09-08 17:41:28 cvskris Exp $
________________________________________________________________________

-*/

#include "embodytr.h"
#include "empolygonbody.h"


polygonEMBodyTranslator::polygonEMBodyTranslator( const char* unm,
						  const char* nm )
    : Translator( unm, nm )
{}


polygonEMBodyTranslator::~polygonEMBodyTranslator()
{}


const char* polygonEMBodyTranslator::sKeyUserName()
{
    return "PolygonBody";
}
