/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vismaterial.cc,v 1.9 2004-07-28 06:55:53 kristofer Exp $";

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
{
    material->ref();
    setMinNrOfMaterials(0);
    updateMaterial(0);
}


visBase::Material::~Material()
{ material->unref(); }


#define mSetGetProperty( Type, func, var ) \
void visBase::Material::set##func( Type n, int idx ) \
{ \
    setMinNrOfMaterials(idx); \
    var[idx] = n; \
    updateMaterial( idx ); \
} \
Type visBase::Material::get##func( int idx ) const \
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


void visBase::Material::updateMaterial(int idx)
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


void visBase::Material::setMinNrOfMaterials(int minnr)
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


SoNode* visBase::Material::getInventorNode() { return material; }


int visBase::Material::usePar( const IOPar& iopar )
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


void visBase::Material::fillPar( IOPar& iopar, TypeSet<int>& saveids ) const
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
