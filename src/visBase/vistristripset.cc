/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: vistristripset.cc,v 1.12 2005-02-04 14:31:34 kristofer Exp $";

#include "vistristripset.h"

#include "Inventor/nodes/SoIndexedTriangleStripSet.h"

namespace visBase
{

mCreateFactoryEntry( TriangleStripSet );


TriangleStripSet::TriangleStripSet()
    : IndexedShape( new SoIndexedTriangleStripSet )
{ }

}; // namespace visBase
