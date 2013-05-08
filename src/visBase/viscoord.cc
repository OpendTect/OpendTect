/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "viscoord.h"

#include "errh.h"
#include "vistransform.h"
#include "UTMPosition.h"
#include "visnormals.h"

#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoGroup.h>

#include <osg/Array>

mCreateFactoryEntry( visBase::Coordinates );

namespace visBase
{

Coordinates::Coordinates()
    : coords_( doOsg() ? 0 : new SoCoordinate3 )
    , transformation_( 0 )
    , utmposition_( 0 )
    , root_( doOsg() ? 0 : new SoGroup )
    , osgcoords_( doOsg() ? new osg::Vec3Array : 0 )
{
    if ( osgcoords_ )
	mGetOsgVec3Arr(osgcoords_)->ref();
    else
    {
	root_->ref();
	root_->addChild( coords_ );
	unusedcoords_ += 0;
	//!<To compensate for that the first coord is set by default by OI

    }
}


Coordinates::~Coordinates()
{
    if ( osgcoords_ )
	mGetOsgVec3Arr(osgcoords_)->unref();
    
    if ( root_ ) root_->unref();
    if ( transformation_ ) transformation_->unRef();
}


void Coordinates::copyFrom( const Coordinates& nc ) 
{
    Threads::MutexLocker lock( mutex_ );
    Threads::MutexLocker nclock( nc.mutex_ );

    if ( doOsg() )
    {
	*mGetOsgVec3Arr(osgcoords_) = *mGetOsgVec3Arr(nc.osgcoords_);
    }
    else
    {
	coords_->point = nc.coords_->point;
	if ( nc.utmposition_ )
	{
	    if ( !utmposition_ )
	    {
		utmposition_ = new UTMPosition;
		root_->insertChild( utmposition_, 0 );
	    }

	    utmposition_->utmposition = nc.utmposition_->utmposition;
	}
    }

    unusedcoords_ = nc.unusedcoords_;
}


void Coordinates::setDisplayTransformation( const mVisTrans* nt )
{
    if ( nt==transformation_ ) return;

    Threads::MutexLocker lock( mutex_ );
    TypeSet<Coord3> worldpos;
    getPositions(worldpos);

    if ( transformation_ )
	transformation_->unRef();

    transformation_ = nt;

    if ( transformation_ )
	transformation_->ref();

    setPositions(worldpos);
}


const mVisTrans*  Coordinates::getDisplayTransformation() const
{
    return transformation_;
}


void Coordinates::setLocalTranslation( const Coord& nc )
{
    Threads::MutexLocker lock( mutex_ );
    setLocalTranslationWithoutLock( nc );
}


void Coordinates::setLocalTranslationWithoutLock( const Coord& nc )
{
    TypeSet<Coord3> worldpos;
    getPositions(worldpos);

    if ( !utmposition_ )
    {
	utmposition_ = new UTMPosition;
	root_->insertChild( utmposition_, 0 );
    }

    Coord3 postoset( nc, 0 );
    if ( transformation_ )
	postoset = transformation_->transform( postoset );

    utmposition_->utmposition.setValue( SbVec3d(postoset.x,postoset.y,0) );

    setPositions(worldpos);
}


Coord Coordinates::getLocalTranslation() const
{
    if ( !utmposition_ ) return Coord(0,0);
    SbVec3d transl = utmposition_->utmposition.getValue();
    Coord3 res( transl[0], transl[1], 0 );
    if ( transformation_ ) res = transformation_->transformBack( res );
    return res;
}


#define mArrSize \
    (doOsg() ? (int) mGetOsgVec3Arr(osgcoords_)->size() \
	     : (int) coords_->point.getNum())

int Coordinates::size(bool includedeleted) const
{
    return mArrSize -(includedeleted ? 0 : unusedcoords_.size()); }


int Coordinates::nextID( int previd ) const
{
    Threads::MutexLocker lock( mutex_ );

    const int sz = mArrSize;

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
    int res;
    const int nrunused = unusedcoords_.size();
    if ( nrunused )
    {
	res = unusedcoords_[nrunused-1];
	unusedcoords_.removeSingle( nrunused-1 );
    }
    else
    {
	res = mArrSize;
    }

    Coord3 postoset = pos;
    if ( postoset.isDefined() )
    {
	if ( transformation_ )
	    postoset = transformation_->transform( postoset );

	if ( utmposition_ )
	{
	    SbVec3d utmoffset = utmposition_->utmposition.getValue();
	    postoset.x -= utmoffset[0];
	    postoset.y -= utmoffset[1];
	}
    }

    if ( doOsg() )
	mGetOsgVec3Arr(osgcoords_)->push_back(
		osg::Vec3f( (float) postoset.x,
			    (float) postoset.y,
			    (float) postoset.z));
    else
	coords_->point.set1Value( res, SbVec3f((float) postoset.x,
				(float) postoset.y,(float) postoset.z) );

    return res;
}


void Coordinates::insertPos( int idx, const Coord3& pos )
{
    if ( doOsg() )
    {
	pErrMsg( "Not implemented" );
	return;
	
    }
    Threads::MutexLocker lock( mutex_ );
    
    coords_->point.insertSpace( idx, 1 );
    for ( int idy=unusedcoords_.size()-1; idy>=0; idy-- )
    {
	if ( unusedcoords_[idy]>=idx )
	    unusedcoords_[idy]++;
    }

    setPosWithoutLock(idx,pos);
}


Coord3 Coordinates::getPos( int idx, bool scenespace ) const
{
    const float* scenepos;
    if ( doOsg() )
        scenepos = (*mGetOsgVec3Arr(osgcoords_))[idx].ptr();
    else
	scenepos = coords_->point[idx].getValue();
    
    Coord3 res( scenepos[0], scenepos[1], scenepos[2] );
    if ( res.isDefined() )
    {
	if ( utmposition_ )
	{
	    SbVec3d utmoffset = utmposition_->utmposition.getValue();
	    res.x += utmoffset[0];
	    res.y += utmoffset[1];
	}

	if ( transformation_ && !scenespace )
	    res = transformation_->transformBack( res );
    }

    return res;
}


bool Coordinates::isDefined( int idx ) const
{
    Threads::MutexLocker lock( mutex_ );
    if ( idx<0 || idx>=mArrSize ||
	 unusedcoords_.isPresent( idx ) )
	return false;

    const float* coord;
    if ( doOsg() )
	coord = (*mGetOsgVec3Arr(osgcoords_))[idx].ptr();
    else
	coord = coords_->point[idx].getValue();
    
    return !mIsUdf(coord[2]) && !mIsUdf(coord[1]) && !mIsUdf(coord[0]);
}


void Coordinates::setPos( int idx, const Coord3& pos )
{
    Threads::MutexLocker lock( mutex_ );
    setPosWithoutLock(idx,pos);
}


void Coordinates::setPosWithoutLock( int idx, const Coord3& pos )
{
    for ( int idy=mArrSize; idy<idx; idy++ )
	unusedcoords_ += idy;

    Coord3 postoset = pos;
    if ( postoset.isDefined() )
    {
	if ( transformation_ )
	{
	    postoset = transformation_->transform( postoset );

	    //HACK: Moved here since it blocks the transform setting
	    //      on inl/crl/t objects
	    if ( !utmposition_ && !idx && !size(false) &&
		    (fabs(postoset.x)>1e5 || fabs(postoset.y)>1e5) )
		setLocalTranslationWithoutLock(postoset);
	}

	/* 
	if ( !utmposition_ && !idx && !size(false) &&
		(fabs(postoset.x)>1e5 || fabs(postoset.y)>1e5) )
	    setLocalTranslationWithoutLock(postoset);
	*/

	if ( utmposition_ )
	{
	    SbVec3d utmoffset = utmposition_->utmposition.getValue();
	    postoset.x -= utmoffset[0];
	    postoset.y -= utmoffset[1];
	}
    }
    
    if ( doOsg() )
    {
	if ( idx>=mGetOsgVec3Arr(osgcoords_)->size() )
	    mGetOsgVec3Arr(osgcoords_)->resize( idx+1 );
	
	(*mGetOsgVec3Arr(osgcoords_))[idx] =
	    osg::Vec3f((float) postoset.x,(float) postoset.y,(float)postoset.z);
	    
    }
    else
	coords_->point.set1Value( idx, SbVec3f((float) postoset.x,
				(float) postoset.y,(float) postoset.z) );

    const int unusedidx = unusedcoords_.indexOf(idx);
    if ( unusedidx!=-1 )
	unusedcoords_.removeSingle( unusedidx );
}


void Coordinates::removePos( int idx, bool keepidxafter )
{
    Threads::MutexLocker lock( mutex_ );
    const int nrcoords = mArrSize;
    if ( idx>=nrcoords || idx<0 )
    {
	pErrMsg("Invalid index");
	return;
    }

    if ( idx==nrcoords-1 )
    {
	if ( doOsg() )
	    mGetOsgVec3Arr(osgcoords_)->resize( idx );
	else
	    coords_->point.deleteValues( idx );
	unusedcoords_ -= idx;
    }
    else if ( keepidxafter )
	unusedcoords_ += idx;
    else
    {
	if ( doOsg() )
	{
	    mGetOsgVec3Arr(osgcoords_)->erase(
				 mGetOsgVec3Arr(osgcoords_)->begin() + idx );
	}
	else
	    coords_->point.deleteValues( idx, 1 );
	
	for ( int idy=unusedcoords_.size()-1; idy>=0; idy-- )
	{
	    if ( unusedcoords_[idy]>idx )
		unusedcoords_[idy]--;
	}
    }
}


void Coordinates::removeAfter( int idx )
{
    Threads::MutexLocker lock( mutex_ );
    if ( idx<-1 || idx>=mArrSize-1 )
	return;

    if ( doOsg() )
	mGetOsgVec3Arr(osgcoords_)->resize( idx+1 );
    else
	coords_->point.deleteValues( idx+1 );
    for ( int idy=0; idy<unusedcoords_.size(); idy++ )
    {
	if ( unusedcoords_[idy]>idx )
	    unusedcoords_.removeSingle(idy--);
    }
}


void Coordinates::setAutoUpdate( bool doupdate )
{
    bool oldvalue = coords_->point.enableNotify( doupdate );
    if ( doupdate && !oldvalue ) coords_->point.touch();
}    


bool Coordinates::autoUpdate()
{
    return coords_->point.isNotifyEnabled();
}


void Coordinates::update()
{
    coords_->point.touch();
}


SoNode* Coordinates::gtInvntrNode() { return root_; }


void Coordinates::setAllZ( const float* vals, int sz, float zscale )
{
    if ( sz != mArrSize )
	coords_->point.setNum( sz );

    float* zvals = ((float*) coords_->point.startEditing())+2;
    float* stopptr = zvals + sz*3;
    if ( !mIsZero(zscale-1,1e-8) )
    {
	while ( zvals<stopptr )
	{
	    *zvals = *vals * zscale;
	    zvals += 3;
	    vals++;
	}
    }
    else
    {
	while ( zvals<stopptr )
	{
	    *zvals = *vals;
	    zvals += 3;
	    vals++;
	}
    }

    coords_->point.finishEditing();
}



void Coordinates::getPositions(TypeSet<Coord3>& res) const
{
    for ( int idx=0; idx<mArrSize; idx++ )
	res += getPos(idx);
}


void Coordinates::setPositions( const TypeSet<Coord3>& pos)
{
    const bool oldstatus = coords_->point.enableNotify( false );
    for ( int idx=0; idx<mArrSize; idx++ )
    {
	if ( unusedcoords_.isPresent(idx) )
	    continue;

	setPosWithoutLock(idx, pos[idx] );
    }

    coords_->point.enableNotify( oldstatus );
    coords_->point.touch();
}


void Coordinates::setPositions( const Coord3* pos, int sz, int start )
{
    Threads::MutexLocker lock( mutex_ );

    const bool oldstatus = coords_->point.enableNotify( false );
    for ( int idx=0; idx<sz; idx++ )
	setPosWithoutLock(idx+start, pos[idx] );

    coords_->point.enableNotify( oldstatus );
    coords_->point.touch();
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

};
