/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2002
___________________________________________________________________

-*/

static const char* rcsID = "$Id: viscoord.cc,v 1.2 2003-01-07 10:19:58 kristofer Exp $";

#include "viscoord.h"

#include "Inventor/nodes/SoCoordinate3.h"

mCreateFactoryEntry( visBase::Coordinates );

visBase::Coordinates::Coordinates()
    : coords( new SoCoordinate3 )
{
    coords->ref();
}


visBase::Coordinates::~Coordinates()
{
    coords->unref();
}


int visBase::Coordinates::size(bool includedeleted) const
{ return coords->point.getNum()-(includedeleted ? unusedcoords.size() : 0); }


int visBase::Coordinates::addPos( const Coord3& pos )
{
    int res;
    const int nrunused = unusedcoords.size();
    if ( unusedcoords.size() )
    {
	res = unusedcoords[nrunused-1];
	unusedcoords.remove( nrunused-1 );
    }
    else
    {
	res = coords->point.getNum();
    }

    coords->point.set1Value( res, SbVec3f( pos.x, pos.y, pos.z ) );
    return res;
}


void visBase::Coordinates::setPos( int idx, const Coord3& pos )
{
    int idy = coords->point.getNum();
    while ( idx>idy )
    {
	unusedcoords += idy;
	idy++;
    }

    coords->point.set1Value( idx, pos.x, pos.y, pos.z );
}


void visBase::Coordinates::removePos( int idx )
{
    if ( idx==coords->point.getNum()-1 )
    {
	coords->point.deleteValues( idx );
    }
    else
	unusedcoords += idx;
}

SoNode* visBase::Coordinates::getData() { return coords; }
