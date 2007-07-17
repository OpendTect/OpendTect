/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2006
 RCS:           $Id: externalattrib.cc,v 1.2 2007-07-17 08:01:01 cvsnanne Exp $
________________________________________________________________________

-*/

#include "externalattrib.h"

namespace Attrib
{

ExtAttribCalc* ExtAttribCalcFact::createCalculator( const SelSpec& spec )
{
    for ( int idx=0; idx<creators_.size(); idx++)
    {
	ExtAttribCalc* res = creators_[idx]->make( spec );
	if ( res ) return res;
    }

    return 0;
}


ExtAttribCalcFact& ExtAttrFact()
{
    static ExtAttribCalcFact* inst = 0;
    if ( !inst ) inst = new ExtAttribCalcFact;
    return *inst;
}

} // namespace Attrib
