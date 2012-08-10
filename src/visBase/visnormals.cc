/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2002
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: visnormals.cc,v 1.24 2012-08-10 03:50:09 cvsaneesh Exp $";

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
    {
	unusednormals_ += 0;
	normals_->vector.set1Value( 0,
		SbVec3f(mUdf(float),mUdf(float),mUdf(float) ) );
    }
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
    {
	unusednormals_ += idy;
	normals_->vector.set1Value( idy,
		SbVec3f(mUdf(float),mUdf(float),mUdf(float) ) );
    }

    normals_->vector.set1Value( idx, SbVec3f( (float) normal.x, 
				    (float) normal.y, (float) normal.z ));
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
    normals_->vector.set1Value( res, SbVec3f( (float) normal.x, 
				    (float) normal.y, (float) normal.z ));

    return res;
}


void Normals::addNormalValue( int idx, const Vector3& n )
{
    Coord3 normal = n;
    transformNormal( transformation_, normal, true );

    Threads::MutexLocker lock( mutex_ );
    SbVec3f newnormal = normals_->vector[idx];
    bool set = idx>=normals_->vector.getNum();

    if ( !set )
    {
	newnormal = normals_->vector[idx];
	if ( mIsUdf(newnormal[0]) )
	    set = true;
	else
	{
	    newnormal[0] += (float) normal.x;
	    newnormal[1] += (float) normal.y;
	    newnormal[2] += (float) normal.z;
	}
    }

    if ( set )
    {
	newnormal[0] = (float) normal.x;
	newnormal[1] = (float) normal.y;
	newnormal[2] = (float) normal.z;
    }

    normals_->vector.set1Value( idx, newnormal );
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
    {
	unusednormals_ += idx;
	normals_->vector.set1Value( idx,
		SbVec3f(mUdf(float),mUdf(float),mUdf(float) ) );
    }
}


void Normals::setAll( const float* vals, int coord3sz )
{
    Threads::MutexLocker lock( mutex_ );

    if ( coord3sz!=normals_->vector.getNum() )
	normals_->vector.setNum( coord3sz );

    float* nms = (float*)normals_->vector.startEditing();
    float* stopptr = nms + coord3sz * 3;
    while ( nms<stopptr )
    {
 	*nms = *vals;
 	nms++;
 	vals++;
    } 
    normals_->vector.finishEditing();
}



Coord3 Normals::getNormal( int idx ) const
{
    Threads::MutexLocker lock( mutex_ );
    if ( idx>=normals_->vector.getNum() )
	return Coord3::udf();

    const SbVec3f norm = normals_->vector[idx];
    Coord3 res( norm[0], norm[1], norm[2] );
    transformNormal( transformation_, res, false );

    return res;
}


SoNode* Normals::gtInvntrNode()
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


void Normals::setDisplayTransformation( const mVisTrans* nt )
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

	normals_->vector.set1Value( idx, SbVec3f((float) res.x,
					    (float) res.y,(float) res.z) );
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
