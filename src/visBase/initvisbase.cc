/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUsedVar = "$Id$";



#include "moddepmgr.h"

#include "visshape.h"
#include "indexedshape.h"


mDefModInitFn(visBase)
{
    mIfNotFirstTime( return );

    Geometry::PrimitiveSetCreator::setCreator(
				    new visBase::PrimitiveSetCreator );
}
