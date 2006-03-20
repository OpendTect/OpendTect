/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2006
 RCS:           $Id: externalattrib.cc,v 1.1 2006-03-20 07:44:48 cvsnanne Exp $
________________________________________________________________________

-*/

#include "externalattrib.h"

namespace Attrib
{

ExtAttribCalcFact& ExtAttrFact()
{
    static ExtAttribCalcFact* inst = 0;
    if ( !inst ) inst = new ExtAttribCalcFact;
    return *inst;
}

} // namespace Attrib
