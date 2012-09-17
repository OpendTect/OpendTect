/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vissceneobj.cc,v 1.11 2009/07/22 16:01:45 cvsbert Exp $";


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
