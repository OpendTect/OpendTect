/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2002
___________________________________________________________________

-*/

static const char* rcsID = "$Id: viscoord.cc,v 1.13 2004-11-03 09:52:42 kristofer Exp $";

#include "viscoord.h"

#include "thread.h"
#include "vistransform.h"

#include "Inventor/nodes/SoCoordinate3.h"
#include "Inventor/nodes/SoGroup.h"
#include "UTMPosition.h"

mCreateFactoryEntry( visBase::Coordinates );

visBase::Coordinates::Coordinates()
    : coords( new SoCoordinate3 )
    , mutex( *new Threads::Mutex )
    , transformation( 0 )
    , utmposition( 0 )
    , root( new SoGroup )
{
    root->ref();
    root->addChild( coords );
    unusedcoords += 0;
    //!<To compensate for that the first coord is set by default by OI
}


visBase::Coordinates::~Coordinates()
{
    root->unref();
    delete &mutex;
    if ( transformation ) transformation->unRef();
}


void visBase::Coordinates::setTransformation( visBase::Transformation* nt )
{
    if ( nt==transformation ) return;

    Threads::MutexLocker lock( mutex );

    bool oldstatus = coords->point.enableNotify( false );
    TypeSet<Coord3> worldpos;

    if ( transformation )
    {
	const int nrcoords = coords->point.getNum();
	for ( int idx=0; idx<nrcoords; idx++ )
	    worldpos += getPos(idx);

	transformation->unRef();
	transformation = 0;
    }

    transformation = nt;

    if ( transformation )
	transformation->ref();

    const int nrcoords = coords->point.getNum();
    for ( int idx=0; idx<nrcoords; idx++ )
    {
	if ( unusedcoords.indexOf(idx)!=-1 )
	    continue;

	Coord3& pos = worldpos[idx];
	if ( pos.isDefined() )
	{
	    if ( transformation )
		pos = transformation->transform( pos );

	    if ( utmposition )
	    {
		SbVec3d utmoffset = utmposition->utmposition.getValue();
		pos.x -= utmoffset[0];
		pos.y -= utmoffset[1];
	    }
	}
	
	coords->point.set1Value( idx, pos.x, pos.y, pos.z );
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

    Coord3 postoset = pos;
    if ( postoset.isDefined() )
    {
	if ( transformation )
	    postoset = transformation->transform( postoset );

	if ( utmposition )
	{
	    SbVec3d utmoffset = utmposition->utmposition.getValue();
	    postoset.x -= utmoffset[0];
	    postoset.y -= utmoffset[1];
	}
    }
    
    coords->point.set1Value( res, SbVec3f(postoset.x,postoset.y,postoset.z) );

    return res;
}


Coord3 visBase::Coordinates::getPos( int idx, bool scenespace ) const
{
    SbVec3f scenepos = coords->point[idx];
    Coord3 res( scenepos[0], scenepos[1], scenepos[2] );
    if ( res.isDefined() )
    {
	if ( utmposition )
	{
	    SbVec3d utmoffset = utmposition->utmposition.getValue();
	    res.x += utmoffset[0];
	    res.y += utmoffset[1];
	}

	if ( transformation && !scenespace )
	    res = transformation->transformBack( res );
    }

    return res;
}


void visBase::Coordinates::setPos( int idx, const Coord3& pos )
{
    Threads::MutexLocker lock( mutex );

    for ( int idy=coords->point.getNum(); idy<idx; idy++ )
	unusedcoords += idy;

    Coord3 postoset = pos;
    if ( postoset.isDefined() )
    {
	if ( transformation )
	    postoset = transformation->transform( postoset );

	if ( !utmposition && !idx && !size(false) &&
		(postoset.x>1e5 || postoset.y>1e5) )
	{
	    utmposition = new UTMPosition;
	    utmposition->utmposition.setValue(
		    SbVec3d(postoset.x,postoset.y,0) );
	    root->insertChild( utmposition, 0 );
	}

	if ( utmposition )
	{
	    SbVec3d utmoffset = utmposition->utmposition.getValue();
	    postoset.x -= utmoffset[0];
	    postoset.y -= utmoffset[1];
	}
    }
    
    coords->point.set1Value( idx, SbVec3f(postoset.x,postoset.y,postoset.z) );

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

void visBase::Coordinates::setAutoUpdate( bool doupdate )
{
    bool oldvalue = coords->point.enableNotify( doupdate );
    if ( doupdate && !oldvalue ) coords->point.touch();
}    


bool visBase::Coordinates::autoUpdate()
{
    return coords->point.isNotifyEnabled();
}


void visBase::Coordinates::update()
{
    coords->point.touch();
}


SoNode* visBase::Coordinates::getInventorNode() { return root; }
