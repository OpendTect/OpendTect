/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          May 2002
________________________________________________________________________

-*/
static const char* mUnusedVar rcsID = "$Id: vistristripset.cc,v 1.16 2012-05-02 11:54:11 cvskris Exp $";

#include "vistristripset.h"

#include <Inventor/nodes/SoIndexedTriangleStripSet.h>

mCreateFactoryEntry( visBase::TriangleStripSet );

namespace visBase
{

TriangleStripSet::TriangleStripSet()
    : IndexedShape( new SoIndexedTriangleStripSet )
{}

}; // namespace visBase
