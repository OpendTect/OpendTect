/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2004
-*/

static const char* rcsID = "$Id: parametriccurve.cc,v 1.1 2005-01-06 09:45:32 kristofer Exp $";

#include "parametriccurve.h"

#include "sets.h"

namespace Geometry
{

void ParametricCurve::getPosIDs( TypeSet<GeomPosID>& ids ) const
{
    ids.erase();
    const StepInterval<int> range = parameterRange();

    for ( int param=range.start; param<=range.stop; param += range.step )
	ids += param;
}


}; //Namespace

