/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : March 2006
-*/

static const char* rcsID mUnusedVar = "$Id: indexedshape.cc,v 1.15 2012-09-04 09:32:39 cvskris Exp $";

#include "indexedshape.h"

#include "positionlist.h"

namespace Geometry
{

    
PtrMan<IndexedPrimitiveCreator> IndexedPrimitiveCreator::creator_ = 0;

    
IndexedPrimitive* IndexedPrimitiveCreator::create()
{
    return creator_ ? creator_->doCreate() : 0;
}
    
    
void IndexedPrimitiveCreator::setCreator(Geometry::IndexedPrimitiveCreator* c)
{
    creator_ = c;
}
    

IndexedGeometry::IndexedGeometry( Type type, NormalBinding nb,
				  Coord3List* coords, Coord3List* normals,
				  Coord3List* texturecoordlist )
    : coordlist_( coords )
    , type_( type )
    , normalbinding_( nb )
    , texturecoordlist_( texturecoordlist )
    , normallist_( normals )
    , ischanged_( true )
    , ishidden_( false )
{
    if ( coordlist_ )		coordlist_->ref();
    if ( normallist_ )		normallist_->ref();
    if ( texturecoordlist_ )	texturecoordlist_->ref();
}


IndexedGeometry::~IndexedGeometry()
{
    removeAll( true );
    Threads::MutexLocker lock( lock_ );
    if ( coordlist_ ) coordlist_->unRef(); coordlist_ = 0;
    if ( normallist_ ) normallist_->unRef(); normallist_ = 0;
    if ( texturecoordlist_ ) texturecoordlist_->unRef(); texturecoordlist_=0;
}

void IndexedGeometry::removeAll( bool deep )
{
    Threads::MutexLocker lock( lock_ );
    if ( deep && coordlist_ )
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

    if ( deep && normallist_ )
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

    if ( deep && texturecoordlist_ )
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
    removeAll( true );

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


void IndexedShape::removeAll( bool deep )
{
    geometrieslock_.writeLock();
    for ( int idx=geometries_.size()-1; idx>=0; idx-- )
	geometries_[idx]->removeAll( deep );

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

int ExplicitIndexedShape::addGeometry( IndexedGeometry* ig )
{
    if ( !ig ) return -1;
    
    geometrieslock_.writeLock();
    int res = geometries_.indexOf( ig );
    if ( res==-1 ) 
    {
    	geometries_ += ig;
	res = geometries_.size()-1;
    }

    geometrieslock_.writeUnLock();
    return res;
}

void ExplicitIndexedShape::removeFromGeometries( const IndexedGeometry* ig )
{
    if ( !ig ) return;
    
    geometrieslock_.writeLock();
    const int idx = geometries_.indexOf( ig );
    if ( idx!=-1 )
	geometries_.remove( idx, false );
    geometrieslock_.writeUnLock();
}


void ExplicitIndexedShape::removeFromGeometries( int idx )
{
    if ( idx<0 || idx>=geometries_.size() )
	return;
 
    removeFromGeometries( geometries_[idx] );
}


}; //namespace
