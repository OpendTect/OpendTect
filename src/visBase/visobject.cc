
/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visobject.cc,v 1.2 2002-02-07 14:15:22 kristofer Exp $";

#include "visobject.h"
#include "viscolortable.h"

#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoSwitch.h"
#include "Inventor/nodes/SoDrawStyle.h"
#include "Inventor/nodes/SoMaterial.h"
#include "Inventor/nodes/SoMaterialBinding.h"
#include "Inventor/nodes/SoGroup.h"

visBase::VisualObject::VisualObject()
    : root( new SoSeparator )
    , onoff( new SoSwitch )
    , material( new SoSwitch )
    , drawstyle( new SoDrawStyle )
    , defaultmaterial( new SoMaterial )
    , colortablematerial( new SoMaterial )
    , usecolortable( false )
    , ambience( 0.2 )
    , diffuseintencity( 0.8 )
    , specularintensity( 0 )
    , emmissiveintensity( 0 )
    , shininess( 0.2 )
    , transparency( 0 )
    , colortable( 0 )
{
    onoff->ref();
    onoff->addChild( root );
    onoff->whichChild = 0;

    SoGroup* defmaterialgroup = new SoGroup;
    defmaterialgroup->addChild( defaultmaterial );
    SoMaterialBinding* defaultbinidng = new SoMaterialBinding;
    defaultbinidng->value = SoMaterialBinding::OVERALL;
    defmaterialgroup->addChild( defaultbinidng );

    material->addChild( defmaterialgroup );
    material->addChild( colortablematerial );
    material->whichChild = 0;
    root->addChild( material );

    root->addChild( drawstyle );
    updateMaterial();
}


visBase::VisualObject::~VisualObject()
{
    onoff->unref();
    delete colortable;
}


void visBase::VisualObject::on()
{
    onoff->whichChild = 0;
}


void visBase::VisualObject::off()
{
    onoff->whichChild = SO_SWITCH_NONE;
}


bool visBase::VisualObject::isOn() const
{
    return !onoff->whichChild.getValue();
}


void visBase::VisualObject::setColor( const Color& nc )
{ color = nc; }
    


void visBase::VisualObject::switchColorMode( bool totable )
{
    material->whichChild = totable ? 1 : 0;
    if ( totable && !colortable ) colortable = new ColorTable;
}


bool  visBase::VisualObject::isColorTable() const
{
    return material->whichChild.getValue();
}


SoNode* visBase::VisualObject::getData() { return onoff; }


void visBase::VisualObject::updateMaterial()
{
    defaultmaterial->ambientColor.setValue( color.r() * ambience/255,
					    color.g() * ambience/255,
					    color.b() * ambience/255 );

    defaultmaterial->diffuseColor.setValue( color.r() * diffuseintencity/255,
					    color.g() * diffuseintencity/255,
					    color.b() * diffuseintencity/255 );

    defaultmaterial->specularColor.setValue( color.r() * specularintensity/255,
					    color.g() * specularintensity/255,
					    color.b() * specularintensity/255 );

    defaultmaterial->emissiveColor.setValue( color.r() * emmissiveintensity/255,
					    color.g() * emmissiveintensity/255,
					    color.b() * emmissiveintensity/255);

    defaultmaterial->shininess = shininess;
    defaultmaterial->transparency = transparency;
}

