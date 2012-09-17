/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Apr 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: vispointset.cc,v 1.11 2012/07/10 13:06:10 cvskris Exp $";

#include "datapointset.h"
#include "viscoord.h"
#include "vispointset.h"
#include "visdrawstyle.h"

#include <Inventor/nodes/SoPointSet.h>

#if COIN_MAJOR_VERSION<3 || (COIN_MAJOR_VERSION==3 && COIN_MINOR_VERSION<1)
# define USE_DGB_INDEXEDPOINTSET
#endif

#ifdef USE_DGB_INDEXEDPOINTSET
# include "SoDGBIndexedPointSet.h"
#else
# include "Inventor/nodes/SoIndexedPointSet.h"
#endif


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
{ return mNINT32(drawstyle_->getPointSize()); }


IndexedPointSet::IndexedPointSet()
#ifdef USE_DGB_INDEXEDPOINTSET
    : IndexedShape( new SoDGBIndexedPointSet )
#else
    : IndexedShape( new SoIndexedPointSet )
#endif
{
}

}; // namespace visBase
