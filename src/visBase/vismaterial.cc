/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vismaterial.cc,v 1.5 2003-08-11 11:24:34 nanne Exp $";

#include "vismaterial.h"
#include "color.h"
#include "iopar.h"

#include "Inventor/nodes/SoMaterial.h"

mCreateFactoryEntry( visBase::Material );

const char* visBase::Material::colorstr = "Color";
const char* visBase::Material::ambiencestr = "Ambient Intensity";
const char* visBase::Material::diffintensstr = "Diffuse Intensity";
const char* visBase::Material::specintensstr = "Specular Intensity";
const char* visBase::Material::emmintensstr = "Emmissive Intensity";
const char* visBase::Material::shininessstr = "Shininess";
const char* visBase::Material::transpstr = "Transparency";

visBase::Material::Material()
    : material( new SoMaterial )
    , color( *new Color )
    , ambience( 0.8 )
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


int visBase::Material::usePar( const IOPar& iopar )
{
    int res = SceneObject::usePar( iopar );
    if ( res!=1 ) return res;

    int r,g,b;

    if ( iopar.get( colorstr, r, g, b ) )
	setColor( Color( r,g,b ));

    float val;
    if ( iopar.get( ambiencestr, val ))
	setAmbience( val );

    if ( iopar.get( diffintensstr, val ))
	setDiffIntensity( val );

    if ( iopar.get( specintensstr, val ))
	setSpecIntensity( val );

    if ( iopar.get( emmintensstr, val ))
	setEmmIntensity( val );

    if ( iopar.get( shininessstr, val ))
	setShininess( val );

    if ( iopar.get( transpstr, val ))
	setTransparency( val );

    return 1;
}


void visBase::Material::fillPar( IOPar& iopar, TypeSet<int>& saveids ) const
{
    SceneObject::fillPar( iopar, saveids );

    Color color = getColor();
    iopar.set( colorstr, color.r(), color.g(), color.b() ) ;
    iopar.set( ambiencestr, getAmbience() );
    iopar.set( diffintensstr, getDiffIntensity() );
    iopar.set( specintensstr, getSpecIntensity() );
    iopar.set( emmintensstr, getEmmIntensity() );
    iopar.set( shininessstr, getShininess() );
    iopar.set( transpstr, getTransparency() );
}
