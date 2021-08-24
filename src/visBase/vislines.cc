/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          October 2012
________________________________________________________________________

-*/

#include "vislines.h"

mCreateFactoryEntry( visBase::Lines );

namespace visBase
{

Lines::Lines()
    : VertexShape(Geometry::PrimitiveSet::Lines,true)
{}

} // namspace visBase
