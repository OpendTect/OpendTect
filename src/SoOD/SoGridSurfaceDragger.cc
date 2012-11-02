/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Marc Gerritsen
 Date:          15-04-2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "SoGridSurfaceDragger.h"

#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoRotationXYZ.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/projectors/SbLineProjector.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/SbLinear.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoRotation.h>


SO_KIT_SOURCE( SoGridSurfaceDragger );


void SoGridSurfaceDragger::initClass()
{
    SO_KIT_INIT_CLASS(SoGridSurfaceDragger, SoDragger, "SoGridSurfaceDragger");
}


SoGridSurfaceDragger::SoGridSurfaceDragger()
{
    SO_KIT_CONSTRUCTOR( SoGridSurfaceDragger );

    SO_KIT_ADD_CATALOG_ENTRY( translator, SoTranslation, FALSE,
                              geomSeparator, "", TRUE );
    SO_KIT_ADD_CATALOG_ENTRY( rotator, SoRotation, FALSE,
                              geomSeparator, "", FALSE );
    SO_KIT_ADD_CATALOG_ENTRY( material, SoMaterial, FALSE,
                              geomSeparator, "", TRUE );
    SO_KIT_ADD_CATALOG_ENTRY( mainseparator, SoSeparator, FALSE,
	    		      geomSeparator, "", TRUE );

    SO_KIT_ADD_CATALOG_ENTRY( cylinder, SoCylinder, FALSE,
                              mainseparator, "", TRUE );
    SO_KIT_ADD_CATALOG_ENTRY( cube, SoCube, FALSE,
                              mainseparator, "", TRUE );

    SO_KIT_ADD_CATALOG_ENTRY( topcylinderseparator, SoSeparator, FALSE,
	    		      mainseparator, "", TRUE );
    SO_KIT_ADD_CATALOG_ENTRY( topcylindertranslator, SoTranslation, FALSE,
	    		      topcylinderseparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY( topcylinder, SoCylinder, FALSE,
	    		      topcylinderseparator, "", TRUE );

    SO_KIT_ADD_CATALOG_ENTRY( bottomcylinderseparator, SoSeparator, FALSE,
	    		      mainseparator, "", TRUE );
    SO_KIT_ADD_CATALOG_ENTRY( bottomcylindertranslator, SoTranslation, FALSE,
	    		      bottomcylinderseparator, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY( bottomcylinder, SoCylinder, FALSE,
	    		      bottomcylinderseparator, "", TRUE );

    SO_KIT_ADD_FIELD( translation, (0.0f, 0.0f, 0.0f) );
    SO_KIT_ADD_FIELD( rotation, (1, 1, 1, 1) );
    SO_KIT_INIT_INSTANCE();

    SoMaterial* materialpart = SO_GET_ANY_PART(this, "material", SoMaterial);
    materialpart->diffuseColor.setValue(1,1,1);
    materialpart->emissiveColor.setValue(0.3,0.3,0.3);

    SoCylinder* cylinderpart = SO_GET_ANY_PART(this, "cylinder", SoCylinder);
    cylinderpart->radius.setValue( 3.0f );
    cylinderpart->height.setValue( 150.0f );

    SoCube* cubepart = SO_GET_ANY_PART(this, "cube", SoCube);
    cubepart->width.setValue(25.0);
    cubepart->depth.setValue(25.0);
    cubepart->height.setValue(0.1);

    SoCylinder* topcylinderpart =
                SO_GET_ANY_PART(this, "topcylinder", SoCylinder);
    topcylinderpart->radius.setValue( 15.0f );
    topcylinderpart->height.setValue( 10.0f );

    SoTranslation* topcylindertranslation =
               SO_GET_ANY_PART(this, "topcylindertranslator", SoTranslation);
    topcylindertranslation->translation.setValue( 0, 75.0f, 0 );

    SoCylinder* bottomcylinderpart =
                SO_GET_ANY_PART(this, "bottomcylinder", SoCylinder);
    bottomcylinderpart->radius.setValue( 15.0f );
    bottomcylinderpart->height.setValue( 10.0f );

    SoTranslation* bottomcylindertranslation =
               SO_GET_ANY_PART(this, "bottomcylindertranslator", SoTranslation);
    bottomcylindertranslation->translation.setValue( 0, -75.0f, 0 );

    lineProj =  new SbLineProjector();

    addStartCallback( &SoGridSurfaceDragger::startCB );
    addMotionCallback( &SoGridSurfaceDragger::motionCB );
    addFinishCallback( &SoGridSurfaceDragger::finishCB );

    addValueChangedCallback( &SoGridSurfaceDragger::valueChangedCB );

    translationFieldSensor = new SoFieldSensor( 
		&SoGridSurfaceDragger::translationFieldSensorCB, this );
    translationFieldSensor->setPriority(0);
    rotationFieldSensor = new SoFieldSensor( 
	    	&SoGridSurfaceDragger::rotationFieldSensorCB, this );
    rotationFieldSensor->setPriority(0);

    setUpConnections( TRUE, TRUE );
}


SoGridSurfaceDragger::~SoGridSurfaceDragger()
{
    if ( translationFieldSensor != NULL )
	delete translationFieldSensor;

    if ( rotationFieldSensor != NULL )
	delete rotationFieldSensor;
}


SbBool SoGridSurfaceDragger::setUpConnections( SbBool onOff, 
					       SbBool doItAlways )
{
    if ( !doItAlways && connectionsSetUp == onOff ) return onOff;

    if ( onOff) 
    {
        SoDragger::setUpConnections( onOff, doItAlways );

        translationFieldSensorCB(this, NULL);
        rotationFieldSensorCB(this, NULL);

        if ( translationFieldSensor->getAttachedField() != &translation )
             translationFieldSensor->attach( &translation );
        if ( rotationFieldSensor->getAttachedField() != &rotation )
             rotationFieldSensor->attach( &rotation );
    }
    else 
    {
        if ( translationFieldSensor->getAttachedField() != NULL)
             translationFieldSensor->detach();
        if ( rotationFieldSensor->getAttachedField() != NULL)
             rotationFieldSensor->detach();

        SoDragger::setUpConnections( onOff, doItAlways );
    }

    connectionsSetUp = onOff;
    return !connectionsSetUp;
}


void SoGridSurfaceDragger::startCB( void*, SoDragger* dragger )
{
    ((SoGridSurfaceDragger*)dragger)->dragStart();
}


void SoGridSurfaceDragger::motionCB( void*, SoDragger* dragger)
{
    ((SoGridSurfaceDragger*)dragger)->drag();
}


void SoGridSurfaceDragger::finishCB( void*, SoDragger* dragger)
{
    ((SoGridSurfaceDragger*)dragger)->dragFinish();
}

void SoGridSurfaceDragger::dragStart()
{
    SbVec3f hitPt = getLocalStartingPoint();
    SbLine line( hitPt, hitPt + SbVec3f(0.0f, 0.0f, 1.0f) );
    lineProj->setLine( line );
}


void SoGridSurfaceDragger::drag()
{
    lineProj->setViewVolume( getViewVolume() );
    lineProj->setWorkingSpace( getLocalToWorldMatrix() );

    SbVec3f projPt = lineProj->project( getNormalizedLocaterPosition() );
    SbVec3f startPt = getLocalStartingPoint();
    SbVec3f motion = projPt - startPt;
    setMotionMatrix( appendTranslation( getStartMotionMatrix(), motion ) );
}


void SoGridSurfaceDragger::dragFinish()
{}


void SoGridSurfaceDragger::valueChangedCB( void*, SoDragger* dragger )
{
    SoGridSurfaceDragger* thisp = (SoGridSurfaceDragger*)dragger;
    SbMatrix matrix = thisp->getMotionMatrix();

    SbVec3f trans, scale;
    SbRotation rot, scaleOrient;
    matrix.getTransform( trans, rot, scale, scaleOrient );
    thisp->translationFieldSensor->detach();
    if ( thisp->translation.getValue() != trans )
        thisp->translation = trans;

    thisp->translationFieldSensor->attach( &thisp->translation );
}


void SoGridSurfaceDragger::translationFieldSensorCB( void* dragger, 
						     SoSensor* )
{
    SoGridSurfaceDragger* thisp = (SoGridSurfaceDragger*)dragger;
    SbMatrix matrix = thisp->getMotionMatrix();
    SbVec3f t = thisp->translation.getValue();
    matrix[3][0] = t[0];
    matrix[3][1] = t[1];
    matrix[3][2] = t[2];
    thisp->setMotionMatrix( matrix );
}

void SoGridSurfaceDragger::rotationFieldSensorCB( void* dragger, SoSensor* )
{
    SoGridSurfaceDragger* myself = (SoGridSurfaceDragger*)dragger;
    SoRotation* rot = SO_GET_ANY_PART( myself, "rotator", SoRotation );
    SbVec4f t = myself->rotation.getValue();
    rot->rotation.setValue(t[0],t[1],t[2],t[3]);
}
