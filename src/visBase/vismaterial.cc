/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vismaterial.cc,v 1.1 2002-03-11 10:46:03 kristofer Exp $";

#include "vismaterial.h"
#include "color.h"

#include "Inventor/nodes/SoMaterial.h"

visBase::Material::Material()
    : material( new SoMaterial )
    , color( *new Color )
    , ambience( 0.2 )
    , diffuseintencity( 0.8 )
    , specularintensity( 0 )
    , emmissiveintensity( 0 )
    , shininess( 0 )
    , transparency( 0 )
{
    material->ref();
}


visBase::Material::~Material()
{
    delete &color;
    material->unref();
}


void visBase::Material::setColor( const Color& n )
{
    color = n;
    updateMaterial();
}


void visBase::Material::setAmbience( float n )
{
    ambience = n;
    updateMaterial();
}


void visBase::Material::setDiffIntensity( float n )
{
    diffuseintencity = n;
    updateMaterial();
}


void visBase::Material::setSpecIntensity( float n )
{
    specularintensity = n;
    updateMaterial();
}


void visBase::Material::setEmmIntensity( float n )
{
    emmissiveintensity = n;
    updateMaterial();
}


void visBase::Material::setShininess( float n )
{
    shininess = n;
    updateMaterial();
}


void visBase::Material::setTransparency( float n )
{
    transparency = n;
    updateMaterial();
}


void visBase::Material::updateMaterial()
{
    material->ambientColor.setValue( color.r() * ambience/255,
				    color.g() * ambience/255,
				    color.b() * ambience/255 );

    material->diffuseColor.setValue( color.r() * diffuseintencity/255,
				    color.g() * diffuseintencity/255,
				    color.b() * diffuseintencity/255 );

    material->specularColor.setValue( color.r() * specularintensity/255,
					color.g() * specularintensity/255,
					color.b() * specularintensity/255 );

    material->emissiveColor.setValue( color.r() * emmissiveintensity/255,
					color.g() * emmissiveintensity/255,
					color.b() * emmissiveintensity/255);

    material->shininess = shininess;
    material->transparency = transparency;
}


SoNode* visBase::Material::getData() { return material; }
