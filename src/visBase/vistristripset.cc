/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: vistristripset.cc,v 1.11 2003-11-07 12:22:03 bert Exp $";

#include "vistristripset.h"

#include "Inventor/nodes/SoIndexedTriangleStripSet.h"

mCreateFactoryEntry( visBase::TriangleStripSet );


visBase::TriangleStripSet::TriangleStripSet()
    : IndexedShape( new SoIndexedTriangleStripSet )
{ }
