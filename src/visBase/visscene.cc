
/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visscene.cc,v 1.5 2002-03-11 10:46:03 kristofer Exp $";

#include "visscene.h"
#include "visobject.h"
#include "visselman.h"

#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoSelection.h"
#include "Inventor/nodes/SoEnvironment.h"

visBase::Scene::Scene()
    : environment( new SoEnvironment )
    , selector( new SoSelection )
{
    mDynamicCastGet( SoGroup*, rt, SceneObjectGroup::getData() );
    rt->addChild( environment );
    selector->addChild( rt );
    selector->ref();
    selector->addSelectionCallback(visBase::SelectionManager::selectCB, this );
    selector->addDeselectionCallback(
	    			visBase::SelectionManager::deSelectCB, this );
}


visBase::Scene::~Scene()
{
    removeAll();
    selector->removeSelectionCallback(
	    			visBase::SelectionManager::selectCB, this );
    selector->removeDeselectionCallback(
	    			visBase::SelectionManager::deSelectCB,this);

    selector->unref();
}



void visBase::Scene::setAmbientLight( float n )
{
    environment->ambientIntensity.setValue( n );
}


float visBase::Scene::ambientLight() const
{
    return environment->ambientIntensity.getValue();
}


void visBase::Scene::deSelectAll()
{
    selector->deselectAll();
}


SoNode* visBase::Scene::getData()
{
    return selector;
}
