/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: visnormals.cc,v 1.17 2009-07-22 16:01:45 cvsbert Exp $";

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
    for ( int idx=normals_->vector.getNum()-1; idx>=0; idx-- )
	unusednormals_ += 0;
}


Normals::~Normals()
{
    normals_->unref();
    delete &mutex_;

    if ( transformation_ ) transformation_->unRef();
}


void Normals::setNormal( int idx, const Vector3& n )
{
    Coord3 normal = n;
    transformNormal( transformation_, normal, true );

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



int Normals::addNormal( const Vector3& n )
{
    Coord3 normal = n;
    transformNormal( transformation_, normal, true );

    Threads::MutexLocker lock( mutex_ );
    const int res = getFreeIdx();
    normals_->vector.set1Value( res, SbVec3f( normal.x, normal.y, normal.z ));

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
    transformNormal( transformation_, res, false );

    return res;
}


SoNode* Normals::getInventorNode()
{ return normals_; }


void Normals::transformNormal( const Transformation* t, Coord3& n,
			       bool todisplay ) const
{
    if ( !t || !n.isDefined() ) return;

    if ( todisplay )
	n = t->transformBack( n ) - t->transformBack( Coord3(0,0,0) );
    else
	n = t->transform( n ) - t->transform( Coord3(0,0,0) );
}


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

    Threads::MutexLocker lock( mutex_ );

    const bool oldstatus = normals_->vector.enableNotify( false );
    for ( int idx=normals_->vector.getNum()-1; idx>=0; idx-- )
    {
	if ( unusednormals_.indexOf( idx )!=-1 )
	    continue;

	const SbVec3f norm = normals_->vector[idx];
	Coord3 res( norm[0], norm[1], norm[2] );
	transformNormal( transformation_, res, false );
	transformNormal( nt, res, true );

	normals_->vector.set1Value( idx, SbVec3f(res.x,res.y,res.z) );
    }

    normals_->vector.enableNotify( oldstatus );
    normals_->touch();


    if ( transformation_ )
	transformation_->unRef();

    transformation_ = nt;

    if ( transformation_ )
	transformation_->ref();
}


}; // namespace visBase
