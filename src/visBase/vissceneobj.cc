/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vissceneobj.cc,v 1.10 2005-02-04 14:31:34 kristofer Exp $";


#include "vistransform.h"
#include "vissceneobj.h"

namespace visBase
{


SceneObject::SceneObject()
    : transformation( 0 )
{}


SceneObject::~SceneObject()
{
    if ( transformation ) transformation->unRef();
}


void SceneObject::setDisplayTransformation( const Transformation* t )
{
    if ( transformation ) transformation->unRef();
    transformation = t;
    if ( transformation ) transformation->ref();
}





}; // namespace visBase
