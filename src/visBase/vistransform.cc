/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vistransform.cc,v 1.2 2002-02-26 19:58:19 kristofer Exp $";

#include "vistransform.h"
#include "Inventor/nodes/SoMatrixTransform.h"


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
    transform->matrix.setValue( a11, a12, a13, a14,
	    			a21, a22, a23, a24,
				a31, a32, a33, a34,
				a41, a42, a43, a44 );
}


SoNode* visBase::Transformation::getData()
{
    return transform;
}

