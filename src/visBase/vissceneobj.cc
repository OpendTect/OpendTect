
/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: vissceneobj.cc,v 1.5 2002-02-27 13:03:56 kristofer Exp $";

#include "vissceneobj.h"

#include "Inventor/nodes/SoSeparator.h"


const SoNode* visBase::SceneObject::getData() const
{ return const_cast<const SoNode*>(((visBase::SceneObject*)this)->getData() ); }


visBase::SceneObjectWrapper::SceneObjectWrapper( SoNode* n )
    : node( n )
{ node->ref(); }


visBase::SceneObjectWrapper::~SceneObjectWrapper()
{ node->unref(); }
