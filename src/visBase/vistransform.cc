/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vistransform.cc,v 1.4 2002-04-26 13:00:09 kristofer Exp $";

#include "vistransform.h"
#include "Inventor/nodes/SoMatrixTransform.h"

mCreateFactoryEntry( visBase::Transformation );

visBase::Transformation::Transformation()
    : transform( new SoMatrixTransform )
{
    transform->ref();
}


visBase::Transformation::~Transformation()
{
    transform->unref();
}


void visBase::Transformation::setA( float a11, float a12, float a13, float a14,
				   float a21, float a22, float a23, float a24,
				   float a31, float a32, float a33, float a34,
				   float a41, float a42, float a43, float a44 )
{
    transform->matrix.setValue( a11, a21, a31, a41,
	    			a12, a22, a32, a42,
				a13, a23, a33, a43,
				a14, a24, a34, a44 );
}


SoNode* visBase::Transformation::getData()
{
    return transform;
}

