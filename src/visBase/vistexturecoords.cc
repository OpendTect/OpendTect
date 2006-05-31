/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2002
 RCS:           $Id: vistexturecoords.cc,v 1.10 2006-05-31 12:42:53 cvskris Exp $
________________________________________________________________________

-*/

#include "vistexturecoords.h"

#include "errh.h"
#include "position.h"
#include "thread.h"

#include <Inventor/nodes/SoTextureCoordinate3.h>

mCreateFactoryEntry( visBase::TextureCoords );

namespace visBase
{

TextureCoords::TextureCoords()
    : coords( new SoTextureCoordinate3 )
    , mutex( *new Threads::Mutex )
{
    coords->ref();
    unusedcoords += 0;
    //!<To compensate for that the first coord is set by default by coin
}


TextureCoords::~TextureCoords()
{
    coords->unref();
    delete &mutex;
}


int TextureCoords::size(bool includedeleted) const
{ return coords->point.getNum()-(includedeleted ? 0 : unusedcoords.size()); }


void TextureCoords::setCoord( int idx, const Coord3& pos )
{
    Threads::MutexLocker lock( mutex );

    for ( int idy=coords->point.getNum(); idy<idx; idy++ )
	unusedcoords += idy;

    coords->point.set1Value( idx, SbVec3f( pos.x, pos.y, pos.z ));
}


void TextureCoords::setCoord( int idx, const Coord& pos )
{
    Threads::MutexLocker lock( mutex );

    for ( int idy=coords->point.getNum(); idy<idx; idy++ )
	unusedcoords += idy;

    coords->point.set1Value( idx, SbVec3f( pos.x, pos.y, 0 ));
}


int TextureCoords::addCoord( const Coord3& pos )
{
    Threads::MutexLocker lock( mutex );
    const int res = getFreeIdx();
    coords->point.set1Value( res, SbVec3f( pos.x, pos.y, pos.z ));

    return res;
}


int TextureCoords::addCoord( const Coord& pos )
{
    Threads::MutexLocker lock( mutex );
    const int res = getFreeIdx();
    coords->point.set1Value( res, SbVec3f( pos.x, pos.y, 0 ));

    return res;
}


void TextureCoords::removeCoord(int idx)
{
    Threads::MutexLocker lock( mutex );
    const int nrcoords = coords->point.getNum();
    if ( idx>=nrcoords )
    {
	pErrMsg("Invalid index");
	return;
    }

    if ( idx==nrcoords-1 )
	coords->point.deleteValues( idx );
    else
	unusedcoords += idx;
}


SoNode* TextureCoords::getInventorNode()
{ return coords; }


int  TextureCoords::getFreeIdx()
{
    if ( unusedcoords.size() )
    {
	const int res = unusedcoords[unusedcoords.size()-1];
	unusedcoords.remove(unusedcoords.size()-1);
	return res;
    }

    return coords->point.getNum();
}

}; // namespace visBase
