/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vistristripset.cc,v 1.4 2002-04-26 13:00:09 kristofer Exp $";

#include "vistristripset.h"
#include "geomposlist.h"
#include "geomtristripset.h"

#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoCoordinate3.h"
#include "Inventor/nodes/SoIndexedTriangleStripSet.h"

mCreateFactoryEntry( visBase::TriangleStripSet );


visBase::TriangleStripSet::TriangleStripSet()
    : coords( new SoCoordinate3 )
    , strips( new SoIndexedTriangleStripSet )
{
    addChild( coords );
    addChild( strips );
}


visBase::TriangleStripSet::~TriangleStripSet()
{
    if ( stripset )
    {
	stripset->poslist.removeNotification(
				mCB( this, TriangleStripSet, updateCoords ));
	stripset->indexchnotifier.remove(
				mCB( this, TriangleStripSet, updateIndexes ));
    }
}


void visBase::TriangleStripSet::setStrips( Geometry::TriangleStripSet* ns, 
					   bool connect )
{
    if ( stripset )
    {
	stripset->poslist.removeNotification(
				mCB( this, TriangleStripSet, updateCoords ));
	stripset->indexchnotifier.remove(
				mCB( this, TriangleStripSet, updateIndexes ));
    }

    stripset = ns;
    updateCoords();
    updateIndexes();

    if ( !connect ) stripset = 0;
    else
    {
	stripset->poslist.notify(mCB( this, TriangleStripSet, updateCoords ));
	stripset->indexchnotifier.notify(
				 mCB( this, TriangleStripSet, updateIndexes ));
    }
}


void visBase::TriangleStripSet::updateCoords( CallBacker* )
{
    coords->point.setValues( 0, stripset->poslist.highestId()+1,
		    (const float (*)[3]) stripset->poslist.getCoords( 0 ) );
}


void visBase::TriangleStripSet::updateIndexes( CallBacker* )
{
    strips->coordIndex.setValues( 0, stripset->getNrIds(), stripset->getIds() );
}
