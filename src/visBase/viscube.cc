/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: viscube.cc,v 1.9 2002-04-30 14:13:00 kristofer Exp $";

#include "viscube.h"
#include "geompos.h"
#include "iopar.h"

#include "Inventor/nodes/SoCube.h"
#include "Inventor/nodes/SoTranslation.h"

mCreateFactoryEntry( visBase::Cube );

const char* visBase::Cube::centerposstr = "Center Pos";
const char* visBase::Cube::widthstr = "Width";


visBase::Cube::Cube()
    : cube( new SoCube )
    , position( new SoTranslation )
{
    addChild( position );
    addChild( cube );
}


void visBase::Cube::setCenterPos( const Geometry::Pos& pos )
{
    position->translation.setValue( pos.x, pos.y, pos.z );
}


Geometry::Pos visBase::Cube::centerPos() const
{
    Geometry::Pos res;
    SbVec3f pos = position->translation.getValue();

    res.x = pos[0];
    res.y = pos[1];
    res.z = pos[2];

    return res;
}


void visBase::Cube::setWidth( const Geometry::Pos& n )
{
    cube->width.setValue(n.x);
    cube->height.setValue(n.y);
    cube->depth.setValue(n.z);
}


Geometry::Pos visBase::Cube::width() const
{
    Geometry::Pos res;
    
    res.x = cube->width.getValue();
    res.y = cube->height.getValue();
    res.z = cube->depth.getValue();

    return res;
}


int visBase::Cube::usePar( const IOPar& iopar )
{
    int res = VisualObjectImpl::usePar( iopar );
    if ( res != 1 ) return res;

    Geometry::Pos pos;
    if ( !iopar.get( centerposstr, pos.x, pos.y, pos.z ) )
	return -1;

    if ( !iopar.get( widthstr, pos.x, pos.y, pos.z ) )
	return -1;

    setCenterPos( pos );
    setWidth( pos );

    return 1;
}


void visBase::Cube::fillPar( IOPar& iopar, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( iopar, saveids );

    Geometry::Pos pos = centerPos();
    iopar.set( centerposstr, pos.x, pos.y, pos.z );

    pos = width();
    iopar.set( widthstr, pos.x, pos.y, pos.z );
}

