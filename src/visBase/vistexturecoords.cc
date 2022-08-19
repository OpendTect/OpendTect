/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vistexturecoords.h"

#include "position.h"
#include "thread.h"

#include <osg/Array>

mCreateFactoryEntry( visBase::TextureCoords );

namespace visBase
{

#define mFreeTag	1e32f	// preferably different than OD undef value
#define mIsFreeTag(x)	( ((x)>9.99999e31f) && ((x)<1.00001e32f) )

#define mFreeOsgVec2	osg::Vec2( mFreeTag, mFreeTag )
#define mIsFreeIdx(idx)	mIsFreeTag( (*mGetOsgVec2Arr(osgcoords_))[idx][0] )


TextureCoords::TextureCoords()
    : lock_( Threads::Lock::MultiRead )
    , osgcoords_( new osg::Vec2Array )
    , nrfreecoords_( 0 )
    , lastsearchedidx_( -1 )
{
    mGetOsgVec2Arr(osgcoords_)->ref();
}


TextureCoords::~TextureCoords()
{
    mGetOsgVec2Arr(osgcoords_)->unref();
}


void TextureCoords::copyFrom( const TextureCoords& tc )
{
    Threads::Locker locker( lock_ );

    *mGetOsgVec2Arr(osgcoords_) = *mGetOsgVec2Arr(tc.osgcoords_);

    nrfreecoords_ = tc.nrfreecoords_;
}


int TextureCoords::size( bool includedeleted ) const
{
    Threads::Locker locker( lock_ );
    const int sz = mGetOsgVec2Arr(osgcoords_)->size();
    return includedeleted ? sz : sz-nrfreecoords_;
}


void TextureCoords::setCoord( int idx, const Coord3& pos )
{
    setCoord( idx, mCast(Coord,pos) );
}


void TextureCoords::setPositions( const Coord* pos, int sz, int start )
{
    Threads::Locker locker( lock_,Threads::Locker::WriteLock );

    for( int idx=0; idx<sz; idx++ )
	 setPosWithoutLock( idx+start, pos[idx] );
}


void TextureCoords::setCoord( int idx, const Coord& pos )
{
    if ( idx<0 )
	return;

    Threads::Locker locker( lock_, Threads::Locker::WriteLock );
    setPosWithoutLock( idx, pos );

}


void TextureCoords::setPosWithoutLock( int idx,const Coord& pos )
{
    int sz = mGetOsgVec2Arr(osgcoords_)->size();

    while(idx >= sz)
    {
	mGetOsgVec2Arr(osgcoords_)->push_back( mFreeOsgVec2 );
	nrfreecoords_++;
	sz++;
    }

    if( mIsFreeIdx(idx) )
	nrfreecoords_--;

    ( *mGetOsgVec2Arr(osgcoords_) )[idx] = Conv::to<osg::Vec2>( pos );

}


int TextureCoords::addCoord( const Coord3& pos )
{
    return addCoord( mCast(Coord,pos) );
}


int TextureCoords::addCoord( const Coord& pos )
{
    const int idx = searchFreeIdx();
    setCoord( idx, pos );
    return idx;
}


Coord3 TextureCoords::getCoord( int idx ) const
{
    Threads::Locker locker( lock_ );
    const int sz = mGetOsgVec2Arr(osgcoords_)->size();

    if ( idx<0 || idx>=sz || mIsFreeIdx(idx) )
	return Coord3::udf();

    return Coord3( Conv::to<Coord>((*mGetOsgVec2Arr(osgcoords_))[idx]), 0.0 );
}


void TextureCoords::clear()
{
    Threads::Locker locker( lock_, Threads::Locker::WriteLock );
    mGetOsgVec2Arr( osgcoords_ )->clear();
    nrfreecoords_ = 0;
    lastsearchedidx_ = -1;
}


int TextureCoords::nextID( int previd ) const
{
    if ( previd < -1 )
	return -1;

    Threads::Locker locker( lock_ );
    const int sz = mGetOsgVec2Arr(osgcoords_)->size();

    for ( int idx=previd+1; idx<sz; idx++ )
    {
	if ( !mIsFreeIdx(idx) )
	    return idx;
    }

    return -1;
}


void TextureCoords::removeCoord( int idx )
{
    Threads::Locker locker( lock_ );
    int sz = mGetOsgVec2Arr(osgcoords_)->size();

    if ( idx<0 || idx>=sz || mIsFreeIdx(idx) )
	return;

    locker.convertToWriteLock();
    (*mGetOsgVec2Arr(osgcoords_))[idx] = mFreeOsgVec2;
    nrfreecoords_++;

    while ( sz && mIsFreeIdx(sz-1) )
    {
	mGetOsgVec2Arr(osgcoords_)->pop_back();
	nrfreecoords_--;
	sz--;
    }
}


int TextureCoords::searchFreeIdx()
{
    Threads::Locker locker( lock_ );
    const int sz = mGetOsgVec2Arr(osgcoords_)->size();

    if ( nrfreecoords_ > 0 )
    {
	locker.convertToWriteLock();

	for ( int count=0; count<sz; count++ )
	{
	    lastsearchedidx_++;
	    if ( lastsearchedidx_ >= sz )
		lastsearchedidx_ = 0;

	    if ( mIsFreeIdx(lastsearchedidx_) )
		return lastsearchedidx_;
	}
    }

    return sz;
}


//==========================================================================


TextureCoordListAdapter::TextureCoordListAdapter( TextureCoords& c )
    : texturecoords_( c )
{ texturecoords_.ref(); }


TextureCoordListAdapter::~TextureCoordListAdapter()
{ texturecoords_.unRef(); }

int TextureCoordListAdapter::nextID( int previd ) const
{ return texturecoords_.nextID( previd ); }


int TextureCoordListAdapter::add( const Coord3& p )
{ return texturecoords_.addCoord( p ); }


void TextureCoordListAdapter::addValue( int, const Coord3& p )
{
    pErrMsg("Not implemented");
}


Coord3 TextureCoordListAdapter::get( int idx ) const
{ return texturecoords_.getCoord( idx ); }


bool TextureCoordListAdapter::isDefined( int idx ) const
{ return texturecoords_.getCoord( idx ).isDefined(); }


void TextureCoordListAdapter::set( int idx, const Coord3& p )
{ texturecoords_.setCoord( idx, p ); }


void TextureCoordListAdapter::remove( int idx )
{ texturecoords_.removeCoord( idx ); }


void TextureCoordListAdapter::remove( const TypeSet<int>& idxs )
{
    for ( int idx=idxs.size()-1; idx>=0; idx-- )
	texturecoords_.removeCoord( idxs[idx] );
}


} // namespace visBase
