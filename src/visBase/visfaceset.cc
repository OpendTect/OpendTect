/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: visfaceset.cc,v 1.1 2003-01-09 09:41:43 kristofer Exp $";

#include "visfaceset.h"

#include "Inventor/nodes/SoIndexedFaceSet.h"

mCreateFactoryEntry( visBase::FaceSet );


visBase::FaceSet::FaceSet()
    : IndexedShape( new SoIndexedFaceSet )
{ }
