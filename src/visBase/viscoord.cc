/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2002
 RCS:           $Id: viscoord.cc,v 1.20 2005-02-07 12:45:40 nanne Exp $
________________________________________________________________________

-*/

#include "viscoord.h"
#include "thread.h"
#include "vistransform.h"
#include "UTMPosition.h"

#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoGroup.h>

mCreateFactoryEntry( visBase::Coordinates );

namespace visBase
{

Coordinates::Coordinates()
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


Coordinates::~Coordinates()
{
    root->unref();
    delete &mutex;
    if ( transformation ) transformation->unRef();
}


void Coordinates::setDisplayTransformation( Transformation* nt )
{
    if ( nt==transformation ) return;

    Threads::MutexLocker lock( mutex );
    TypeSet<Coord3> worldpos;
    getPositions(worldpos);

    if ( transformation )
	transformation->unRef();

    transformation = nt;

    if ( transformation )
	transformation->ref();

    setPositions(worldpos);
}


Transformation*  Coordinates::getDisplayTransformation()
{
    return transformation;
}


void Coordinates::setLocalTranslation( const Coord& nc )
{
    Threads::MutexLocker lock( mutex );
    setLocalTranslationWithoutLock( nc );
}


void Coordinates::setLocalTranslationWithoutLock( const Coord& nc )
{
    TypeSet<Coord3> worldpos;
    getPositions(worldpos);

    if ( !utmposition )
    {
	utmposition = new UTMPosition;
	root->insertChild( utmposition, 0 );
    }

    Coord3 postoset( nc, 0 );
    if ( transformation )
	postoset = transformation->transform( postoset );

    utmposition->utmposition.setValue( SbVec3d(postoset.x,postoset.y,0) );

    setPositions(worldpos);
}


Coord Coordinates::getLocalTranslation() const
{
    if ( !utmposition ) return Coord(0,0);
    SbVec3d transl = utmposition->utmposition.getValue();
    Coord3 res( transl[0], transl[1], 0 );
    if ( transformation ) res = transformation->transformBack( res );
    return res;
}


int Coordinates::size(bool includedeleted) const
{ return coords->point.getNum()-(includedeleted ? 0 : unusedcoords.size()); }


int Coordinates::addPos( const Coord3& pos )
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


Coord3 Coordinates::getPos( int idx, bool scenespace ) const
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


void Coordinates::setPos( int idx, const Coord3& pos )
{
    Threads::MutexLocker lock( mutex );
    setPosWithoutLock(idx,pos);
}


void Coordinates::setPosWithoutLock( int idx, const Coord3& pos )
{
    for ( int idy=coords->point.getNum(); idy<idx; idy++ )
	unusedcoords += idy;

    Coord3 postoset = pos;
    if ( postoset.isDefined() )
    {
	if ( transformation )
	    postoset = transformation->transform( postoset );

	if ( !utmposition && !idx && !size(false) &&
		(fabs(postoset.x)>1e5 || fabs(postoset.y)>1e5) )
	    setLocalTranslationWithoutLock(postoset);

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


void Coordinates::removePos( int idx )
{
    Threads::MutexLocker lock( mutex );
    if ( idx==coords->point.getNum()-1 )
	coords->point.deleteValues( idx );
    else
	unusedcoords += idx;
}

void Coordinates::setAutoUpdate( bool doupdate )
{
    bool oldvalue = coords->point.enableNotify( doupdate );
    if ( doupdate && !oldvalue ) coords->point.touch();
}    


bool Coordinates::autoUpdate()
{
    return coords->point.isNotifyEnabled();
}


void Coordinates::update()
{
    coords->point.touch();
}


SoNode* Coordinates::getInventorNode() { return root; }


void Coordinates::getPositions(TypeSet<Coord3>& res) const
{
    for ( int idx=0; idx<coords->point.getNum(); idx++ )
	res += getPos(idx);
}


void Coordinates::setPositions( const TypeSet<Coord3>& pos)
{
    const bool oldstatus = coords->point.enableNotify( false );
    for ( int idx=0; idx<coords->point.getNum(); idx++ )
    {
	if ( unusedcoords.indexOf(idx)!=-1 )
	    continue;

	setPosWithoutLock(idx, pos[idx] );
    }

    coords->point.enableNotify( oldstatus );
    coords->point.touch();
}

};
