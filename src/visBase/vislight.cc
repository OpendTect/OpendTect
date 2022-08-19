/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vislight.h"
#include "iopar.h"
#include "keystrs.h"

#include <osg/Light>

namespace visBase
{

const char* Light::sKeyIsOn()	{ return "Is on"; }
const char* Light::sKeyAmbient()  { return "Ambient"; }
const char* Light::sKeyDiffuse()  { return "Diffuse"; }
const char* Light::sKeyLightNum() { return "Light num"; }
const char* Light::sKeyDirection() { return "Direction"; }


Light::Light( )
    : light_( addAttribute( new osg::Light ) )
    , ison_( true )
    , diffuse_( 0.8f )
    , ambient_( 0.2f )
{
    light_->ref();
    initLight();
}


Light::~Light ( )
{
    light_->unref();
}


void Light::initLight()
{
    if ( !light_ ) return;
    light_->setSpotExponent( 0.0f );
    light_->setSpotCutoff( 180.0f );
    light_->setConstantAttenuation( 1.0f );
    light_->setLinearAttenuation( 0.0f );
    light_->setQuadraticAttenuation( 0.0f );
    updateLights();
}


void Light::setLightNum( int num )
{
    light_->setLightNum( num );
}


int Light::getLightNum() const
{
    return light_->getLightNum();
}

#define mSetGet( var, key ) \
void Light::set##key(float n) \
{ \
    var = n; \
    updateLights(); \
} \
 \
\
float Light::get##key() const \
{ return var; }

mSetGet( ambient_, Ambient )
mSetGet( diffuse_, Diffuse )


void Light::updateLights()
{
    float newlight = ison_ ? ambient_ : 0;
    light_->setAmbient( osg::Vec4( newlight,newlight,newlight,1.0f ) );

    newlight = ison_ ? diffuse_ : 0;
    light_->setDiffuse( osg::Vec4( newlight,newlight,newlight,1.0f ) );
    visBase::DataObject::requestSingleRedraw();
}


void Light::setDirection( float x, float y, float z )
{
    osg::Vec3 dir( x, y, z );
    dir.normalize();
    light_->setPosition( osg::Vec4( dir, 0.0 ) );
}


float Light::direction( int dim ) const
{
    const osg::Vec4 dir = light_->getPosition();
    return dir[dim];
}


void Light::fillPar( IOPar& par ) const
{
    par.setYN( sKeyIsOn(), isOn() );
    par.set( sKeyAmbient(), getAmbient() );
    par.set( sKeyDiffuse(), getDiffuse() );
    par.set( sKeyLightNum(), getLightNum() );
    par.set( sKeyDirection(), direction(0), direction(1), direction(2) );
}


bool Light::usePar( const IOPar& par )
{
    bool yn;
    if ( !par.getYN( sKeyIsOn(), yn ))
	return false;

    float diffuse;
    if ( !par.get( sKeyDiffuse(), diffuse ))
	return false;

    float ambient;
    if ( !par.get( sKeyAmbient(), ambient ))
	return false;

    int lightnum;
    if ( !par.get( sKeyLightNum(), lightnum ))
	return false;

    float dirx, diry, dirz;
    if ( !par.get( sKeyDirection(), dirx, diry, dirz ) )
	return false;

    setLightNum( lightnum );
    setDiffuse( diffuse );
    setAmbient( ambient );
    turnOn( yn );
    setDirection( dirx, diry, dirz );

    return true;
}


void Light::applyAttribute( osg::StateSet* ns, osg::StateAttribute* attr )
{
    ns->setAttributeAndModes( attr,osg::StateAttribute::ON  );
}

} // namespace visBase
