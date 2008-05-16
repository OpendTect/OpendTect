/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2002
 RCS:           $Id: vistexturecoords.cc,v 1.11 2008-05-16 16:04:10 cvskris Exp $
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
    : coords_( new SoTextureCoordinate3 )
    , mutex_( *new Threads::Mutex )
{
    coords_->ref();
    unusedcoords_ += 0;
    //!<To compensate for that the first coord is set by default by coin
}


TextureCoords::~TextureCoords()
{
    coords_->unref();
    delete &mutex_;
}


int TextureCoords::size(bool includedeleted) const
{ return coords_->point.getNum()-(includedeleted ? 0 : unusedcoords_.size()); }


void TextureCoords::setCoord( int idx, const Coord3& pos )
{
    Threads::MutexLocker lock( mutex_ );

    for ( int idy=coords_->point.getNum(); idy<idx; idy++ )
	unusedcoords_ += idy;

    coords_->point.set1Value( idx, SbVec3f( pos.x, pos.y, pos.z ));
}


void TextureCoords::setCoord( int idx, const Coord& pos )
{
    Threads::MutexLocker lock( mutex_ );

    for ( int idy=coords_->point.getNum(); idy<idx; idy++ )
	unusedcoords_ += idy;

    coords_->point.set1Value( idx, SbVec3f( pos.x, pos.y, 0 ));
}


int TextureCoords::addCoord( const Coord3& pos )
{
    Threads::MutexLocker lock( mutex_ );
    const int res = getFreeIdx();
    coords_->point.set1Value( res, SbVec3f( pos.x, pos.y, pos.z ));

    return res;
}


int TextureCoords::addCoord( const Coord& pos )
{
    Threads::MutexLocker lock( mutex_ );
    const int res = getFreeIdx();
    coords_->point.set1Value( res, SbVec3f( pos.x, pos.y, 0 ));

    return res;
}


Coord3 TextureCoords::getCoord( int idx ) const
{
    Threads::MutexLocker lock( mutex_ );
    if ( idx<0 || idx>=coords_->point.getNum() ||
	 unusedcoords_.indexOf(idx)!=-1 )
    {
	return Coord3::udf();
    }

    SbVec3f res = coords_->point[idx];
    return Coord3( res[0], res[1], res[2] );
}


int TextureCoords::nextID( int previd ) const
{
    Threads::MutexLocker lock( mutex_ );
    const int sz = coords_->point.getNum();

    int res = previd+1;
    while ( res<sz )
    {
	if ( unusedcoords_.indexOf(res)==-1 )
	    return res;
    }

    return -1;
}



void TextureCoords::removeCoord(int idx)
{
    Threads::MutexLocker lock( mutex_ );
    const int nrcoords = coords_->point.getNum();
    if ( idx>=nrcoords )
    {
	pErrMsg("Invalid index");
	return;
    }

    if ( idx==nrcoords-1 )
	coords_->point.deleteValues( idx );
    else
	unusedcoords_ += idx;
}


SoNode* TextureCoords::getInventorNode()
{ return coords_; }


int  TextureCoords::getFreeIdx()
{
    if ( unusedcoords_.size() )
    {
	const int res = unusedcoords_[unusedcoords_.size()-1];
	unusedcoords_.remove(unusedcoords_.size()-1);
	return res;
    }

    return coords_->point.getNum();
}


TextureCoordListAdapter::TextureCoordListAdapter( TextureCoords& c )
    : texturecoords_( c )
{ texturecoords_.ref(); }


TextureCoordListAdapter::~TextureCoordListAdapter()
{ texturecoords_.unRef(); }

int TextureCoordListAdapter::nextID( int previd ) const
{ return texturecoords_.nextID( previd ); }


int TextureCoordListAdapter::add( const Coord3& p )
{ return texturecoords_.addCoord( p ); }


Coord3 TextureCoordListAdapter::get( int idx ) const
{ return texturecoords_.getCoord( idx ); }


void TextureCoordListAdapter::set( int idx, const Coord3& p )
{ texturecoords_.setCoord( idx, p ); }


void TextureCoordListAdapter::remove( int idx )
{ texturecoords_.removeCoord( idx ); }


}; // namespace visBase
