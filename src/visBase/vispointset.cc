/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Apr 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: vispointset.cc,v 1.6 2009-10-21 06:18:56 cvssatyaki Exp $";

#include "datapointset.h"
#include "viscoord.h"
#include "vispointset.h"
#include "visdrawstyle.h"

#include "SoDGBIndexedPointSet.h"

#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>

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
    insertNode( drawstyle_->getInventorNode() );
}


int PointSet::getPointSize() const
{ return mNINT(drawstyle_->getPointSize()); }


IndexedPointSet::IndexedPointSet()
    : IndexedShape( new SoDGBIndexedPointSet )
{
}

}; // namespace visBase
