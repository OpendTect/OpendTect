/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vismaterial.cc,v 1.10 2005-02-04 14:31:34 kristofer Exp $";

#include "vismaterial.h"
#include "color.h"
#include "iopar.h"

#include "Inventor/nodes/SoMaterial.h"

namespace visBase
{

mCreateFactoryEntry( Material );

const char* Material::colorstr = "Color";
const char* Material::ambiencestr = "Ambient Intensity";
const char* Material::diffintensstr = "Diffuse Intensity";
const char* Material::specintensstr = "Specular Intensity";
const char* Material::emmintensstr = "Emmissive Intensity";
const char* Material::shininessstr = "Shininess";
const char* Material::transpstr = "Transparency";

Material::Material()
    : material( new SoMaterial )
{
    material->ref();
    setMinNrOfMaterials(0);
    updateMaterial(0);
}


Material::~Material()
{ material->unref(); }


#define mSetGetProperty( Type, func, var ) \
void Material::set##func( Type n, int idx ) \
{ \
    setMinNrOfMaterials(idx); \
    var[idx] = n; \
    updateMaterial( idx ); \
} \
Type Material::get##func( int idx ) const \
{ \
    if ( idx>=0 && idx<var.size() ) \
	return var[idx]; \
    return var[0]; \
}


mSetGetProperty( const Color&, Color, color );
mSetGetProperty( float, Ambience, ambience );
mSetGetProperty( float, DiffIntensity, diffuseintencity );
mSetGetProperty( float, SpecIntensity, specularintensity );
mSetGetProperty( float, EmmIntensity, emmissiveintensity );
mSetGetProperty( float, Shininess, shininess );
mSetGetProperty( float, Transparency, transparency );


void Material::updateMaterial(int idx)
{
    material->ambientColor.set1Value( idx, color[idx].r() * ambience[idx]/255,
				    color[idx].g() * ambience[idx]/255,
				    color[idx].b() * ambience[idx]/255 );

    material->diffuseColor.set1Value( idx,
		color[idx].r() * diffuseintencity[idx]/255,
		color[idx].g() * diffuseintencity[idx]/255,
		color[idx].b() * diffuseintencity[idx]/255 );

    material->specularColor.set1Value( idx,
		color[idx].r() * specularintensity[idx]/255,
		color[idx].g() * specularintensity[idx]/255,
		color[idx].b() * specularintensity[idx]/255 );

    material->emissiveColor.set1Value( idx,
		color[idx].r() * emmissiveintensity[idx]/255,
		color[idx].g() * emmissiveintensity[idx]/255,
		color[idx].b() * emmissiveintensity[idx]/255);

    material->shininess.set1Value(idx, shininess[idx] );
    material->transparency.set1Value(idx, transparency[idx] );
}


void Material::setMinNrOfMaterials(int minnr)
{
    while ( color.size()<=minnr )
    {
	color += Color(179,179,179);
	ambience += 0.8;
	diffuseintencity += 0.8;
	specularintensity += 0;
	emmissiveintensity += 0;
	shininess += 0;
	transparency += 0;
    }
}


SoNode* Material::getInventorNode() { return material; }


int Material::usePar( const IOPar& iopar )
{
    int res = DataObject::usePar( iopar );
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


void Material::fillPar( IOPar& iopar, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( iopar, saveids );

    Color tmpcolor = getColor();
    iopar.set( colorstr, tmpcolor.r(), tmpcolor.g(), tmpcolor.b() ) ;
    iopar.set( ambiencestr, getAmbience() );
    iopar.set( diffintensstr, getDiffIntensity() );
    iopar.set( specintensstr, getSpecIntensity() );
    iopar.set( emmintensstr, getEmmIntensity() );
    iopar.set( shininessstr, getShininess() );
    iopar.set( transpstr, getTransparency() );
}

}; // namespace visBase
