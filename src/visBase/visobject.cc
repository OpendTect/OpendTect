
/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visobject.cc,v 1.11 2002-03-11 10:46:03 kristofer Exp $";

#include "visobject.h"
#include "vismaterial.h"
#include "visdataman.h"

#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/nodes/SoDrawStyle.h"

visBase::VisualObjectImpl::VisualObjectImpl()
    : root( new SoSeparator )
    , onoff( new SoSwitch )
    , drawstyle( new SoDrawStyle )
    , material( 0 )
{
    setMaterial( visBase::Material::create() );
    onoff->ref();
    onoff->addChild( root );
    onoff->whichChild = 0;
}


visBase::VisualObjectImpl::~VisualObjectImpl()
{
    onoff->unref();
    material->unRef();
}


void visBase::VisualObjectImpl::turnOn(bool n)
{
    onoff->whichChild = n ? 0 : SO_SWITCH_NONE;
}


bool visBase::VisualObjectImpl::isOn() const
{
    return !onoff->whichChild.getValue();
}


void visBase::VisualObjectImpl::setMaterial( Material* nm )
{
    if ( material )
    {
	root->removeChild( material->getData() );
	material->unRef();
    }

    material = nm;

    if ( material )
    {
	material->ref();
	root->insertChild( material->getData(), 0 );
    }
}


SoNode* visBase::VisualObjectImpl::getData() 
{ return onoff; }
