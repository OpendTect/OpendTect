/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* mUnusedVar rcsID = "$Id: visfaceset.cc,v 1.7 2012-05-02 11:54:08 cvskris Exp $";

#include "visfaceset.h"

#include <Inventor/nodes/SoIndexedFaceSet.h>

mCreateFactoryEntry( visBase::FaceSet );

namespace visBase
{

FaceSet::FaceSet()
    : IndexedShape(new SoIndexedFaceSet)
{}

}; // namespace visBase
