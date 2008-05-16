/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : March 2006
-*/

static const char* rcsID = "$Id: indexedshape.cc,v 1.4 2008-05-16 13:37:31 cvskris Exp $";

#include "indexedshape.h"

#include "positionlist.h"

namespace Geometry
{


IndexedGeometry::IndexedGeometry( Type type, NormalBinding nb,
				  Coord3List* coords, Coord3List* normals,
				  Coord2List* texturecoordlist )
    : coordlist_( coords )
    , type_( type )
    , normalbinding_( nb )
    , texturecoordlist_( texturecoordlist )
    , normallist_( normals )
    , ischanged_( true )
{
    if ( coordlist_ )		coordlist_->ref();
    if ( normallist_ )		normallist_->ref();
    if ( texturecoordlist_ )	texturecoordlist_->ref();
}


IndexedGeometry::~IndexedGeometry()
{
    removeAll();

    Threads::MutexLocker lock( lock_ );
    if ( coordlist_ ) coordlist_->unRef(); coordlist_ = 0;
    if ( normallist_ ) normallist_->unRef(); normallist_ = 0;
    if ( texturecoordlist_ ) texturecoordlist_->unRef(); texturecoordlist_=0;
}

void IndexedGeometry::removeAll()
{
    Threads::MutexLocker lock( lock_ );
    if ( coordlist_ )
    {
	for ( int idx=coordindices_.size()-1; idx>=0; idx-- )
	{
	    if ( coordindices_[idx]<0 )
		continue;

	    coordlist_->remove( coordindices_[idx] );
	}
    }

    if ( normallist_ )
    {
	for ( int idx=normalindices_.size()-1; idx>=0; idx-- )
	{
	    if ( normalindices_[idx]<0 )
		continue;

	    normallist_->remove( normalindices_[idx] );
	}
    }

    if ( texturecoordlist_ )
    {
	for ( int idx=texturecoordindices_.size()-1; idx>=0; idx-- )
	{
	    if ( texturecoordindices_[idx]<0 )
		continue;

	    texturecoordlist_->remove( texturecoordindices_[idx] );
	}
    }

    if ( coordindices_.size() || normalindices_.size() || texturecoordindices_.size() )
	ischanged_ = true;

    coordindices_.erase();
    normalindices_.erase();
    texturecoordindices_.erase();
}


bool IndexedGeometry::isEmpty() const
{
    return coordindices_.isEmpty() && normalindices_.isEmpty();
}


IndexedShape::IndexedShape()
    : coordlist_( 0 )
    , normallist_( 0 )
    , righthandednormals_( true )
{}


IndexedShape::~IndexedShape()
{
    setCoordList( 0, 0 );
}


void IndexedShape::setCoordList( Coord3List* cl,Coord3List* nl )
{
    removeAll();

    if ( coordlist_ ) coordlist_->unRef();
    coordlist_ = cl;
    if ( coordlist_ ) coordlist_->ref();

    if ( normallist_ ) normallist_->unRef();
    normallist_ = nl;
    if ( normallist_ ) normallist_->ref();
}


void IndexedShape::setRightHandedNormals( bool yn )
{ righthandednormals_ = yn; }


void IndexedShape::removeAll()
{
    geometrieslock_.writeLock();
    deepErase( geometries_ );
    geometrieslock_.writeUnLock();
}


const ObjectSet<IndexedGeometry>& IndexedShape::getGeometry() const
{ return geometries_; }


}; //namespace
