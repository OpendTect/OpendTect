/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUnusedVar = "$Id$";



#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTexture3.h>
#include <Inventor/projectors/SbLineProjector.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/SoPath.h>

#include "SoTranslateRectangleDragger.h"
#include "SoTranslateRectangleDraggerGeom.h"

SO_KIT_SOURCE(SoTranslateRectangleDragger);

void SoTranslateRectangleDragger::initClass()
{
    SO_KIT_INIT_CLASS(SoTranslateRectangleDragger, SoDragger, "Dragger" );
}

SoTranslateRectangleDragger::SoTranslateRectangleDragger()
{
    SO_KIT_CONSTRUCTOR(SoTranslateRectangleDragger);

    SO_KIT_ADD_CATALOG_ENTRY(translator, SoSeparator, true,
	    			geomSeparator, "", false );
    SO_KIT_ADD_CATALOG_ENTRY(prefixgroup, SoGroup, true,
	    			geomSeparator, translator, true );

    if ( SO_KIT_IS_FIRST_INSTANCE() )
	readDefaultParts( "SoTranslateRectangleDragger.iv",
			  geombuffer, strlen( geombuffer ));

    SO_KIT_ADD_FIELD( translation, (0, 0, 0) );
    SO_KIT_ADD_FIELD( min, (-1) );
    SO_KIT_ADD_FIELD( max, (1) );

    SO_KIT_INIT_INSTANCE();

    setPartAsDefault("translator", "translateTranslateRectangle" );
    lineproj = new SbLineProjector;

    addStartCallback( &SoTranslateRectangleDragger::startCB);
    addMotionCallback( &SoTranslateRectangleDragger::motionCB);
    addValueChangedCallback(&SoTranslateRectangleDragger::valueChangedCB );

    fieldsensor =
	new SoFieldSensor( &SoTranslateRectangleDragger::fieldsensorCB, this );

    fieldsensor->setPriority( 0 );
    setUpConnections( true, true );
}


SoTranslateRectangleDragger::~SoTranslateRectangleDragger()
{
    delete lineproj;
    delete fieldsensor;
}


SbBool SoTranslateRectangleDragger::setUpConnections( SbBool onoff,
						      SbBool doitalways )
{
    if ( !doitalways && connectionsSetUp==onoff )
	return onoff;

    if ( onoff )
    {
	SoDragger::setUpConnections( onoff, doitalways );

	fieldsensorCB(this, 0 );
	if ( fieldsensor->getAttachedField()!=&translation )
	    fieldsensor->attach(&translation);
    }
    else
    {
	if ( fieldsensor->getAttachedField() )
	    fieldsensor->detach();

	SoDragger::setUpConnections( onoff, doitalways );
    }

    return !(connectionsSetUp=onoff);
}


void SoTranslateRectangleDragger::startCB( void*, SoDragger* dragger )
{
    SoTranslateRectangleDragger* myself =
	(SoTranslateRectangleDragger*) dragger;

    myself->dragStart();
}


void SoTranslateRectangleDragger::motionCB( void*, SoDragger* dragger )
{
    SoTranslateRectangleDragger* myself =
	(SoTranslateRectangleDragger*) dragger;

    myself->drag();
}


void SoTranslateRectangleDragger::dragStart()
{
    SbVec3f startpt = getLocalStartingPoint();
    lineproj->setLine(SbLine(startpt, startpt + SbVec3f(1.0f, 0.0f, 0.0f)));
}


void SoTranslateRectangleDragger::drag()
{
    lineproj->setViewVolume(getViewVolume());
    lineproj->setWorkingSpace(getLocalToWorldMatrix());

    SbVec3f newhitpt = lineproj->project(getNormalizedLocaterPosition());
    SbVec3f starthitpt = getLocalStartingPoint();

    SbVec3f motion = newhitpt-starthitpt;
    SbMatrix motmat = appendTranslation(getStartMotionMatrix(), motion );
    SbVec3f trans, scale;
    SbRotation rot, scaleorient;
    motmat.getTransform(trans, rot, scale, scaleorient );

    SbVec3f mov = trans.getValue();
    if ( mov[0] > max.getValue() )
    {
	mov[0] = max.getValue();
	trans = mov;
	motmat.setTransform(trans, rot, scale, scaleorient );
    }
    else if ( mov[0] < min.getValue() )
    {
	mov[0] = min.getValue();
	trans = mov;
	motmat.setTransform(trans, rot, scale, scaleorient );
    }

    setMotionMatrix(motmat);
}


void SoTranslateRectangleDragger::valueChangedCB( void*, SoDragger* dragger )
{
    SoTranslateRectangleDragger* myself =
        (SoTranslateRectangleDragger*) dragger;

    SbMatrix motmat = myself->getMotionMatrix();
    SbVec3f trans, scale;
    SbRotation rot, scaleorient;
    motmat.getTransform(trans, rot, scale, scaleorient );
    
    myself->fieldsensor->detach();
    if ( myself->translation.getValue() != trans )
	myself->translation = trans;
    myself->fieldsensor->attach( &myself->translation );
}


void SoTranslateRectangleDragger::fieldsensorCB( void* dragger, SoSensor* )
{
    SoTranslateRectangleDragger* myself =
        (SoTranslateRectangleDragger*) dragger;

    SbMatrix motmat = myself->getMotionMatrix();
    myself->workFieldsIntoTransform(motmat);
    myself->setMotionMatrix(motmat);
}


