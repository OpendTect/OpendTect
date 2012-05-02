/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUnusedVar = "$Id: vissceneobj.cc,v 1.13 2012-05-02 15:12:34 cvskris Exp $";


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
