/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: vislight.cc,v 1.1 2002-02-09 13:39:20 kristofer Exp $";

#include "vislight.h"

#include "Inventor/nodes/SoPointLight.h"
#include "Inventor/nodes/SoDirectionalLight.h"
#include "Inventor/nodes/SoSpotLight.h"


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


visBase::SpotLight::SpotLight()
    : Light( new SoSpotLight )
{ }


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
