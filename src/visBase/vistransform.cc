/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vistransform.cc,v 1.5 2002-04-29 07:07:00 kristofer Exp $";

#include "vistransform.h"
#include "Inventor/nodes/SoMatrixTransform.h"

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


Geometry::Pos visBase::Transformation::transform(const Geometry::Pos& pos) const
{
    const SbVec3f src( pos.x, pos.y, pos.z );
    SbVec3f dst;

    transform_->matrix.getValue().multMatrixVec( src, dst );

    return Geometry::Pos( dst[0], dst[1], dst[2] );
}


Geometry::Pos visBase::Transformation::transformBack(
						const Geometry::Pos& pos) const
{
    const SbVec3f src( pos.x, pos.y, pos.z );
    SbVec3f dst;

    SbMatrix inverse = transform_->matrix.getValue().inverse();
    inverse.multMatrixVec( src, dst );

    return Geometry::Pos( dst[0], dst[1], dst[2] );
}




SoNode* visBase::Transformation::getData()
{
    return transform_;
}

