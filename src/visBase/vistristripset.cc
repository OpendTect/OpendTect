/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vistristripset.h"

mCreateFactoryEntry( visBase::TriangleStripSet );

namespace visBase
{

TriangleStripSet::TriangleStripSet()
    : VertexShape( Geometry::PrimitiveSet::TriangleStrip, true )
{}


TriangleStripSet::~TriangleStripSet()
{}

} // namespace visBase
