
/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visscene.cc,v 1.1 2002-02-26 13:32:54 kristofer Exp $";

#include "visscene.h"
#include "visobject.h"
#include "visselobj.h"
#include "vissceneman.h"

#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoSelection.h"
#include "Inventor/nodes/SoEnvironment.h"

visBase::Scene::Scene()
    : selman( new SelectionManager )
    , environment( new SoEnvironment )
    , id_( visBase::SM().addScene( this ) )
{
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
