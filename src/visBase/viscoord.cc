/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2002
________________________________________________________________________

-*/

#include "viscoord.h"

#include "vistransform.h"
#include "visnormals.h"
#include "paralleltask.h"

#include <osg/Array>

mCreateFactoryEntry( visBase::Coordinates );

namespace visBase
{

class SetOrGetCoordinates: public ParallelTask
{
public:
    SetOrGetCoordinates(Coordinates* p, const od_int64 size,
			const Coord3* inpositions = 0,
			TypeSet<Coord3>* outpositions= 0,int startidx=0,
			bool scenespace = false);
    od_int64	totalNr() const override { return totalnrcoords_; }
    void	setWithSingleCoord(const Coord3 coord)
		{singlecoord_ = coord;setwithsinglecoord_ = true;}

protected:
    bool	doWork(od_int64 start,od_int64 stop,int) override;
    od_int64	nrIterations() const override { return totalnrcoords_; }

private:
    Coordinates*		coordinates_;
    TypeSet<Coord3>*		outpositions_;
    const Coord3*		inpositions_;
    Threads::Atomic<od_int64>	totalnrcoords_;
    Coord3			singlecoord_;
    bool			setwithsinglecoord_;
    const int			startidx_;
    bool			scenespace_;
};


SetOrGetCoordinates::SetOrGetCoordinates( Coordinates* p, const od_int64 size,
					  const Coord3* inpositions,
					  TypeSet<Coord3>* outpositions,
					  int startidx,
					  bool scenespace )
    : coordinates_( p )
    , totalnrcoords_( size )
    , inpositions_( inpositions )
    , outpositions_( outpositions )
    , setwithsinglecoord_( false )
    , singlecoord_( Coord3( 0,0,0 ) )
    , startidx_( startidx )
    , scenespace_( scenespace )
{
    if ( outpositions ) outpositions->setSize( size, Coord3::udf() );
    if ( (startidx_+size)>= p->size() )
	mGetOsgVec3Arr(p->osgArray())->resize( startidx_+size );
}


bool SetOrGetCoordinates::doWork(od_int64 start,od_int64 stop,int)
{
    if ( !inpositions_ && !outpositions_ )
	return false;

    for ( int idx=mCast(int,start); idx<=mCast(int,stop); idx++ )
    {
	if ( inpositions_ )
	{
	    const Coord3& pos = setwithsinglecoord_ ?
				singlecoord_ : inpositions_[idx];

	    coordinates_->setPosWithoutLock( startidx_+idx,
					     pos, scenespace_ );
	}
	else
	{
	     (*outpositions_)[idx] = coordinates_->getPos( startidx_+ idx );
	}
    }
    return true;
}


Coordinates::Coordinates()
    : transformation_( 0 )
    , osgcoords_( new osg::Vec3Array )
    , change( this )
{
    mGetOsgVec3Arr(osgcoords_)->ref();
}


Coordinates::~Coordinates()
{

    mGetOsgVec3Arr(osgcoords_)->unref();

    if ( transformation_ ) transformation_->unRef();
}


void Coordinates::copyFrom( const Coordinates& nc )
{
    Threads::MutexLocker lock( mutex_ );
    Threads::MutexLocker nclock( nc.mutex_ );

    *mGetOsgVec3Arr(osgcoords_) = *mGetOsgVec3Arr(nc.osgcoords_);

    unusedcoords_ = nc.unusedcoords_;
}


int Coordinates::arraySize() const
{
    return sCast(osg::Vec3Array*,osgcoords_)->size();
}


void Coordinates::setDisplayTransformation( const mVisTrans* nt )
{
    if ( nt==transformation_ ) return;

    TypeSet<Coord3> worldpos;
    worldpos.setSize( arraySize() );
    getPositions(worldpos);

    if ( transformation_ )
	transformation_->unRef();

    transformation_ = nt;

    if ( transformation_ )
	transformation_->ref();

    setPositions(worldpos);

    change.trigger();
}


const mVisTrans*  Coordinates::getDisplayTransformation() const
{
    return transformation_;
}


int Coordinates::size(bool includedeleted) const
{
    return arraySize() -(includedeleted ? 0 : unusedcoords_.size());
}


void Coordinates::setEmpty()
{
    Threads::MutexLocker lock( mutex_ );

    unusedcoords_.erase();
    mGetOsgVec3Arr(osgcoords_)->clear();

    change.trigger();

}


int Coordinates::nextID( int previd ) const
{
    Threads::MutexLocker lock( mutex_ );

    const int sz = arraySize();

    int res = previd+1;
    while ( res<sz )
    {
	if ( !unusedcoords_.isPresent(res) )
	    return res;
	else
	    res++;
    }

    return -1;
}


int Coordinates::addPos( const Coord3& pos )
{
    Threads::MutexLocker lock( mutex_ );
    const int nrunused = unusedcoords_.size();
    if ( nrunused )
    {
	int res = unusedcoords_.pop();
	setPosWithoutLock( res, pos, false );
	return res;
    }

    Coord3 postoset = pos;
    if ( postoset.isDefined() )
    {
	Transformation::transform( transformation_, postoset );
    }

    mGetOsgVec3Arr(osgcoords_)->push_back( Conv::to<osg::Vec3>(postoset) );
    change.trigger();

    return  mGetOsgVec3Arr(osgcoords_)->size()-1;
}


void Coordinates::insertPos( int idx, const Coord3& pos )
{
    Threads::MutexLocker lock( mutex_ );

    if ( unusedcoords_.isPresent(idx) ||
	mGetOsgVec3Arr(osgcoords_)->size()<idx )
	return;

    Coord3 postoset = pos;
    if ( postoset.isDefined() && transformation_ )
	transformation_->transform(postoset);

    mGetOsgVec3Arr(osgcoords_)->insert(
	mGetOsgVec3Arr(osgcoords_)->begin()+idx,Conv::to<osg::Vec3f>(postoset));

    for ( int idy = unusedcoords_.size()-1; idy>=0; idy-- )
    {
	if ( unusedcoords_[idy]>=idx )
	    unusedcoords_[idy]++;
    }

    osgcoords_->dirty();

}


Coord3 Coordinates::getPos( int idx, bool scenespace ) const
{
    if ( idx >= arraySize() )
	return Coord3::udf();

    const osg::Vec3* allpos =
		sCast(const osg::Vec3*,osgcoords_->getDataPointer());
    const float* scenepos = allpos[idx].ptr();

    Coord3 res = scenepos ? Coord3( scenepos[0], scenepos[1], scenepos[2] )
			  : Coord3::udf();
    if ( res.isDefined() )
    {
	if ( transformation_ && !scenespace )
	    transformation_->transformBack( res );
    }

    return res;
}


bool Coordinates::isDefined( int idx ) const
{
    Threads::MutexLocker lock( mutex_ );
    if ( idx<0 || idx>=arraySize() ||
	 unusedcoords_.isPresent( idx ) )
	return false;

    const float* coord = (*mGetOsgVec3Arr(osgcoords_))[idx].ptr();

    return !mIsUdf(coord[2]) && !mIsUdf(coord[1]) && !mIsUdf(coord[0]);
}


void Coordinates::setPos( int idx, const Coord3& pos )
{
    Threads::MutexLocker lock( mutex_ );
    setPosWithoutLock(idx,pos,false);
    change.trigger();
}


void Coordinates::setPosWithoutLock( int idx, const Coord3& pos,
				     bool scenespace )
{
    if ( unusedcoords_.isPresent(idx) )
	return;

    for ( int idy=arraySize(); idy<idx; idy++ )
	unusedcoords_ += idy;

    Coord3 postoset = pos;
    if ( !scenespace && postoset.isDefined() && transformation_ )
	transformation_->transform( postoset );

    if ( idx>=mGetOsgVec3Arr(osgcoords_)->size() )
	mGetOsgVec3Arr(osgcoords_)->resize( idx+1 );

    (*mGetOsgVec3Arr(osgcoords_))[idx] = Conv::to<osg::Vec3f>( postoset );
    osgcoords_->dirty();

    const int unusedidx = unusedcoords_.indexOf(idx);
    if ( unusedidx!=-1 )
	unusedcoords_.removeSingle( unusedidx );

}


void Coordinates::removePos( int idx, bool keepidxafter )
{
    Threads::MutexLocker lock( mutex_ );
    const int nrcoords = arraySize();
    if ( idx>=nrcoords || idx<0 )
    {
	pErrMsg("Invalid index");
	return;
    }

    if ( idx==nrcoords-1 )
    {
	mGetOsgVec3Arr(osgcoords_)->resize( idx );

	unusedcoords_ -= idx;
    }
    else if ( keepidxafter )
	unusedcoords_ += idx;
    else
    {
	if ( idx < mGetOsgVec3Arr(osgcoords_)->size()  )
	{
	    mGetOsgVec3Arr(osgcoords_)->erase(
		mGetOsgVec3Arr(osgcoords_)->begin() + idx );
	    osgcoords_->dirty();

	    for ( int idy=unusedcoords_.size()-1; idy>=0; idy-- )
	    {
		if ( unusedcoords_[idy]>idx )
		    unusedcoords_[idy]--;
	    }
	}
    }

    change.trigger();
}


void Coordinates::removeAfter( int idx )
{
    Threads::MutexLocker lock( mutex_ );
    if ( idx<-1 || idx>=arraySize()-1 )
	return;

    mGetOsgVec3Arr(osgcoords_)->resize( idx+1 );

    for ( int idy=0; idy<unusedcoords_.size(); idy++ )
    {
	if ( unusedcoords_[idy]>idx )
	    unusedcoords_.removeSingle(idy--);
    }

    dirty();

    change.trigger();

}


void Coordinates::setAllZ( const float* vals, int sz, bool dotransf )
{
    Threads::MutexLocker lock( mutex_ );
    if ( sz != arraySize() )
	mGetOsgVec3Arr(osgcoords_)->resize( sz );

    auto* czvals = sCast(const float*,osgcoords_->getDataPointer())+2;
    float* zvals = cCast(float*,czvals);
    float* stopptr = zvals + sz*3;
    while ( zvals<stopptr )
    {
	if ( dotransf && transformation_ && !mIsUdf(*vals) )
	    *zvals = *vals*transformation_->getScale().z+
	    transformation_->getTranslation().z;
	else
	    *zvals = *vals;
	zvals += 3;
	vals++;
    }
}


void Coordinates::getPositions(TypeSet<Coord3>& res) const
{
    Threads::MutexLocker lock(mutex_);

    SetOrGetCoordinates getcoordinates( const_cast<Coordinates*>(this),
	arraySize(), 0, &res );

    getcoordinates.execute();
}


void Coordinates::setPositions( const TypeSet<Coord3>& pos)
{
    setPositions( pos.arr(), pos.size(), 0 );
}


void Coordinates::setPositions( const Coord3* pos, int sz, int start,
				bool scenespace )
{
    if ( sz==0 ) return;

    Threads::MutexLocker lock( mutex_ );

    SetOrGetCoordinates setcoordinates( this, sz, pos, 0, start, scenespace );
    setcoordinates.execute();

    change.trigger();
}


void Coordinates::setAllPositions( const Coord3& pos, int sz, int start )
{
    Threads::MutexLocker lock( mutex_ );
    SetOrGetCoordinates setcoordinates( this,sz,0 );
    setcoordinates.setWithSingleCoord( pos );

    setcoordinates.execute();

    change.trigger();
}


void Coordinates::dirty() const
{
    if ( osgcoords_  )
	osgcoords_->dirty();
}

CoinFloatVertexAttribList::CoinFloatVertexAttribList(Coordinates& c, Normals* n)
    : coords_( c )
    , normals_( n )
{
    coords_.ref();
    if ( normals_ ) normals_->ref();
}


CoinFloatVertexAttribList::~CoinFloatVertexAttribList()
{
    coords_.unRef();
    if ( normals_ ) normals_->unRef();
}


int CoinFloatVertexAttribList::size() const
{
    return coords_.size();
}


bool CoinFloatVertexAttribList::setSize(int sz,bool cpdata)
{
    if ( sz>size() )
    {
	coords_.setPos( sz-1, Coord3::udf() );
	if ( normals_ ) normals_->setNormal(sz-1, Coord3::udf() );
    }
    else if ( sz>size() )
    {
	coords_.removeAfter( sz-1 );
    }

    return true;
}


 void	CoinFloatVertexAttribList::setCoord(int idx,const float* pos)
{
    const Coord3 coord( pos[0], pos[1], pos[2] );
    coords_.setPos( idx, coord );
}


void	CoinFloatVertexAttribList::getCoord( int idx, float* res ) const
{
    const Coord3 coord = coords_.getPos( idx );
    res[0] = (float) coord.x;
    res[1] = (float) coord.y;
    res[2] = (float) coord.z;
}


void CoinFloatVertexAttribList::setNormal( int idx, const float* pos )
{
    if ( !normals_ )
	return;

    const Coord3 coord( pos[0], pos[1], pos[2] );
    normals_->setNormal( idx, coord );
}


void	CoinFloatVertexAttribList::getNormal( int idx, float* res ) const
{
    if ( !normals_ )
	return;

    const Coord3 coord = normals_->getNormal( idx );
    res[0] = (float) coord.x;
    res[1] = (float) coord.y;
    res[2] = (float) coord.z;
}


void CoinFloatVertexAttribList::setTCoord(int,const float*)
{}


void CoinFloatVertexAttribList::getTCoord(int,float*) const
{}



CoordListAdapter::CoordListAdapter( Coordinates& c )
    : coords_( c )
{
    coords_.ref();
}


CoordListAdapter::~CoordListAdapter()
{
    coords_.unRef();
}


int CoordListAdapter::nextID( int previd ) const
{ return coords_.nextID( previd ); }


int CoordListAdapter::add( const Coord3& p )
{ return coords_.addPos( p ); }


void CoordListAdapter::addValue( int idx, const Coord3& p )
{
    pErrMsg("Not implemented");
}


Coord3 CoordListAdapter::get( int idx ) const
{ return coords_.getPos( idx, false ); }


bool CoordListAdapter::isDefined( int idx ) const
{ return coords_.isDefined( idx ); }


void CoordListAdapter::set( int idx, const Coord3& p )
{ coords_.setPos( idx, p ); }


void CoordListAdapter::remove( int idx )
{
    coords_.removePos( idx, true );
}

void CoordListAdapter::remove(const TypeSet<int>& idxs)
{
    for ( int idx = idxs.size()-1; idx>=0; idx-- )
	coords_.removePos( idxs[idx], true );

}

} // namespace visBase
