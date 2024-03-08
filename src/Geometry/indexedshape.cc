/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "indexedshape.h"

#include "positionlist.h"

namespace Geometry
{

PtrMan<PrimitiveSetCreator> PrimitiveSetCreator::creator_ = 0;

mDefineEnumUtils(PrimitiveSet, PrimitiveType, "PrimitiveType" )
{ "Points", "Lines", "Triangles", "LineStrips", "TriangleStrips", "Fans",
  "Other", 0 };


PrimitiveSet::PrimitiveSet()
    : primitivetype_( Triangles )
{}


PrimitiveSet::~PrimitiveSet()
{}


void PrimitiveSet::getAll( TypeSet<int>& res, bool onlyunique ) const
{
    res.setEmpty();

    for ( int idx=size()-1; idx>=0; idx-- )
    {
	if ( onlyunique )
	    res.addIfNew( get( idx) );
	else
	    res += get( idx );
    }
}


PrimitiveSet::PrimitiveType PrimitiveSet::getPrimitiveType() const
{
    return primitivetype_;
}


void PrimitiveSet::setPrimitiveType(Geometry::PrimitiveSet::PrimitiveType tp)
{
    primitivetype_ = tp;
}




// IndexedPrimitiveSet
IndexedPrimitiveSet::IndexedPrimitiveSet()
{}


IndexedPrimitiveSet::~IndexedPrimitiveSet()
{}


IndexedPrimitiveSet* IndexedPrimitiveSet::create( bool large )
{
    return (IndexedPrimitiveSet*) PrimitiveSetCreator::create( true, large );
}



// IndexedPrimitiveSetImpl
IndexedPrimitiveSetImpl::IndexedPrimitiveSetImpl()
{}


IndexedPrimitiveSetImpl::~IndexedPrimitiveSetImpl()
{}


int IndexedPrimitiveSetImpl::size() const
{ return indexset_.size(); }

int IndexedPrimitiveSetImpl::get( int idx ) const
{ return indexset_[idx]; }

int IndexedPrimitiveSetImpl::indexOf( const int val )
{ return indexset_.indexOf( val ); }

void IndexedPrimitiveSetImpl::append( int val )
{ indexset_ += val; }

void IndexedPrimitiveSetImpl::append( const int* arr, int num )
{ indexset_.append( arr, num ); }

void IndexedPrimitiveSetImpl::setEmpty()
{ indexset_.size(); }

void IndexedPrimitiveSetImpl::getAll( TypeSet<int>& valset, bool ) const
{ valset = indexset_; }

int IndexedPrimitiveSetImpl::pop()
{ return indexset_.pop(); }

int IndexedPrimitiveSetImpl::set( int, int )
{ return 0; }

void IndexedPrimitiveSetImpl::set( const int* arr, int num )
{ indexset_.copy( arr, num ); }



// RangePrimitiveSet
RangePrimitiveSet::RangePrimitiveSet()
{}


RangePrimitiveSet::~RangePrimitiveSet()
{}


RangePrimitiveSet* RangePrimitiveSet::create()
{
    return (RangePrimitiveSet*) PrimitiveSetCreator::create( false, false );
}


void RangePrimitiveSet::getAll( TypeSet<int>& res, bool ) const
{
    res.erase();

    Interval<int> range = getRange();

    for ( int idx=range.start; idx<=range.stop; idx++ )
	res += idx;
}



// PrimitiveSetCreator
PrimitiveSetCreator::PrimitiveSetCreator()
{}


PrimitiveSetCreator::~PrimitiveSetCreator()
{}


void PrimitiveSetCreator::setCreator( Geometry::PrimitiveSetCreator* c )
{
    creator_ = c;
}


PrimitiveSet* PrimitiveSetCreator::create( bool indexed, bool large )
{
    return creator_ ? creator_->doCreate( indexed, large ) : 0;
}



// PrimitiveSetCreatorDefImpl
PrimitiveSetCreatorDefImpl::PrimitiveSetCreatorDefImpl()
{}


PrimitiveSetCreatorDefImpl::~PrimitiveSetCreatorDefImpl()
{}


PrimitiveSet* PrimitiveSetCreatorDefImpl::doCreate( bool indexed, bool large )
{
    return (indexed && !large) ? new IndexedPrimitiveSetImpl : 0;
}



// IndexedGeometry
IndexedGeometry::IndexedGeometry( Type type, Coord3List* coords,
				  Coord3List* normals,
				  Coord3List* texturecoordlist,
				  SetType settype, bool large)
    : coordlist_( coords )
    , primitivetype_( type )
    , texturecoordlist_( texturecoordlist )
    , normallist_( normals )
    , ischanged_( true )
    , ishidden_( false )
    , primitivesettype_( settype )
{
    if ( primitivesettype_ == RangeSet )
	primitiveset_ =  Geometry::RangePrimitiveSet::create();
    else
	primitiveset_ =  Geometry::IndexedPrimitiveSet::create( large );

    if ( primitiveset_ )	primitiveset_->ref();
    if ( coordlist_ )		coordlist_->ref();
    if ( normallist_ )		normallist_->ref();
    if ( texturecoordlist_ )	texturecoordlist_->ref();

}


IndexedGeometry::~IndexedGeometry()
{
    removeAll( true );
    Threads::Locker lckr( lock_ );
    if ( primitiveset_ ) primitiveset_->unRef();
    if ( coordlist_ ) coordlist_->unRef(); coordlist_ = 0;
    if ( normallist_ ) normallist_->unRef(); normallist_ = 0;
    if ( texturecoordlist_ ) texturecoordlist_->unRef(); texturecoordlist_=0;
}


void IndexedGeometry::appendCoordIndices( const TypeSet<int>& indices,
					   bool reverse )
{
    if ( primitivesettype_ == RangeSet ) return;

    if ( indices.size()<2  ) return;

    switch ( primitivetype_ )
    {
    case Triangles:
	appendCoordIndicesAsTriangles( indices,reverse );
	break;
    case TriangleStrip:
	appendCoordIndicesAsTriangleStrips( indices );
	break;
    case TriangleFan:
	appendCoordIndicesAsTriangleFan( indices );
	break;
    case Points:
    case Lines:
	if ( primitiveset_ )
	    primitiveset_->append( indices.arr(), indices.size() );
	break;
    default:
	pErrMsg("Not implemented");
	break;
    }

}


void IndexedGeometry::appendCoordIndicesAsTriangleFan(
			const TypeSet<int>& indices )
{
    pErrMsg("Not implemented");
}


void IndexedGeometry::setCoordIndices( const TypeSet<int>& indices )
{
    if ( !primitiveset_ ) return;

    primitiveset_->setEmpty();
    appendCoordIndices( indices );
}


void IndexedGeometry::appendCoordIndicesAsTriangles(
	const TypeSet<int>& indices, bool reverse )
{
    if ( !primitiveset_ || primitivesettype_ == RangeSet ) return;

    for ( int idx = 0; idx< indices.size()-2; idx++ )
    {
	const bool doreverse = reverse != bool(idx%2);

	const int startidx = doreverse ? idx+2 : idx;
	const int endidx = doreverse ? idx : idx+2;
	primitiveset_->append( indices[startidx] );
	primitiveset_->append( indices[idx+1] );
	primitiveset_->append( indices[endidx] );
    }

}


void IndexedGeometry::appendCoordIndicesAsTriangleStrips(
			const TypeSet<int>& indices )
{
    if ( !primitiveset_ || primitivesettype_ == RangeSet ) return;

    if ( primitiveset_->size() )
    {
	primitiveset_->append( primitiveset_->get( primitiveset_->size()-1 ) );
	primitiveset_->append( indices.first() );
    }
    primitiveset_->append( indices.arr(),indices.size() );
}


void IndexedGeometry::removeAll( bool deep )
{
    if ( !primitiveset_ ) return;

    Threads::Locker lckr( lock_ );

    TypeSet<int> idxs;

    primitiveset_->getAll( idxs, true );

    if( coordlist_ )
	coordlist_->remove( idxs );
    if ( normallist_ )
	normallist_->remove( idxs );
    if ( texturecoordlist_ )
	texturecoordlist_->remove( idxs );

    if ( deep )
     unRefAndNullPtr( primitiveset_ );
    else
	primitiveset_->setEmpty();
}


bool IndexedGeometry::isEmpty() const
{
    return !primitiveset_ || !primitiveset_->size();
}



// IndexedShape
IndexedShape::IndexedShape()
{}


IndexedShape::~IndexedShape()
{
    setCoordList( 0, 0, 0 );
}


void IndexedShape::setCoordList( Coord3List* cl, Coord3List* nl,
				 Coord3List* tcl, bool createnew )
{
    if ( createnew )
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
    else
    {
	if( cl ) cl->unRef();
	    cl = coordlist_;
	if ( cl ) cl->ref();
	if( nl )  nl->unRef();
	    nl = normallist_;
	if ( nl ) nl->ref();
	if( tcl ) tcl->unRef();
	    tcl = texturecoordlist_;
	if( tcl ) tcl->ref();
    }
}


void IndexedShape::removeAll( bool deep )
{
    mGetIndexedShapeWriteLocker4Geometries();
    for ( int idx=geometries_.size()-1; idx>=0; idx-- )
	geometries_[idx]->removeAll( deep );

    deepErase( geometries_ );
}


const ObjectSet<IndexedGeometry>& IndexedShape::getGeometry() const
{ return geometries_; }


ObjectSet<IndexedGeometry>& IndexedShape::getGeometry()
{ return geometries_; }


void IndexedShape::addVersion()
{
    version_++;
    if ( version_<0 ) version_ = 0;  //If it goes beyond INT_MAX
}



// ExplicitIndexedShape
ExplicitIndexedShape::ExplicitIndexedShape()
{}


ExplicitIndexedShape::~ExplicitIndexedShape()
{}


int ExplicitIndexedShape::addGeometry( IndexedGeometry* ig )
{
    if ( !ig ) return -1;

    mGetIndexedShapeWriteLocker4Geometries();
    int res = geometries_.indexOf( ig );
    if ( res==-1 )
    {
	geometries_ += ig;
	res = geometries_.size()-1;
    }

    return res;
}

void ExplicitIndexedShape::removeFromGeometries( const IndexedGeometry* ig )
{
    if ( !ig ) return;

    mGetIndexedShapeWriteLocker4Geometries();
    const int idx = geometries_.indexOf( ig );
    if ( idx!=-1 )
	geometries_.removeSingle( idx, false );
}


void ExplicitIndexedShape::removeFromGeometries( int idx )
{
    if ( idx<0 || idx>=geometries_.size() )
	return;

    removeFromGeometries( geometries_[idx] );
}

} // namespace Geometry
