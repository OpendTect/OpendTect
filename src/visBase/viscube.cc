/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: viscube.cc,v 1.13 2003-09-22 09:10:55 kristofer Exp $";

#include "viscube.h"
#include "iopar.h"

#include "Inventor/nodes/SoCube.h"
#include "Inventor/nodes/SoTranslation.h"

mCreateFactoryEntry( visBase::Cube );

const char* visBase::Cube::centerposstr = "Center Pos";
const char* visBase::Cube::widthstr = "Width";


visBase::Cube::Cube()
    : Shape( new SoCube )
    , position( new SoTranslation )
{
    insertNode( position );
}


void visBase::Cube::setCenterPos( const Coord3& pos )
{
    position->translation.setValue( pos.x, pos.y, pos.z );
}


Coord3 visBase::Cube::centerPos() const
{
    Coord3 res;
    SbVec3f pos = position->translation.getValue();

    res.x = pos[0];
    res.y = pos[1];
    res.z = pos[2];

    return res;
}


void visBase::Cube::setWidth( const Coord3& n )
{
    SoCube* cube = reinterpret_cast<SoCube*>( shape );
    cube->width.setValue(n.x);
    cube->height.setValue(n.y);
    cube->depth.setValue(n.z);
}


Coord3 visBase::Cube::width() const
{
    Coord3 res;
    
    SoCube* cube = reinterpret_cast<SoCube*>( shape );
    res.x = cube->width.getValue();
    res.y = cube->height.getValue();
    res.z = cube->depth.getValue();

    return res;
}


int visBase::Cube::usePar( const IOPar& iopar )
{
    int res = Shape::usePar( iopar );
    if ( res != 1 ) return res;

    Coord3 pos;
    if ( !iopar.get( centerposstr, pos.x, pos.y, pos.z ) )
	return -1;

    setCenterPos( pos );

    if ( !iopar.get( widthstr, pos.x, pos.y, pos.z ) )
	return -1;

    setWidth( pos );

    return 1;
}


void visBase::Cube::fillPar( IOPar& iopar, TypeSet<int>& saveids ) const
{
    Shape::fillPar( iopar, saveids );

    Coord3 pos = centerPos();
    iopar.set( centerposstr, pos.x, pos.y, pos.z );

    pos = width();
    iopar.set( widthstr, pos.x, pos.y, pos.z );
}

