/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2002
___________________________________________________________________

-*/

static const char* rcsID = "$Id: viscoord.cc,v 1.8 2003-09-10 10:00:08 kristofer Exp $";

#include "viscoord.h"

#include "thread.h"
#include "vistransform.h"

#include "Inventor/nodes/SoCoordinate3.h"

mCreateFactoryEntry( visBase::Coordinates );

visBase::Coordinates::Coordinates()
    : coords( new SoCoordinate3 )
    , mutex( *new Threads::Mutex )
    , transformation( 0 )
{
    coords->ref();
    unusedcoords += 0;
    //!<To compensate for that the first coord is set by default by OI
}


visBase::Coordinates::~Coordinates()
{
    coords->unref();
    delete &mutex;
    if ( transformation ) transformation->unRef();
}


void visBase::Coordinates::setTransformation( visBase::Transformation* nt )
{
    if ( nt==transformation ) return;

    Threads::MutexLocker lock( mutex );

    bool oldstatus = coords->point.enableNotify( false );

    if ( transformation )
    {
	const int nrcoords = coords->point.getNum();
	for ( int idx=0; idx<nrcoords; idx++ )
	{
	    SbVec3f scenepos = coords->point[idx];
	    Coord3 ownscenepos( scenepos[0], scenepos[1], scenepos[2] );
	    Coord3 worldpos = transformation->transformBack( ownscenepos );
	    coords->point.set1Value( idx, worldpos.x, worldpos.y, worldpos.z );
	}

	transformation->unRef();
	transformation = 0;
    }

    transformation = nt;

    if ( transformation )
    {
	transformation->ref();

	const int nrcoords = coords->point.getNum();
	for ( int idx=0; idx<nrcoords; idx++ )
	{
	    SbVec3f worldpos = coords->point[idx];
	    Coord3 ownworldpos( worldpos[0], worldpos[1], worldpos[2] );
	    Coord3 scenepos = transformation->transform( ownworldpos );
	    coords->point.set1Value( idx, scenepos.x, scenepos.y, scenepos.z );
	}
    }

    coords->point.enableNotify( oldstatus );
    coords->point.touch();
}


visBase::Transformation*  visBase::Coordinates::getTransformation()
{
    return transformation;
}


int visBase::Coordinates::size(bool includedeleted) const
{ return coords->point.getNum()-(includedeleted ? 0 : unusedcoords.size()); }


int visBase::Coordinates::addPos( const Coord3& pos )
{
    Threads::MutexLocker lock( mutex );
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

    if ( transformation )
    {
	Coord3 scenepos = transformation->transform( pos );
	coords->point.set1Value( res,SbVec3f(scenepos.x,scenepos.y,scenepos.z));
    }
    else 
	coords->point.set1Value( res, SbVec3f(pos.x,pos.y,pos.z) );

    return res;
}


Coord3 visBase::Coordinates::getPos( int idx, bool scenespace ) const
{
    SbVec3f scenepos = coords->point[idx];
    Coord3 res( scenepos[0], scenepos[1], scenepos[2] );

    if ( transformation && !scenespace )
	res = transformation->transformBack( res );

    return res;
}


void visBase::Coordinates::setPos( int idx, const Coord3& pos )
{
    Threads::MutexLocker lock( mutex );

    for ( int idy=coords->point.getNum(); idy<idx; idy++ )
	unusedcoords += idy;

    if ( transformation )
    {
	Coord3 scenepos = transformation->transform( pos );
	coords->point.set1Value( idx,SbVec3f(scenepos.x,scenepos.y,scenepos.z));
    }
    else 
	coords->point.set1Value( idx, SbVec3f(pos.x,pos.y,pos.z) );

    const int unusedidx = unusedcoords.indexOf(idx);
    if ( unusedidx!=-1 )
	unusedcoords.remove( unusedidx );
}


void visBase::Coordinates::removePos( int idx )
{
    Threads::MutexLocker lock( mutex );
    if ( idx==coords->point.getNum()-1 )
	coords->point.deleteValues( idx );
    else
	unusedcoords += idx;
}

void visBase::Coordinates::setAutoUpdate( bool update )
{
    bool oldvalue = coords->point.enableNotify( update );
    if ( update && !oldvalue ) coords->point.touch();
}    


bool visBase::Coordinates::autoUpdate()
{
    return coords->point.isNotifyEnabled();
}


void visBase::Coordinates::update()
{
    coords->point.touch();
}


SoNode* visBase::Coordinates::getData() { return coords; }
