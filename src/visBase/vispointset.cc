/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Apr 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "datapointset.h"
#include "viscoord.h"
#include "vispointset.h"
#include "visdrawstyle.h"

mCreateFactoryEntry( visBase::PointSet );

namespace visBase
{

PointSet::PointSet()
    : VertexShape( Geometry::PrimitiveSet::Points, true )
    , drawstyle_( new DrawStyle )
{
    drawstyle_->ref();
    drawstyle_->setPointSize( 5.0 );
    pErrMsg("Fix drawstyle.");
    //insertNode( drawstyle_->getInventorNode() );
}


void PointSet::setPointSize( int sz )
{
    drawstyle_->setPointSize( (float)sz );
}


int PointSet::getPointSize() const
{ return mNINT32(drawstyle_->getPointSize()); }


}; // namespace visBase
