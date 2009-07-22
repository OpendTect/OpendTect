/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : March 2006
-*/

static const char* rcsID = "$Id: indexedshape.cc,v 1.9 2009-07-22 16:01:33 cvsbert Exp $";

#include "indexedshape.h"

#include "positionlist.h"

namespace Geometry
{


IndexedGeometry::IndexedGeometry( Type type, NormalBinding nb,
				  Coord3List* coords, Coord3List* normals,
				  Coord3List* texturecoordlist )
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

	    if ( idx<coordindices_.size()-1 &&
	         coordindices_.indexOf( coordindices_[idx], true, idx+1 )!=-1 )
	    {
		continue;
	    }

	    coordlist_->remove( coordindices_[idx] );
	}
    }

    if ( normallist_ )
    {
	for ( int idx=normalindices_.size()-1; idx>=0; idx-- )
	{
	    if ( normalindices_[idx]<0 )
		continue;

	    if ( idx<normalindices_.size()-1 && normalindices_.indexOf( 
			normalindices_[idx], true, idx+1 )!=-1 )
	    {
		continue;
	    }

	    normallist_->remove( normalindices_[idx] );
	}
    }

    if ( texturecoordlist_ )
    {
	for ( int idx=texturecoordindices_.size()-1; idx>=0; idx-- )
	{
	    if ( texturecoordindices_[idx]<0 )
		continue;

	    if ( idx<texturecoordindices_.size()-1 && texturecoordindices_.
		    indexOf( texturecoordindices_[idx], true, idx+1 )!=-1 )
	    {
		continue;
	    }

	    texturecoordlist_->remove( texturecoordindices_[idx] );
	}
    }

    if ( coordindices_.size() || normalindices_.size() ||
	 texturecoordindices_.size() )
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
    , texturecoordlist_( 0 )
    , righthandednormals_( true )
    , version_( 0 )
{}


IndexedShape::~IndexedShape()
{ setCoordList( 0, 0, 0 ); }


void IndexedShape::setCoordList( Coord3List* cl, Coord3List* nl,
       				 Coord3List* tcl )
{
    removeAll();

    if ( coordlist_ ) coordlist_->unRef();
    coordlist_ = cl;
    if ( coordlist_ ) coordlist_->ref();

    if ( normallist_ ) normallist_->unRef();
    normallist_ = nl;
    if ( normallist_ ) normallist_->ref();

    if ( texturecoordlist_ ) texturecoordlist_->unRef();
    texturecoordlist_ = tcl;
    if ( texturecoordlist_ ) texturecoordlist_->ref();
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


void IndexedShape::addVersion()
{
    version_++;
    if ( version_<0 ) version_ = 0;  //If it goes beyond INT_MAX
}


}; //namespace
