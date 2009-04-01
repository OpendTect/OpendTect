/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Apr 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: vispointset.cc,v 1.3 2009-04-01 07:02:03 cvssatyaki Exp $";

#include "datapointset.h"
#include "viscoord.h"
#include "vispointset.h"

#include "SoIndexedPointSet.h"

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
    : IndexedShape( new SoIndexedPointSet )
{
}

}; // namespace visBase
