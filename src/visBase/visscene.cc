
/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visscene.cc,v 1.4 2002-02-28 08:39:55 kristofer Exp $";

#include "visscene.h"
#include "visobject.h"
#include "visselman.h"
#include "vissceneman.h"

#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoSelection.h"
#include "Inventor/nodes/SoEnvironment.h"

visBase::Scene::Scene()
    : selman( new SelectionManager )
    , environment( new SoEnvironment )
    , id_( visBase::SM().addScene( this ) )
{
    selman->setPolicy( visBase::SelectionManager::Single );
    mDynamicCastGet( SoGroup*, rt, SceneObjectGroup::getData() );
    rt->addChild( environment );
    selman->getNode()->addChild( rt );
}


visBase::Scene::~Scene()
{
    visBase::SM().removeScene( this );
    removeAll();
    delete selman;
}


int visBase::Scene::addObject( SceneObject* obj )
{
    mDynamicCastGet(visBase::VisualObject*, visobj, obj );
    if ( visobj ) visobj->regForSelection();

    return visBase::SceneObjectGroup::addObject( obj );
}


void visBase::Scene::setAmbientLight( float n )
{
    environment->ambientIntensity.setValue( n );
}


float visBase::Scene::ambientLight() const
{
    return environment->ambientIntensity.getValue();
}

SoNode* visBase::Scene::getData()
{
    return (SoNode*)selman->getNode();
}
