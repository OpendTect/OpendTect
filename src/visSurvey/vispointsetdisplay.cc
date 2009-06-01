/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Satyaki Maitra
 * DATE     : March 2009
-*/

static const char* rcsID = "$Id: vispointsetdisplay.cc,v 1.4 2009-06-01 04:17:36 cvssatyaki Exp $";

#include "viscoord.h"
#include "vispointsetdisplay.h"

#include "datapointset.h"
#include "vispointset.h"
#include "vismaterial.h"


mCreateFactoryEntry( visSurvey::PointSetDisplay );


namespace visSurvey {


PointSetDisplay::PointSetDisplay()
    : VisualObjectImpl( true )
    , data_( * new DataPointSet(TypeSet<DataPointSet::DataRow>(),
				BufferStringSet(),false,true) )
{
    pointset_ = visBase::PointSet::create();
    addChild( pointset_->getInventorNode() );
}
    

PointSetDisplay::~PointSetDisplay()
{
    removeChild( pointset_->getInventorNode() );
}


void PointSetDisplay::setColor( Color col )
{ getMaterial()->setColor( col ); }


Color PointSetDisplay::getColor() const
{ return getMaterial()->getColor(); }


bool PointSetDisplay::setDataPack( const DataPointSet& dps )
{
    data_ = dps;
    pointset_->getCoordinates()->removeAfter(-1);
    getMaterial()->setColor( Color::DgbColor() );

    for ( int idx=0; idx<dps.size(); idx++ )
    {
	if ( dps.isSelected(idx) )
	    pointset_->getCoordinates()->addPos(
		    Coord3(dps.coord(idx),dps.z(idx)) );
    }

    return true;
}


void PointSetDisplay::setDisplayTransformation( visBase::Transformation* nt )
{
    pointset_->setDisplayTransformation( nt );
}


visBase::Transformation* PointSetDisplay::getDisplayTransformation()
{ return pointset_->getDisplayTransformation(); }

} //namespace visSurvey
