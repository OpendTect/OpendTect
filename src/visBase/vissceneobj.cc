
/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: vissceneobj.cc,v 1.1 2002-02-06 22:29:14 kristofer Exp $";

#include "vissceneobj.h"

#include "Inventor/nodes/SoSeparator.h"


visBase::SceneObjectGroup::SceneObjectGroup()
    : root ( new SoSeparator )
{
    root->ref();
    objects.allowNull();
}


visBase::SceneObjectGroup::~SceneObjectGroup()
{
    removeAll();
    root->unref();
}


int visBase::SceneObjectGroup::addObject( SceneObject* no )
{
    objects += no;
    root->addChild( no->getData() );
    return objects.size()-1;
}

void visBase::SceneObjectGroup::removeAll()
{
    deepErase( objects );
    root->removeAllChildren();
}

SoNode*  visBase::SceneObjectGroup::getData()
{ return root; }
