/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: visfaceset.cc,v 1.3 2005-02-04 14:31:34 kristofer Exp $";

#include "visfaceset.h"

#include "Inventor/nodes/SoIndexedFaceSet.h"

namespace visBase
{

mCreateFactoryEntry( FaceSet );


FaceSet::FaceSet()
    : IndexedShape( new SoIndexedFaceSet )
{ }

}; // namespace visBase
