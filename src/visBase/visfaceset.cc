/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Oct 1999
 RCS:           $Id: visfaceset.cc,v 1.4 2005-02-07 12:45:40 nanne Exp $
________________________________________________________________________

-*/

#include "visfaceset.h"

#include <Inventor/nodes/SoIndexedFaceSet.h>

mCreateFactoryEntry( visBase::FaceSet );

namespace visBase
{

FaceSet::FaceSet()
    : IndexedShape(new SoIndexedFaceSet)
{}

}; // namespace visBase
