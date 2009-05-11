/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Apr 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: vispointset.cc,v 1.4 2009-05-11 04:46:21 cvsraman Exp $";

#include "datapointset.h"
#include "viscoord.h"
#include "vispointset.h"

#include "SoDGBIndexedPointSet.h"

#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>

mCreateFactoryEntry( visBase::PointSet );
mCreateFactoryEntry( visBase::IndexedPointSet );

namespace visBase
{

PointSet::PointSet()
    : VertexShape( new SoPointSet )
{ }


IndexedPointSet::IndexedPointSet()
    : IndexedShape( new SoDGBIndexedPointSet )
{
}

}; // namespace visBase
