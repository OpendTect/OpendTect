/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: vistristripset.cc,v 1.10 2003-01-07 10:27:35 kristofer Exp $";

#include "vistristripset.h"

#include "Inventor/nodes/SoIndexedTriangleStripSet.h"

mCreateFactoryEntry( visBase::TriangleStripSet );


visBase::TriangleStripSet::TriangleStripSet()
    : IndexedShape( new SoIndexedTriangleStripSet )
{ }
