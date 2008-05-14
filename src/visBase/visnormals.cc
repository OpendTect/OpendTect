/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          Dec 2002
 RCS:           $Id: visnormals.cc,v 1.13 2008-05-14 20:27:19 cvskris Exp $
________________________________________________________________________

-*/

#include "visnormals.h"

#include "errh.h"
#include "trigonometry.h"
#include "thread.h"
#include "vistransform.h"

#include <Inventor/nodes/SoNormal.h>

mCreateFactoryEntry( visBase::Normals );

namespace visBase
{

Normals::Normals()
    : normals_( new SoNormal )
    , mutex_( *new Threads::Mutex )
    , transformation_( 0 )
{
    normals_->ref();
    unusednormals_ += 0;
    //!<To compensate for that the first coord is set by default by coin
}


Normals::~Normals()
{
    normals_->unref();
    delete &mutex_;

    if ( transformation_ ) transformation_->unRef();
}


void Normals::setNormal( int idx, const Vector3& normal )
{
    Threads::MutexLocker lock( mutex_ );

    for ( int idy=normals_->vector.getNum(); idy<idx; idy++ )
	unusednormals_ += idy;

    normals_->vector.set1Value( idx, SbVec3f( normal.x, normal.y, normal.z ));
}


int Normals::nrNormals() const
{ return normals_->vector.getNum(); }


void Normals::inverse()
{
    Threads::MutexLocker lock( mutex_ );

    SbVec3f* normals = normals_->vector.startEditing();

    for ( int idx=normals_->vector.getNum()-1; idx>=0; idx-- )
	normals[idx] *= -1;

    if ( normals_->vector.getNum() )
	normals_->vector.finishEditing();
}


int Normals::nextID( int previd ) const
{
    Threads::MutexLocker lock( mutex_ );

    const int sz = normals_->vector.getNum();

    int res = previd+1;
    while ( res<sz )
    {
	if ( unusednormals_.indexOf(res)==-1 )
	    return res;
    }

    return -1;
}



int Normals::addNormal( const Vector3& normal )
{
    Coord3 normaltoset = normal;
    if ( transformation_ && normal.isDefined() )
	normaltoset = transformation_->transformBack( normaltoset )-
	    	      transformation_->transformBack( Coord3(0,0,0) );

    Threads::MutexLocker lock( mutex_ );
    const int res = getFreeIdx();
    normals_->vector.set1Value( res,
	    SbVec3f( normaltoset.x, normaltoset.y, normaltoset.z ));

    return res;
}


void Normals::removeNormal(int idx)
{
    Threads::MutexLocker lock( mutex_ );
    const int nrnormals = normals_->vector.getNum();
    if ( idx<0 || idx>=nrnormals )
    {
	pErrMsg("Invalid index");
	return;
    }
    
    if ( idx==nrnormals-1 )
	normals_->vector.deleteValues( idx );
    else
	unusednormals_ += idx;
}


Coord3 Normals::getNormal( int idx ) const
{
    Threads::MutexLocker lock( mutex_ );
    const SbVec3f norm = normals_->vector[idx];
    Coord3 res( norm[0], norm[1], norm[2] );
    if ( transformation_ && res.isDefined() )
	return transformation_->transform( res ) -
	       transformation_->transform( Coord3(0,0,0) );

    return res;
}


SoNode* Normals::getInventorNode()
{ return normals_; }


int  Normals::getFreeIdx()
{
    if ( unusednormals_.size() )
    {
	const int res = unusednormals_[unusednormals_.size()-1];
	unusednormals_.remove(unusednormals_.size()-1);
	return res;
    }

    return normals_->vector.getNum();
}


void Normals::setDisplayTransformation( Transformation* nt )
{
    if ( nt==transformation_ ) return;

    if ( normals_->vector.getNum()-unusednormals_.size() )
    {
	pErrMsg("setting transform will not transform existing values");
    }

    if ( transformation_ )
	transformation_->unRef();

    transformation_ = nt;

    if ( transformation_ )
	transformation_->ref();
}


}; // namespace visBase
