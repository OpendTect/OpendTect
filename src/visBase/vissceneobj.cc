
/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: vissceneobj.cc,v 1.3 2002-02-26 17:54:21 kristofer Exp $";

#include "vissceneobj.h"

#include "Inventor/nodes/SoSeparator.h"


const SoNode* visBase::SceneObject::getData() const
{ return const_cast<const SoNode*>(((visBase::SceneObject*)this)->getData() ); }


visBase::SceneObjectWrapper::SceneObjectWrapper( SoNode* n )
    : node( n )
{ node->ref(); }


visBase::SceneObjectWrapper::~SceneObjectWrapper()
{ node->unref(); }


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
