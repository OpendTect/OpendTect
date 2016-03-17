/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          May 2002
________________________________________________________________________

-*/

#include "vistristripset.h"

mCreateFactoryEntry( visBase::TriangleStripSet );

namespace visBase
{

TriangleStripSet::TriangleStripSet()
    : VertexShape( Geometry::PrimitiveSet::TriangleStrip, true )
{}

}; // namespace visBase
