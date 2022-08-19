/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vislines.h"

mCreateFactoryEntry( visBase::Lines );

namespace visBase
{

Lines::Lines()
    : VertexShape(Geometry::PrimitiveSet::Lines,true)
{}

} // namespace visBase
