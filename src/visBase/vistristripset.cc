/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "vistristripset.h"

#include <Inventor/nodes/SoIndexedTriangleStripSet.h>

mCreateFactoryEntry( visBase::TriangleStripSet );

namespace visBase
{

TriangleStripSet::TriangleStripSet()
    : IndexedShape( new SoIndexedTriangleStripSet )
{}

}; // namespace visBase
