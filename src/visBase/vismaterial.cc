/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: vismaterial.cc,v 1.24 2012-09-17 16:33:43 cvskris Exp $";

#include "vismaterial.h"
#include "visosg.h"
#include "iopar.h"

#include <Inventor/nodes/SoMaterial.h>

#include <osg/Material>
#include <osg/Array>

mCreateFactoryEntry( visBase::Material );

namespace visBase
{

const char* Material::sKeyColor()		{ return "Color"; }
const char* Material::sKeyAmbience()		{ return "Ambient Intensity"; }
const char* Material::sKeyDiffIntensity()	{ return "Diffuse Intensity"; }
const char* Material::sKeySpectralIntensity()	{ return "Specular Intensity"; }
const char* Material::sKeyEmmissiveIntensity()	{ return "Emmissive Intensity";}
const char* Material::sKeyShininess()		{ return "Shininess"; }
const char* Material::sKeyTransparency()	{ return "Transparency"; }

Material::Material()
    : coinmaterial_( new SoMaterial )
    , material_( new osg::Material )
    , colorarray_( 0 )
    , ambience_( 0.8 )
    , specularintensity_( 0 )
    , emmissiveintensity_( 0 )
    , shininess_( 0 )
    , change( this )
{
    material_->ref();
    coinmaterial_->ref();
    setMinNrOfMaterials(0);
    updateMaterial(0);
}


Material::~Material()
{
    material_->unref();
    if ( colorarray_ ) colorarray_->unref();
    coinmaterial_->unref();
}
    

#define mSetProp( prop ) prop = mat.prop
void Material::setFrom( const Material& mat )
{
    mSetProp( color_ );
    mSetProp( diffuseintencity_ );
    mSetProp( ambience_ );
    mSetProp( specularintensity_ );
    mSetProp( emmissiveintensity_ );
    mSetProp( shininess_ );
    mSetProp( transparency_ );

    for ( int idx=color_.size()-1; idx>=0; idx-- )
	updateMaterial( idx );
}


void Material::setColor( const Color& n, int idx )
{
    setMinNrOfMaterials(idx);
    if ( color_[idx]==n )
	return;

    color_[idx] = n;
    updateMaterial( idx );
}


const Color& Material::getColor( int idx ) const
{
    if ( idx>=0 && idx<color_.size() )
	return color_[idx];
    return color_[0];
}


void Material::setDiffIntensity( float n, int idx )
{
    setMinNrOfMaterials(idx);
    diffuseintencity_[idx] = n;
    updateMaterial( idx );
}


float Material::getDiffIntensity( int idx ) const
{
    if ( idx>=0 && idx<diffuseintencity_.size() )
	return diffuseintencity_[idx];
    return diffuseintencity_[0];
}


void Material::setTransparency( float n, int idx )
{
    setMinNrOfMaterials(idx);
    transparency_[idx] = n;
    updateMaterial( idx );
}


float Material::getTransparency( int idx ) const
{
    if ( idx>=0 && idx<transparency_.size() )
	return transparency_[idx];
    return transparency_[0];
}


#define mSetGetProperty( Type, func, var ) \
void Material::set##func( Type n ) \
{ \
    var = n; \
    updateMaterial( 0 ); \
} \
Type Material::get##func() const \
{ \
    return var; \
}


mSetGetProperty( float, Ambience, ambience_ );
mSetGetProperty( float, SpecIntensity, specularintensity_ );
mSetGetProperty( float, EmmIntensity, emmissiveintensity_ );
mSetGetProperty( float, Shininess, shininess_ );


void Material::updateMaterial(int idx)
{
    if ( !idx )
    {
	coinmaterial_->ambientColor.set1Value( 0, color_[0].r() * ambience_/255,
					     color_[0].g() * ambience_/255,
					     color_[0].b() * ambience_/255 );
	coinmaterial_->specularColor.set1Value( 0,
		    color_[0].r() * specularintensity_/255,
		    color_[0].g() * specularintensity_/255,
		    color_[0].b() * specularintensity_/255 );

	coinmaterial_->emissiveColor.set1Value( idx,
		    color_[0].r() * emmissiveintensity_/255,
		    color_[0].g() * emmissiveintensity_/255,
		    color_[0].b() * emmissiveintensity_/255);

	coinmaterial_->shininess.set1Value(0, shininess_ );
    }

    coinmaterial_->transparency.set1Value( idx, transparency_[idx] );
    coinmaterial_->diffuseColor.set1Value( idx,
		color_[idx].r() * diffuseintencity_[idx]/255,
		color_[idx].g() * diffuseintencity_[idx]/255,
		color_[idx].b() * diffuseintencity_[idx]/255 );

    change.trigger();
}


int Material::nrOfMaterial() const
{ return color_.size(); }


void Material::setMinNrOfMaterials(int minnr)
{
    while ( color_.size()<=minnr )
    {
	color_ += Color(179,179,179);
	diffuseintencity_ += 0.8;
	transparency_ += 0.0;
    }
}


SoNode* Material::gtInvntrNode() { return coinmaterial_; }


int Material::usePar( const IOPar& iopar )
{
    int res = DataObject::usePar( iopar );
    if ( res!=1 ) return res;

    int r,g,b;

    if ( iopar.get( sKeyColor(), r, g, b ) )
	setColor( Color( r,g,b ));

    float val;
    if ( iopar.get( sKeyAmbience(), val ))
	setAmbience( val );

    if ( iopar.get( sKeyDiffIntensity(), val ))
	setDiffIntensity( val );

    if ( iopar.get( sKeySpectralIntensity(), val ))
	setSpecIntensity( val );

    if ( iopar.get( sKeyEmmissiveIntensity(), val ))
	setEmmIntensity( val );

    if ( iopar.get( sKeyShininess(), val ))
	setShininess( val );

    if ( iopar.get( sKeyTransparency(), val ))
	setTransparency( val );

    return 1;
}


void Material::fillPar( IOPar& iopar, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( iopar, saveids );

    Color tmpcolor = getColor();
    iopar.set( sKeyColor(), tmpcolor.r(), tmpcolor.g(), tmpcolor.b() ) ;
    iopar.set( sKeyAmbience(), getAmbience() );
    iopar.set( sKeyDiffIntensity(), getDiffIntensity() );
    iopar.set( sKeySpectralIntensity(), getSpecIntensity() );
    iopar.set( sKeyEmmissiveIntensity(), getEmmIntensity() );
    iopar.set( sKeyShininess(), getShininess() );
    iopar.set( sKeyTransparency(), getTransparency() );
}
    
    
const osg::Material* Material::getMaterial() const
{ return material_; }
    
    
const osg::Array* Material::getColorArray() const
{
    return colorarray_;
}
    

void Material::createArray()
{
    if ( colorarray_ )
	return;
    
    colorarray_ = new osg::Vec4Array;
    colorarray_->ref();
    mGetOsgVec4Arr(colorarray_)->
        push_back( material_->getDiffuse( osg::Material::FRONT ) );
}

}; // namespace visBase
