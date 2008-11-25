/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: vistristripset.cc,v 1.14 2008-11-25 15:35:27 cvsbert Exp $";

#include "vistristripset.h"

#include <Inventor/nodes/SoIndexedTriangleStripSet.h>

mCreateFactoryEntry( visBase::TriangleStripSet );

namespace visBase
{

TriangleStripSet::TriangleStripSet()
    : IndexedShape( new SoIndexedTriangleStripSet )
{}

}; // namespace visBase
