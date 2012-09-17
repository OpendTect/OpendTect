/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID = "$Id: visfaceset.cc,v 1.6 2009/07/22 16:01:45 cvsbert Exp $";

#include "visfaceset.h"

#include <Inventor/nodes/SoIndexedFaceSet.h>

mCreateFactoryEntry( visBase::FaceSet );

namespace visBase
{

FaceSet::FaceSet()
    : IndexedShape(new SoIndexedFaceSet)
{}

}; // namespace visBase
