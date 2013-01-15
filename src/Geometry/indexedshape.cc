/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : March 2006
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "indexedshape.h"

#include "positionlist.h"

namespace Geometry
{

    
PtrMan<PrimitiveSetCreator> PrimitiveSetCreator::creator_ = 0;
    
    
DefineEnumNames(PrimitiveSet, PrimitiveType, 5, "PrimitiveType" )
{ "Points", "Lines", "Triangles", "LineStrips", "TriangleStrips", "Fans", 0 };
    
    
    
PrimitiveSet::PrimitiveSet()
    : primitivetype_( Triangles )
{}
    
PrimitiveSet::PrimitiveType PrimitiveSet::getPrimitiveType() const
{
    return primitivetype_;
}


void PrimitiveSet::setPrimitiveType(Geometry::PrimitiveSet::PrimitiveType tp)
{
    primitivetype_ = tp;
}

    
    
PrimitiveSet* PrimitiveSetCreator::create( bool indexed, bool large )
{
    return creator_ ? creator_->doCreate( indexed, large ) : 0;
}
    
    
IndexedPrimitiveSet* IndexedPrimitiveSet::create( bool large )
{
    return (IndexedPrimitiveSet*) PrimitiveSetCreator::create( true, large );
}
    
    
RangePrimitiveSet* RangePrimitiveSet::create()
{
    return (RangePrimitiveSet*) PrimitiveSetCreator::create( true, false );
}

    
    
void PrimitiveSetCreator::setCreator( Geometry::PrimitiveSetCreator* c )
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
    
    if ( primitivesets_.size() && deep )
    {
	for ( int idx=0; idx<primitivesets_.size(); idx++ )
	{
	    RefMan<const IndexedPrimitiveSet> primitive = primitivesets_[idx];
	    for ( int idy=primitive->size()-1; idy>=0; idy-- )
	    {
		const int index = primitive->get(idy);
		if ( coordlist_ )
		    coordlist_->remove( index );
		if ( normallist_ )
		    normallist_->remove( index );
		if ( texturecoordlist_ )
		    texturecoordlist_->remove( index );
	    }
	}
    }

    if ( coordindices_.size() || normalindices_.size() ||
	 texturecoordindices_.size() )
	ischanged_ = true;

    coordindices_.erase();
    normalindices_.erase();
    texturecoordindices_.erase();
    
    deepUnRef( primitivesets_ );
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
	geometries_.removeSingle( idx, false );
    geometrieslock_.writeUnLock();
}


void ExplicitIndexedShape::removeFromGeometries( int idx )
{
    if ( idx<0 || idx>=geometries_.size() )
	return;
 
    removeFromGeometries( geometries_[idx] );
}


}; //namespace
