/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vissceneobj.cc,v 1.8 2002-07-30 11:29:06 kristofer Exp $";


#include "vistransform.h"
#include "vissceneobj.h"


visBase::SceneObject::SceneObject()
    : transformation( 0 )
{}


visBase::SceneObject::~SceneObject()
{
    if ( transformation ) transformation->unRef();
}


void visBase::SceneObject::setDisplayTransformation( const Transformation* t )
{
    if ( transformation ) transformation->unRef();
    transformation = t;
    if ( transformation ) transformation->ref();
}




