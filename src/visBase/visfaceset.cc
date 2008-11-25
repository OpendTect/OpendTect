/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID = "$Id: visfaceset.cc,v 1.5 2008-11-25 15:35:27 cvsbert Exp $";

#include "visfaceset.h"

#include <Inventor/nodes/SoIndexedFaceSet.h>

mCreateFactoryEntry( visBase::FaceSet );

namespace visBase
{

FaceSet::FaceSet()
    : IndexedShape(new SoIndexedFaceSet)
{}

}; // namespace visBase
