/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Apr 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: vispointset.cc,v 1.5 2009-07-22 16:01:45 cvsbert Exp $";

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
