/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vislight.cc,v 1.4 2003-11-07 12:22:02 bert Exp $";

#include "vislight.h"
#include "iopar.h"

#include "Inventor/nodes/SoPointLight.h"
#include "Inventor/nodes/SoDirectionalLight.h"
#include "Inventor/nodes/SoSpotLight.h"

const char* visBase::Light::isonstr = "Is On";
const char* visBase::Light::intensitystr = "Intensity";


visBase::Light::Light( SoLight* light_ )
    : light( light_ )
{ light->ref(); }


visBase::Light::~Light()
{ light->unref(); }


void visBase::Light::turnOn(bool n)
{
    light->on.setValue( n );
}


bool visBase::Light::isOn() const
{ return light->on.getValue(); }


void visBase::Light::setIntensity(float n)
{
    light->intensity.setValue( n );
}


float visBase::Light::intensity() const
{ return light->intensity.getValue(); }


SoNode* visBase::Light::getData()
{ return light; }


void visBase::Light::fillPar( IOPar& par, TypeSet<int>& storeids ) const
{
    SceneObject::fillPar( par, storeids );

    par.setYN( isonstr, isOn() );
    par.set( intensitystr, intensity() );
}


int visBase::Light::usePar( const IOPar& par )
{
    int res = SceneObject::usePar( par );
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


mCreateFactoryEntry( visBase::PointLight );

const char* visBase::PointLight::positionstr = "Position";

visBase::PointLight::PointLight()
    : Light( new SoPointLight )
{}


void visBase::PointLight::setPosition(float x, float y, float z )
{
    ((SoPointLight*) light)->location.setValue( x, y, z );
}


float visBase::PointLight::position( int dim ) const
{
    return ((SoPointLight*) light)->location.getValue()[dim];
}


void visBase::PointLight::fillPar( IOPar& par, TypeSet<int>& storeids ) const
{
    Light::fillPar( par, storeids );

    par.set( positionstr, position(0), position(1), position(2) );
}


int visBase::PointLight::usePar( const IOPar& par )
{
    int res = Light::usePar( par );
    if ( res != 1 ) return res;

    double x, y, z;
    if ( !par.get( positionstr, x, y, z ))
	return -1;

    setPosition( x, y, z );
    return 1;
}


mCreateFactoryEntry( visBase::DirectionalLight );
const char* visBase::DirectionalLight::directionstr = "Direction";


visBase::DirectionalLight::DirectionalLight()
    : Light( new SoDirectionalLight )
{ }


void visBase::DirectionalLight::setDirection(float x, float y, float z )
{
    ((SoDirectionalLight*) light)->direction.setValue( x, y, z );
}


float visBase::DirectionalLight::direction( int dim ) const
{
    return ((SoDirectionalLight*) light)->direction.getValue()[dim];
}


void visBase::DirectionalLight::fillPar( IOPar& par, TypeSet<int>& storeids ) const
{
    Light::fillPar( par, storeids );

    par.set( directionstr, direction(0), direction(1), direction(2) );
}


int visBase::DirectionalLight::usePar( const IOPar& par )
{
    int res = Light::usePar( par );
    if ( res != 1 ) return res;

    double x, y, z;
    if ( !par.get( directionstr, x, y, z ))
	return -1;

    setDirection( x, y, z );
    return 1;
}


mCreateFactoryEntry( visBase::SpotLight );

const char* visBase::SpotLight::directionstr = "Direction";
const char* visBase::SpotLight::positionstr = "Position";
const char* visBase::SpotLight::coneanglestr = "Cone Angle";
const char* visBase::SpotLight::dropoffratestr = "Drop Off Rate";

visBase::SpotLight::SpotLight()
    : Light( new SoSpotLight )
{}


void visBase::SpotLight::setDirection(float x, float y, float z )
{
    ((SoSpotLight*) light)->direction.setValue( x, y, z );
}


float visBase::SpotLight::direction( int dim ) const
{
    return ((SoSpotLight*) light)->direction.getValue()[dim];
}


void visBase::SpotLight::setPosition(float x, float y, float z )
{
    ((SoSpotLight*) light)->location.setValue( x, y, z );
}


float visBase::SpotLight::position( int dim ) const
{
    return ((SoSpotLight*) light)->location.getValue()[dim];
}


void visBase::SpotLight::setConeAngle(float n)
{
    ((SoSpotLight*) light)->cutOffAngle.setValue(n);
}


float visBase::SpotLight::coneAngle() const
{
    return ((SoSpotLight*) light)->cutOffAngle.getValue();
}


void visBase::SpotLight::setDropOffRate(float n)
{
    ((SoSpotLight*) light)->dropOffRate.setValue(n);
}


float visBase::SpotLight::dropOffRate() const
{
    return ((SoSpotLight*) light)->dropOffRate.getValue();
}


void visBase::SpotLight::fillPar( IOPar& par, TypeSet<int>& storeids ) const
{
    Light::fillPar( par, storeids );

    par.set( directionstr, direction(0), direction(1), direction(2) );
    par.set( positionstr, position(0), position(1), position(2) );
    par.set( coneanglestr, coneAngle() );
    par.set( dropoffratestr, dropOffRate() );
}


int visBase::SpotLight::usePar( const IOPar& par )
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
