/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2002
________________________________________________________________________

-*/

#include "visnormals.h"

#include "trigonometry.h"
#include "thread.h"
#include "vistransform.h"
#include "paralleltask.h"

#include <osg/Array>

mCreateFactoryEntry( visBase::Normals );

namespace visBase
{


class DoTransformation: public ParallelTask
{
public:
    DoTransformation(Normals* p, const od_int64 size, const mVisTrans* oldtrans,
		     const mVisTrans* newtrans);
    od_int64	totalNr() const { return totalnrcoords_; }

protected:
    bool	doWork(od_int64 start, od_int64 stop, int);
    od_int64	nrIterations() const { return totalnrcoords_; }

private:
    Normals* normals_;
    Threads::Atomic<od_int64>	totalnrcoords_;
    const mVisTrans* oldtrans_;
    const mVisTrans* newtrans_;

};


DoTransformation::DoTransformation( Normals* p, const od_int64 size,
			const mVisTrans* oldtrans, const mVisTrans* newtrans )
    : normals_( p )
    , totalnrcoords_( size )
    , oldtrans_( oldtrans )
    , newtrans_( newtrans )
{
}



bool DoTransformation::doWork(od_int64 start,od_int64 stop,int)
{
    osg::Vec3Array* osgnormals = mGetOsgVec3Arr( normals_->osgnormals_ );
    if ( !osgnormals )
	return false;
    for ( int idx = mCast(int,start); idx<=mCast(int,stop); idx++ )
    {
	osg::Vec3f normal;
	visBase::Transformation::transformBackNormal( oldtrans_,
	    (*osgnormals)[idx], normal );
	visBase::Transformation::transformNormal( newtrans_, normal );
	normal.normalize();
	    (*osgnormals)[idx] =  normal;
    }
    return true;
}



Normals::Normals()
    : osgnormals_( new osg::Vec3Array )
    , mutex_( *new Threads::Mutex )
    , transformation_( 0 )
{
    osgnormals_->ref();
}


Normals::~Normals()
{
    delete &mutex_;
    if ( transformation_ ) transformation_->unRef();

    osgnormals_->unref();
}


void Normals::setNormal( int idx, const Vector3& n )
{
    osg::Vec3f osgnormal;
    visBase::Transformation::transformNormal( transformation_, n, osgnormal );
    if ( transformation_ )
	osgnormal.normalize();
    Threads::MutexLocker lock( mutex_ );
    osg::Vec3Array* osgnormals = mGetOsgVec3Arr( osgnormals_ );
    for ( int idy=osgnormals->size(); idy<=idx; idy++ )
    {
	unusednormals_ += idy;
	osgnormals->push_back( osg::Vec3f(mUdf(float),mUdf(float),mUdf(float)));
    }
    (*osgnormals)[idx] = osgnormal;
}


int Normals::nrNormals() const
{ return mGetOsgVec3Arr(osgnormals_)->size(); }


void Normals::clear()
{
    Threads::MutexLocker lock( mutex_ );
    mGetOsgVec3Arr( osgnormals_ )->clear();
}


void Normals::inverse()
{
    Threads::MutexLocker lock( mutex_ );

    osg::Vec3Array* osgnormals = mGetOsgVec3Arr( osgnormals_ );
    for ( int idx=osgnormals->size()-1; idx>=0; idx-- )
	(*osgnormals)[idx] *= -1;
}


int Normals::nextID( int previd ) const
{
    Threads::MutexLocker lock( mutex_ );

    const int sz = nrNormals();

    int res = previd+1;
    while ( res<sz )
    {
	if ( unusednormals_.indexOf(res)==-1 )
	    return res;
    }

    return -1;
}


void Normals::setAll( const Coord3* coords, int nmsz )
{
    Threads::MutexLocker lock( mutex_ );

    osg::Vec3Array* osgnormals = mGetOsgVec3Arr( osgnormals_ );
    if ( nmsz!=nrNormals() )
	 osgnormals->resize( nmsz );

    int nrnormals( 0 );
    while ( nrnormals< nmsz )
    {
	( *osgnormals )[nrnormals] =
	   osg::Vec3f( Conv::to<osg::Vec3>(coords[nrnormals]) );
	 nrnormals++;
     }
}


void Normals::setAll( const Coord3& coord, int nmsz )
{
     Threads::MutexLocker lock( mutex_ );

     osg::Vec3Array* osgnormals = mGetOsgVec3Arr( osgnormals_ );
     if ( nmsz!=nrNormals() )
	  osgnormals->resize( nmsz );

     int nrnormals( 0 );
     while ( nrnormals< nmsz )
     {
	( *osgnormals )[nrnormals] =
	    osg::Vec3f( Conv::to<osg::Vec3>(coord) );
	nrnormals++;
     }

}

int Normals::addNormal( const Vector3& n )
{

    osg::Vec3f osgnormal;
    visBase::Transformation::transformNormal( transformation_, n, osgnormal );

    Threads::MutexLocker lock( mutex_ );

    osg::Vec3Array* osgnormals = mGetOsgVec3Arr( osgnormals_ );
    osgnormals->push_back( osgnormal ) ;

    return nrNormals();
}


void Normals::addNormalValue( int idx, const Vector3& n )
{
    osg::Vec3f osgnormal;
    visBase::Transformation::transformNormal( transformation_, n, osgnormal );

    Threads::MutexLocker lock( mutex_ );
    osg::Vec3Array* osgnormals = mGetOsgVec3Arr( osgnormals_ );
    if( idx >= nrNormals() )
    {
	osgnormals->push_back( osgnormal ) ;
    }
    else
    {
	float xval = (*osgnormals)[idx][0];
	if( mIsUdf( xval ) )
	    (*osgnormals)[idx] = osgnormal;
	else
	    (*osgnormals)[idx] += osgnormal;
    }
}


void Normals::removeNormal(int idx)
{
    Threads::MutexLocker lock( mutex_ );
    const int nrnormals = nrNormals();

    if ( idx<0 || idx>=nrnormals )
    {
	pErrMsg("Invalid index");
	return;
    }

    osg::Vec3Array* osgnormals = mGetOsgVec3Arr( osgnormals_ );

    if ( idx==nrnormals-1 )
	osgnormals->pop_back();
    else
    {
	if ( idx>=unusednormals_.size() )
	    unusednormals_ += 1;
	(*osgnormals)[idx] = osg::Vec3f( mUdf(float),mUdf(float),mUdf(float) );
    }
}


void Normals::setAll( const float* vals, int coord3sz )
{
    Threads::MutexLocker lock( mutex_ );

    osg::Vec3Array* osgnormals = mGetOsgVec3Arr( osgnormals_ );
    if ( coord3sz!=nrNormals() )
	osgnormals->resize( coord3sz );

    int nrnormals( 0 );
    while ( nrnormals< coord3sz )
    {
	( *osgnormals )[nrnormals] =
	    osg::Vec3f( vals[nrnormals*3], vals[nrnormals*3+1],
		        vals[nrnormals*3+2] );
	nrnormals++;
    }
}


Coord3 Normals::getNormal( int idx ) const
{
    Threads::MutexLocker lock( mutex_ );
    if ( idx>=nrNormals() )
	return Coord3::udf();

    osg::Vec3Array* osgnormals = mGetOsgVec3Arr( osgnormals_ );
    const osg::Vec3f norm = ( *osgnormals )[idx];
    if ( mIsUdf( Conv::to<Coord3> (norm) ) )
	return Coord3::udf();

    Coord3 res;
    Transformation::transformBackNormal( transformation_, norm, res );

    return res;
}


void Normals::setDisplayTransformation( const mVisTrans* nt )
{
    if ( nt==transformation_ ) return;

    DoTransformation dotf( this, nrNormals(), transformation_, nt );
    TaskRunner taskrunner;
    TaskRunner::execute( &taskrunner,dotf );

    if ( transformation_ )
	transformation_->unRef();

    transformation_ = nt;

    if ( transformation_ )
	transformation_->ref();
}


void NormalListAdapter::remove( const TypeSet<int>& idxs )
{
    for ( int idx=idxs.size()-1; idx>=0; idx-- )
    {
	if ( idxs[idx]<normals_.nrNormals() )
    	    normals_.removeNormal( idxs[idx] );
    }
}


}; // namespace visBase
