/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: viscube.cc,v 1.1 2002-02-27 12:40:40 kristofer Exp $";

#include "viscube.h"

#include "Inventor/nodes/SoCube.h"
#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoTranslation.h"



visBase::Cube::Cube( visBase::Scene& s )
    : VisualObject( s )
    , cube( new SoCube )
    , position( new SoTranslation )
{
    root->addChild( position );
    root->addChild( cube );
}


void visBase::Cube::setCenterPos( float x, float y, float z )
{
    position->translation.setValue( x, y, z );
}


float visBase::Cube::centerPos( int dim ) const
{
    return position->translation.getValue()[dim];
}


void visBase::Cube::setWidth( float x, float y, float z )
{
    cube->width.setValue(x);
    cube->height.setValue(y);
    cube->depth.setValue(z);
}


float visBase::Cube::width( int dim ) const
{
    if ( !dim ) return cube->width.getValue();
    if ( dim==1 ) return cube->height.getValue();
    return cube->depth.getValue();
}
