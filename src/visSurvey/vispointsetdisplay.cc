/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Satyaki Maitra
 * DATE     : March 2009
-*/

static const char* rcsID = "$Id: vispointsetdisplay.cc,v 1.1 2009-04-01 07:00:24 cvssatyaki Exp $";

#include "viscoord.h"
#include "vispointsetdisplay.h"

#include "datapointset.h"
#include "vispointset.h"
#include "vismaterial.h"


mCreateFactoryEntry( visSurvey::PointSetDisplay );


namespace visSurvey {


PointSetDisplay::PointSetDisplay()
    : VisualObjectImpl( true )
{
    pointset_ = visBase::IndexedPointSet::create();
    addChild( pointset_->getInventorNode() );
}
    

PointSetDisplay::~PointSetDisplay()
{
    removeChild( pointset_->getInventorNode() );
}


bool PointSetDisplay::setDataPackID( int attrib, const DataPack::ID dpid )
{
    DataPackMgr& dpman = DPM( DataPackMgr::PointID() );
    const DataPack* datapack = dpman.obtain( dpid );
    if ( !datapack ) return false;

    mDynamicCastGet(const DataPointSet*,dps,datapack);

    getMaterial()->setColor( Color::DgbColor() );

    for ( int idx=0; idx<dps->size(); idx++ )
    {
	if ( dps->isSelected(idx) )
	    pointset_->getCoordinates()->setPos( idx, Coord3(dps->coord(idx),
							dps->z(idx)) );
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
