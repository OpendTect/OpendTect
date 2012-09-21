/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "vislight.h"
#include "iopar.h"
#include "keystrs.h"

#include <Inventor/nodes/SoPointLight.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoSpotLight.h>
#include <Inventor/nodes/SoLightModel.h>


mCreateFactoryEntry( visBase::PointLight );
mCreateFactoryEntry( visBase::DirectionalLight );
mCreateFactoryEntry( visBase::SpotLight );
mCreateFactoryEntry( visBase::LightModel );

namespace visBase
{

const char* Light::isonstr()  { return "Is On"; }
const char* Light::intensitystr()  { return "Intensity"; }


Light::Light( SoLight* light )
    : light_( light )
    , ison_( true )
    , intensity_( 1 )
{ light_->ref(); }


Light::~Light()
{ light_->unref(); }


void Light::turnOn(bool n)
{
    ison_ = n;
    light_->intensity.setValue( ison_ ? intensity_ : 0 );
}


bool Light::isOn() const
{ return ison_; }


void Light::setIntensity(float n)
{
    intensity_ = n;
    light_->intensity.setValue( ison_ ? intensity_ : 0 );
}


float Light::intensity() const
{ return intensity_; }


SoNode* Light::gtInvntrNode()
{ return light_; }


void Light::fillPar( IOPar& par, TypeSet<int>& storeids ) const
{
    DataObject::fillPar( par, storeids );

    par.setYN( isonstr(), isOn() );
    par.set( intensitystr(), intensity() );
}


int Light::usePar( const IOPar& par )
{
    int res = DataObject::usePar( par );
    if ( res != 1 ) return res;

    bool yn;
    if ( !par.getYN( isonstr(), yn ))
	return -1;

    turnOn( yn );

    double intens;
    if ( !par.get( intensitystr(), intens ))
	return -1;

    setIntensity( (float) intens );
    return 1;
}


const char* PointLight::positionstr() { return sKey::Position(); }

PointLight::PointLight()
    : Light( new SoPointLight )
{}


void PointLight::setPosition(float x, float y, float z )
{
    ((SoPointLight*) light_)->location.setValue( x, y, z );
}


float PointLight::position( int dim ) const
{
    return ((SoPointLight*) light_)->location.getValue()[dim];
}


void PointLight::fillPar( IOPar& par, TypeSet<int>& storeids ) const
{
    Light::fillPar( par, storeids );

    par.set( positionstr(), position(0), position(1), position(2) );
}


int PointLight::usePar( const IOPar& par )
{
    int res = Light::usePar( par );
    if ( res != 1 ) return res;

    double x, y, z;
    if ( !par.get( positionstr(), x, y, z ))
	return -1;

    setPosition( (float) x, (float) y, (float) z );
    return 1;
}


const char* DirectionalLight::directionstr() { return "Direction"; }


DirectionalLight::DirectionalLight()
    : Light( new SoDirectionalLight )
{ }


void DirectionalLight::setDirection(float x, float y, float z )
{
    ((SoDirectionalLight*) light_)->direction.setValue( x, y, z );
}


float DirectionalLight::direction( int dim ) const
{
    return ((SoDirectionalLight*) light_)->direction.getValue()[dim];
}


void DirectionalLight::fillPar( IOPar& par, TypeSet<int>& storeids ) const
{
    Light::fillPar( par, storeids );

    par.set( directionstr(), direction(0), direction(1), direction(2) );
}


int DirectionalLight::usePar( const IOPar& par )
{
    int res = Light::usePar( par );
    if ( res != 1 ) return res;

    double x, y, z;
    if ( !par.get( directionstr(), x, y, z ))
	return -1;

    setDirection( (float) x, (float) y, (float) z );
    return 1;
}


const char* SpotLight::directionstr()  { return "Direction"; }
const char* SpotLight::positionstr()   { return sKey::Position(); }
const char* SpotLight::coneanglestr()  { return "Cone Angle"; }
const char* SpotLight::dropoffratestr(){ return "Drop Off Rate"; }

SpotLight::SpotLight()
    : Light( new SoSpotLight )
{}


void SpotLight::setDirection(float x, float y, float z )
{
    ((SoSpotLight*) light_)->direction.setValue( x, y, z );
}


float SpotLight::direction( int dim ) const
{
    return ((SoSpotLight*) light_)->direction.getValue()[dim];
}


void SpotLight::setPosition(float x, float y, float z )
{
    ((SoSpotLight*) light_)->location.setValue( x, y, z );
}


float SpotLight::position( int dim ) const
{
    return ((SoSpotLight*) light_)->location.getValue()[dim];
}


void SpotLight::setConeAngle(float n)
{
    ((SoSpotLight*) light_)->cutOffAngle.setValue(n);
}


float SpotLight::coneAngle() const
{
    return ((SoSpotLight*) light_)->cutOffAngle.getValue();
}


void SpotLight::setDropOffRate(float n)
{
    ((SoSpotLight*) light_)->dropOffRate.setValue(n);
}


float SpotLight::dropOffRate() const
{
    return ((SoSpotLight*) light_)->dropOffRate.getValue();
}


void SpotLight::fillPar( IOPar& par, TypeSet<int>& storeids ) const
{
    Light::fillPar( par, storeids );

    par.set( directionstr(), direction(0), direction(1), direction(2) );
    par.set( positionstr(), position(0), position(1), position(2) );
    par.set( coneanglestr(), coneAngle() );
    par.set( dropoffratestr(), dropOffRate() );
}


int SpotLight::usePar( const IOPar& par )
{
    int res = Light::usePar( par );
    if ( res != 1 ) return res;

    double x, y, z;
    if ( !par.get( directionstr(), x, y, z ))
	return -1;

    setDirection( (float) x, (float) y, (float) z );

    if ( !par.get( positionstr(), x, y, z ))
	return -1;

    setPosition( (float) x, (float) y, (float) z );

    if ( !par.get( coneanglestr(), x ))
	return -1;

    setConeAngle( (float) x );

    if ( !par.get( dropoffratestr(), x ))
	return -1;

    setDropOffRate( (float) x );

    return 1;
}

/// Light Model

LightModel::LightModel()
    : lightmodel_(new SoLightModel())
{
    lightmodel_->ref();
    setModel( BaseColor );
}


LightModel::~LightModel()
{
    lightmodel_->unref();
}


void LightModel::setModel( Type tp )
{
    switch( tp )
    {
    case BaseColor:
	lightmodel_->model = SoLightModel::BASE_COLOR;
	break;
    case Phong:
	lightmodel_->model = SoLightModel::PHONG;
	break;
    }
}


LightModel::Type LightModel::getModel() const
{
    return lightmodel_->model.getValue() == SoLightModel::BASE_COLOR
	? BaseColor : Phong;
}


SoNode* LightModel::gtInvntrNode()
{
    return lightmodel_;
}

} // namespace visBase
