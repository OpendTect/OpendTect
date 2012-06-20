/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Feb 2002
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: vistransform.cc,v 1.32 2012-06-20 13:09:53 cvsjaap Exp $";

#include "vistransform.h"
#include "iopar.h"
#include "trigonometry.h"

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMatrixTransform.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/SbLinear.h>

#include <osg/MatrixTransform>

mCreateFactoryEntry( visBase::Transformation );
mCreateFactoryEntry( visBase::Rotation );

namespace visBase
{

const char* Transformation::matrixstr()  { return "Matrix Row "; }

Transformation::Transformation()
    : transform_( new SoMatrixTransform )
    , transformgroup_( new SoGroup )
    , node_( 0 )
{
    if ( doOsg() )
    {
	osggroup_ = node_ = new osg::MatrixTransform;
	osggroup_->ref();
    }
    separate_ = false;
    transformgroup_->ref();
    transformgroup_->addChild( transform_ );
}


Transformation::~Transformation()
{
    //node is unreffed in visBase::DataObjectGroup
    transformgroup_->unref();
}


void Transformation::ensureGroup()
{
    const bool setgrp = !group_;
    DataObjectGroup::ensureGroup();
    if ( group_ && setgrp )
	transformgroup_->addChild( group_ );
}


void Transformation::setRotation( const Coord3& vec, double angle )
{
    SbVec3f translation;
    SbRotation rotation;
    SbVec3f scale;
    SbRotation scaleorientation;

    SbMatrix matrix = transform_->matrix.getValue();
    matrix.getTransform( translation, rotation, scale, scaleorientation );
    rotation = SbRotation( SbVec3f( vec.x, vec.y, vec.z ), angle );
    matrix.setTransform( translation, rotation, scale, scaleorientation );

    transform_->matrix.setValue( matrix );
    if ( node_ )
    {
	osg::Matrix matrix = node_->getMatrix();
	const osg::Quat rotation( angle, osg::Vec3d(vec.x,vec.y,vec.z ) );
	matrix.setRotate( rotation );
	node_->setMatrix( matrix );
    }
}


void Transformation::setTranslation( const Coord3& vec )
{
    SbVec3f translation;
    SbRotation rotation;
    SbVec3f scale;
    SbRotation scaleorientation;

    SbMatrix matrix = transform_->matrix.getValue();
    matrix.getTransform( translation, rotation, scale, scaleorientation );
    translation = SbVec3f( vec.x, vec.y, vec.z );
    matrix.setTransform( translation, rotation, scale, scaleorientation );

    transform_->matrix.setValue( matrix );
    if ( node_ )
    {
	osg::Matrix matrix = node_->getMatrix();
	matrix.setTrans( vec.x, vec.y, vec.z );
	node_->setMatrix( matrix );
    }
}


Coord3 Transformation::getTranslation() const
{
    if ( node_ )
    {
	const osg::Matrix matrix = node_->getMatrix();
	const osg::Vec3d vec = matrix.getTrans();
	return Coord3( vec.x(), vec.y(), vec.z() );
    }

    SbVec3f translation;
    SbRotation rotation;
    SbVec3f scale;
    SbRotation scaleorientation;

    const SbMatrix matrix = transform_->matrix.getValue();
    matrix.getTransform( translation, rotation, scale, scaleorientation );
    return Coord3( translation[0], translation[1], translation[2] );
}


void Transformation::setScale( const Coord3& vec )
{
    SbVec3f translation;
    SbRotation rotation;
    SbVec3f scale;
    SbRotation scaleorientation;

    SbMatrix matrix = transform_->matrix.getValue();
    matrix.getTransform( translation, rotation, scale, scaleorientation );
    scale = SbVec3f( vec.x, vec.y, vec.z );
    matrix.setTransform( translation, rotation, scale, scaleorientation );

    transform_->matrix.setValue( matrix );
    if ( node_ )
    {
	osg::Matrix matrix = node_->getMatrix();
	matrix.makeScale( vec.x, vec.y, vec.z );
	node_->setMatrix( matrix );
    }
}


Coord3 Transformation::getScale() const
{
    if ( node_ )
    {
	const osg::Matrix matrix = node_->getMatrix();
	const osg::Vec3d vec = matrix.getScale();
	return Coord3( vec.x(), vec.y(), vec.z() );
    }

    SbVec3f translation;
    SbRotation rotation;
    SbVec3f scale;
    SbRotation scaleorientation;

    const SbMatrix matrix = transform_->matrix.getValue();
    matrix.getTransform( translation, rotation, scale, scaleorientation );
    return Coord3( scale[0], scale[1], scale[2] );
}


void Transformation::reset()
{
    if ( node_ )
    {
	node_->setMatrix( osg::Matrix::identity() );
	return;
    }
    setA( 1, 0, 0, 0,
	  0, 1, 0, 0,
	  0, 0, 1, 0,
	  0, 0, 0, 1 );

}


void Transformation::setA( double a11, double a12, double a13, double a14,
			   double a21, double a22, double a23, double a24,
			   double a31, double a32, double a33, double a34,
			   double a41, double a42, double a43, double a44 )
{
    transform_->matrix.setValue( a11, a21, a31, a41,
	    			a12, a22, a32, a42,
				a13, a23, a33, a43,
				a14, a24, a34, a44 );
    if ( node_ )
    {
	node_->setMatrix( osg::Matrix(
			    a11, a21, a31, a41,
			    a12, a22, a32, a42,
			    a13, a23, a33, a43,
			    a14, a24, a34, a44 ) );
    }
}

/*
void Transformation::setA( const SbMatrix& matrix )
{
    transform_->matrix.setValue(matrix);
}
*/


Coord3 Transformation::transform( const Coord3& pos ) const
{
    if ( node_ )
    {
	osg::Vec3d res( pos.x, pos.y, pos.z );
	transform( res );
	return Coord3( res[0], res[1], res[2] );
    }
    SbVec3f res( pos.x, pos.y, pos.z );
    transform( res );
    if ( mIsUdf(pos.z) ) res[2] = mUdf(float);

    return Coord3( res[0], res[1], res[2] );
}


void Transformation::transform( SbVec3f& res ) const
{ transform_->matrix.getValue().multVecMatrix( res, res ); } 


void Transformation::transformBack( SbVec3f& res ) const
{
    SbMatrix inverse = transform_->matrix.getValue().inverse();
    inverse.multVecMatrix( res, res );
}


void Transformation::transform( osg::Vec3d& res ) const
{
//TODO Check that we should use preMult
    if ( node_ )
    {
	res = node_->getMatrix().preMult( res );
    }
} 


void Transformation::transformBack( osg::Vec3d& res ) const
{
//TODO Check that we should use preMult, the inverse or postMult
    if ( node_ )
    {
	res = osg::Matrixd::inverse(node_->getMatrix()).preMult( res );
    }
}


Coord3 Transformation::transformBack( const Coord3& pos ) const
{
    if ( node_ )
    {
	osg::Vec3d res( pos.x, pos.y, pos.z );
	transformBack( res );
	return Coord3( res[0], res[1], res[2] );
    }
    SbVec3f res( pos.x, pos.y, pos.z );
    transformBack( res );
    if ( mIsUdf(pos.z) ) res[2] = mUdf(float);

    return Coord3( res[0], res[1], res[2] );
}


void Transformation::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( par, saveids );
    if ( node_ )
    {
	const osg::Matrix matrix = node_->getMatrix();
	BufferString key = matrixstr(); key += 1; 
	par.set( key, matrix(0,0), matrix(1,0), matrix(2,0), matrix(3,0) );

	key = matrixstr(); key += 2;
	par.set( key, matrix(0,1), matrix(1,1), matrix(2,1), matrix(3,1) );

	key = matrixstr(); key += 3;
	par.set( key, matrix(0,2), matrix(1,2), matrix(2,2), matrix(3,2) );

	key = matrixstr(); key += 4;
	par.set( key, matrix(0,3), matrix(1,3), matrix(2,3), matrix(3,3) );
	return;
    }
    const SbMat& matrix = transform_->matrix.getValue().getValue();

    BufferString key = matrixstr(); key += 1; 
    par.set( key, matrix[0][0], matrix[1][0], matrix[2][0], matrix[3][0] );

    key = matrixstr(); key += 2;
    par.set( key, matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1] );

    key = matrixstr(); key += 3;
    par.set( key, matrix[0][2], matrix[1][2], matrix[2][2], matrix[3][2] );

    key = matrixstr(); key += 4;
    par.set( key, matrix[0][3], matrix[1][3], matrix[2][3], matrix[3][3] );
}


int Transformation::usePar( const IOPar& par )
{
    int res = DataObject::usePar( par );
    if ( res!= 1 ) return res;

    double matrix[4][4];
    BufferString key = matrixstr(); key += 1; 
    SbMatrix inverse = transform_->matrix.getValue();
    if ( !par.get( key, matrix[0][0],matrix[1][0],matrix[2][0],matrix[3][0] ))
	return -1;

    key = matrixstr(); key += 2;
    if ( !par.get( key, matrix[0][1],matrix[1][1],matrix[2][1],matrix[3][1] ))
	return -1;

    key = matrixstr(); key += 3;
    if ( !par.get( key, matrix[0][2],matrix[1][2],matrix[2][2],matrix[3][2] ))
	return -1;

    key = matrixstr(); key += 4;
    if ( !par.get( key, matrix[0][3],matrix[1][3],matrix[2][3],matrix[3][3] ))
	return -1;

    setA(   matrix[0][0], matrix[1][0], matrix[2][0], matrix[3][0],
	    matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1],
	    matrix[0][2], matrix[1][2], matrix[2][2], matrix[3][2],
	    matrix[0][3], matrix[1][3], matrix[2][3], matrix[3][3] );

    return 1;
}

		  
SoNode* Transformation::gtInvntrNode()
{
    return transformgroup_;
}


Rotation::Rotation()
    : rotation_( new SoRotation )
{
    rotation_->ref();
}


Rotation::~Rotation()
{
    rotation_->unref();
}


void Rotation::set( const Coord3& vec, double angle )
{
    set( Quaternion( vec, angle ) );
}


void Rotation::set( const Quaternion& q )
{
    rotation_->rotation.setValue( q.vec_.x, q.vec_.y, q.vec_.z, q.s_ );
}


void Rotation::get( Quaternion& q ) const
{
    const float* rot = rotation_->rotation.getValue().getValue();
    q.s_ = rot[3];
    q.vec_.x = rot[0];
    q.vec_.y = rot[1];
    q.vec_.z = rot[2];
}


Coord3 Rotation::transform( const Coord3& input ) const
{
    const float* rot = rotation_->rotation.getValue().getValue();
    const Quaternion q( rot[3], rot[0], rot[1], rot[2] );
    return q.rotate( input );
}


Coord3 Rotation::transformBack( const Coord3& input ) const
{
    const float* rot = rotation_->rotation.getValue().getValue();
    const Quaternion q = Quaternion(rot[3],rot[0],rot[1],rot[2]).inverse();
    return q.rotate( input );
}


SoNode* Rotation::gtInvntrNode() { return rotation_; }



}; // namespace visBase
