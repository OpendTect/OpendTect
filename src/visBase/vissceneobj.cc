
/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: vissceneobj.cc,v 1.6 2002-03-11 10:46:03 kristofer Exp $";

#include "vissceneobj.h"

const SoNode* visBase::SceneObject::getData() const
{ return const_cast<const SoNode*>(((visBase::SceneObject*)this)->getData() ); }
