/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vistransform.cc,v 1.11 2003-10-09 14:12:58 nanne Exp $";

#include "vistransform.h"
#include "iopar.h"
#include "trigonometry.h"

#include "Inventor/nodes/SoMatrixTransform.h"
#include "Inventor/SbLinear.h" 

const char* visBase::Transformation::matrixstr = "Matrix Row ";

mCreateFactoryEntry( visBase::Transformation );

visBase::Transformation::Transformation()
    : transform_( new SoMatrixTransform )
{
    transform_->ref();
}


visBase::Transformation::~Transformation()
{
    transform_->unref();
}


void visBase::Transformation::setRotation( const Coord3& vec, float angle )
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
}


void visBase::Transformation::setTranslation( const Coord3& vec )
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
}


Coord3 visBase::Transformation::getTranslation() const
{
    SbVec3f translation;
    SbRotation rotation;
    SbVec3f scale;
    SbRotation scaleorientation;

    const SbMatrix matrix = transform_->matrix.getValue();
    matrix.getTransform( translation, rotation, scale, scaleorientation );
    return Coord3( translation[0], translation[1], translation[2] );
}


void visBase::Transformation::setScale( const Coord3& vec )
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
}


Coord3 visBase::Transformation::getScale() const
{
    SbVec3f translation;
    SbRotation rotation;
    SbVec3f scale;
    SbRotation scaleorientation;

    const SbMatrix matrix = transform_->matrix.getValue();
    matrix.getTransform( translation, rotation, scale, scaleorientation );
    return Coord3( scale[0], scale[1], scale[2] );
}


void visBase::Transformation::reset()
{
    setA( 1, 0, 0, 0,
	  0, 1, 0, 0,
	  0, 0, 1, 0,
	  0, 0, 0, 1 );
}


void visBase::Transformation::setA( float a11, float a12, float a13, float a14,
				   float a21, float a22, float a23, float a24,
				   float a31, float a32, float a33, float a34,
				   float a41, float a42, float a43, float a44 )
{
    transform_->matrix.setValue( a11, a21, a31, a41,
	    			a12, a22, a32, a42,
				a13, a23, a33, a43,
				a14, a24, a34, a44 );
}


Coord3 visBase::Transformation::transform( const Coord3& pos ) const
{
    const SbVec3f src( pos.x, pos.y, pos.z );
    SbVec3f dst;

    transform_->matrix.getValue().multVecMatrix( src, dst );
    if ( mIsUndefined(pos.z) )
	dst[2] = mUndefValue;

    return Coord3( dst[0], dst[1], dst[2] );
}


Coord3 visBase::Transformation::transformBack( const Coord3& pos ) const
{
    const SbVec3f src( pos.x, pos.y, pos.z );
    SbVec3f dst;

    SbMatrix inverse = transform_->matrix.getValue().inverse();
    inverse.multVecMatrix( src, dst );
    if ( mIsUndefined(pos.z) )
	dst[2] = mUndefValue;

    return Coord3( dst[0], dst[1], dst[2] );
}


void visBase::Transformation::fillPar( IOPar& par, TypeSet<int>& saveids ) const
{
    SceneObject::fillPar( par, saveids );
    const SbMat& matrix = transform_->matrix.getValue().getValue();

    BufferString key = matrixstr; key += 1; 
    par.set( key, matrix[0][0], matrix[1][0], matrix[2][0], matrix[3][0] );

    key = matrixstr; key += 2;
    par.set( key, matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1] );

    key = matrixstr; key += 3;
    par.set( key, matrix[0][2], matrix[1][2], matrix[2][2], matrix[3][2] );

    key = matrixstr; key += 4;
    par.set( key, matrix[0][3], matrix[1][3], matrix[2][3], matrix[3][3] );
}


int visBase::Transformation::usePar( const IOPar& par )
{
    int res = SceneObject::usePar( par );
    if ( res!= 1 ) return res;

    double matrix[4][4];
    BufferString key = matrixstr; key += 1; 
    SbMatrix inverse = transform_->matrix.getValue();
    if ( !par.get( key, matrix[0][0],matrix[1][0],matrix[2][0],matrix[3][0] ))
	return -1;

    key = matrixstr; key += 2;
    if ( !par.get( key, matrix[0][1],matrix[1][1],matrix[2][1],matrix[3][1] ))
	return -1;

    key = matrixstr; key += 3;
    if ( !par.get( key, matrix[0][2],matrix[1][2],matrix[2][2],matrix[3][2] ))
	return -1;

    key = matrixstr; key += 4;
    if ( !par.get( key, matrix[0][3],matrix[1][3],matrix[2][3],matrix[3][3] ))
	return -1;

    setA(   matrix[0][0], matrix[1][0], matrix[2][0], matrix[3][0],
	    matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1],
	    matrix[0][2], matrix[1][2], matrix[2][2], matrix[3][2],
	    matrix[0][3], matrix[1][3], matrix[2][3], matrix[3][3] );

    return 1;
}

		  




SoNode* visBase::Transformation::getData()
{
    return transform_;
}

