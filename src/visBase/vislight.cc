/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vislight.cc,v 1.6 2005-02-04 14:31:34 kristofer Exp $";

#include "vislight.h"
#include "iopar.h"

#include "Inventor/nodes/SoPointLight.h"
#include "Inventor/nodes/SoDirectionalLight.h"
#include "Inventor/nodes/SoSpotLight.h"

namespace visBase
{

const char* Light::isonstr = "Is On";
const char* Light::intensitystr = "Intensity";


Light::Light( SoLight* light_ )
    : light( light_ )
{ light->ref(); }


Light::~Light()
{ light->unref(); }


void Light::turnOn(bool n)
{
    light->on.setValue( n );
}


bool Light::isOn() const
{ return light->on.getValue(); }


void Light::setIntensity(float n)
{
    light->intensity.setValue( n );
}


float Light::intensity() const
{ return light->intensity.getValue(); }


SoNode* Light::getInventorNode()
{ return light; }


void Light::fillPar( IOPar& par, TypeSet<int>& storeids ) const
{
    DataObject::fillPar( par, storeids );

    par.setYN( isonstr, isOn() );
    par.set( intensitystr, intensity() );
}


int Light::usePar( const IOPar& par )
{
    int res = DataObject::usePar( par );
    if ( res != 1 ) return res;

    bool yn;
    if ( !par.getYN( isonstr, yn ))
	return -1;

    turnOn( yn );

    double intens;
    if ( !par.get( intensitystr, intens ))
	return -1;

    setIntensity( intens );
    return 1;
}


mCreateFactoryEntry( PointLight );

const char* PointLight::positionstr = "Position";

PointLight::PointLight()
    : Light( new SoPointLight )
{}


void PointLight::setPosition(float x, float y, float z )
{
    ((SoPointLight*) light)->location.setValue( x, y, z );
}


float PointLight::position( int dim ) const
{
    return ((SoPointLight*) light)->location.getValue()[dim];
}


void PointLight::fillPar( IOPar& par, TypeSet<int>& storeids ) const
{
    Light::fillPar( par, storeids );

    par.set( positionstr, position(0), position(1), position(2) );
}


int PointLight::usePar( const IOPar& par )
{
    int res = Light::usePar( par );
    if ( res != 1 ) return res;

    double x, y, z;
    if ( !par.get( positionstr, x, y, z ))
	return -1;

    setPosition( x, y, z );
    return 1;
}


mCreateFactoryEntry( DirectionalLight );
const char* DirectionalLight::directionstr = "Direction";


DirectionalLight::DirectionalLight()
    : Light( new SoDirectionalLight )
{ }


void DirectionalLight::setDirection(float x, float y, float z )
{
    ((SoDirectionalLight*) light)->direction.setValue( x, y, z );
}


float DirectionalLight::direction( int dim ) const
{
    return ((SoDirectionalLight*) light)->direction.getValue()[dim];
}


void DirectionalLight::fillPar( IOPar& par, TypeSet<int>& storeids ) const
{
    Light::fillPar( par, storeids );

    par.set( directionstr, direction(0), direction(1), direction(2) );
}


int DirectionalLight::usePar( const IOPar& par )
{
    int res = Light::usePar( par );
    if ( res != 1 ) return res;

    double x, y, z;
    if ( !par.get( directionstr, x, y, z ))
	return -1;

    setDirection( x, y, z );
    return 1;
}


mCreateFactoryEntry( SpotLight );

const char* SpotLight::directionstr = "Direction";
const char* SpotLight::positionstr = "Position";
const char* SpotLight::coneanglestr = "Cone Angle";
const char* SpotLight::dropoffratestr = "Drop Off Rate";

SpotLight::SpotLight()
    : Light( new SoSpotLight )
{}


void SpotLight::setDirection(float x, float y, float z )
{
    ((SoSpotLight*) light)->direction.setValue( x, y, z );
}


float SpotLight::direction( int dim ) const
{
    return ((SoSpotLight*) light)->direction.getValue()[dim];
}


void SpotLight::setPosition(float x, float y, float z )
{
    ((SoSpotLight*) light)->location.setValue( x, y, z );
}


float SpotLight::position( int dim ) const
{
    return ((SoSpotLight*) light)->location.getValue()[dim];
}


void SpotLight::setConeAngle(float n)
{
    ((SoSpotLight*) light)->cutOffAngle.setValue(n);
}


float SpotLight::coneAngle() const
{
    return ((SoSpotLight*) light)->cutOffAngle.getValue();
}


void SpotLight::setDropOffRate(float n)
{
    ((SoSpotLight*) light)->dropOffRate.setValue(n);
}


float SpotLight::dropOffRate() const
{
    return ((SoSpotLight*) light)->dropOffRate.getValue();
}


void SpotLight::fillPar( IOPar& par, TypeSet<int>& storeids ) const
{
    Light::fillPar( par, storeids );

    par.set( directionstr, direction(0), direction(1), direction(2) );
    par.set( positionstr, position(0), position(1), position(2) );
    par.set( coneanglestr, coneAngle() );
    par.set( dropoffratestr, dropOffRate() );
}


int SpotLight::usePar( const IOPar& par )
{
    int res = Light::usePar( par );
    if ( res != 1 ) return res;

    double x, y, z;
    if ( !par.get( directionstr, x, y, z ))
	return -1;

    setDirection( x, y, z );

    if ( !par.get( positionstr, x, y, z ))
	return -1;

    setPosition( x, y, z );

    if ( !par.get( coneanglestr, x ))
	return -1;

    setConeAngle( x );

    if ( !par.get( dropoffratestr, x ))
	return -1;

    setDropOffRate( x );

    return 1;
}

}; // namespace visBase
