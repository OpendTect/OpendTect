/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2002
___________________________________________________________________

-*/

static const char* rcsID = "$Id: vistexturecoords.cc,v 1.2 2003-01-08 15:17:32 kristofer Exp $";

#include "vistexturecoords.h"

#include "position.h"
#include "thread.h"

#include "Inventor/nodes/SoTextureCoordinate3.h"

mCreateFactoryEntry( visBase::TextureCoords );


visBase::TextureCoords::TextureCoords()
    : coords( new SoTextureCoordinate3 )
    , mutex( *new Threads::Mutex )
{
    coords->ref();
}


visBase::TextureCoords::~TextureCoords()
{
    coords->unref();
    delete &mutex;
}


void visBase::TextureCoords::setCoord( int idx,  const Coord3& pos )
{
    Threads::MutexLocker lock( mutex );
    if ( idx>=coords->point.getNum() )
	return;

    coords->point.set1Value( idx, SbVec3f( pos.x, pos.y, pos.z ));
}


void visBase::TextureCoords::setCoord( int idx,  const Coord& pos )
{
    Threads::MutexLocker lock( mutex );
    if ( idx>=coords->point.getNum() )
	return;

    coords->point.set1Value( idx, SbVec3f( pos.x, pos.y, 0 ));
}


int visBase::TextureCoords::addCoord( const Coord3& pos )
{
    Threads::MutexLocker lock( mutex );
    const int res = getFreeIdx();
    coords->point.set1Value( res, SbVec3f( pos.x, pos.y, pos.z ));

    return res;
}


int visBase::TextureCoords::addCoord( const Coord& pos )
{
    Threads::MutexLocker lock( mutex );
    const int res = getFreeIdx();
    coords->point.set1Value( res, SbVec3f( pos.x, pos.y, 0 ));

    return res;
}


void visBase::TextureCoords::removeCoord(int idx)
{
    Threads::MutexLocker lock( mutex );
    if ( idx==coords->point.getNum()-1 )
    {
	coords->point.deleteValues( idx );
    }
    else
    {
	unusedcoords += idx;
    }
}


SoNode* visBase::TextureCoords::getData()
{ return coords; }


int  visBase::TextureCoords::getFreeIdx()
{
    if ( unusedcoords.size() )
    {
	const int res = unusedcoords[unusedcoords.size()-1];
	unusedcoords.remove(unusedcoords.size()-1);
	return res;
    }

    return coords->point.getNum();
}
