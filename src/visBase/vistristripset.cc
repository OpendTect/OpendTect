/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          May 2002
 RCS:           $Id: vistristripset.cc,v 1.13 2005-02-07 12:45:40 nanne Exp $
________________________________________________________________________

-*/

#include "vistristripset.h"

#include <Inventor/nodes/SoIndexedTriangleStripSet.h>

mCreateFactoryEntry( visBase::TriangleStripSet );

namespace visBase
{

TriangleStripSet::TriangleStripSet()
    : IndexedShape( new SoIndexedTriangleStripSet )
{}

}; // namespace visBase
