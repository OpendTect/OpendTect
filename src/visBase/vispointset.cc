/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Apr 2002
 RCS:           $Id: vispointset.cc,v 1.1 2006-04-19 22:20:16 cvskris Exp $
________________________________________________________________________

-*/

#include "vispointset.h"

#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>

mCreateFactoryEntry( visBase::PointSet );

namespace visBase
{

PointSet::PointSet()
    : VertexShape( new SoPointSet )
{ }



}; // namespace visBase
