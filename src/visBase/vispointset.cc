/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Apr 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: vispointset.cc,v 1.9 2011-04-20 12:50:01 cvskris Exp $";

#include "datapointset.h"
#include "viscoord.h"
#include "vispointset.h"
#include "visdrawstyle.h"

#if COIN_MAJOR_VERSION >= 3 && COIN_MINOR_VERSION <1
#define USE_DGB_INDEXEDPOINTSET
#endif

#ifdef USE_DGB_INDEXEDPOINTSET
#include "SoDGBIndexedPointSet.h"
#else
#include "Inventor/nodes/SoIndexedPointSet.h"
#endif

#include <Inventor/nodes/SoPointSet.h>

mCreateFactoryEntry( visBase::PointSet );
mCreateFactoryEntry( visBase::IndexedPointSet );

namespace visBase
{

PointSet::PointSet()
    : VertexShape( new SoPointSet )
    , drawstyle_( DrawStyle::create() )
{
    drawstyle_->setPointSize( 5.0 );
    insertNode( drawstyle_->getInventorNode() );
}


void PointSet::setPointSize( int sz )
{
    drawstyle_->setPointSize( (float)sz );
}


int PointSet::getPointSize() const
{ return mNINT(drawstyle_->getPointSize()); }


IndexedPointSet::IndexedPointSet()
#ifdef USE_DGB_INDEXEDPOINTSET
    : IndexedShape( new SoDGBIndexedPointSet )
#else
    : IndexedShape( new SoIndexedPointSet )
#endif
{
}

}; // namespace visBase
