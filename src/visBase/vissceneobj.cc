
/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: vissceneobj.cc,v 1.2 2002-02-09 13:39:20 kristofer Exp $";

#include "vissceneobj.h"

#include "Inventor/nodes/SoSeparator.h"


visBase::SceneObjectGroup::SceneObjectGroup(bool separate)
    : root ( separate ? (SoGroup*) new SoSeparator : new SoGroup )
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
