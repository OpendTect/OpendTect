/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : May 2002
-*/

static const char* rcsID = "$Id: visfaceset.cc,v 1.2 2003-11-07 12:22:02 bert Exp $";

#include "visfaceset.h"

#include "Inventor/nodes/SoIndexedFaceSet.h"

mCreateFactoryEntry( visBase::FaceSet );


visBase::FaceSet::FaceSet()
    : IndexedShape( new SoIndexedFaceSet )
{ }
