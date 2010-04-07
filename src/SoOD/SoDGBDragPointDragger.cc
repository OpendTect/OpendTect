/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          March 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: SoDGBDragPointDragger.cc,v 1.4 2010-04-07 07:28:36 cvskarthika Exp $";

#include "SoDGBDragPointDragger.h"

#include <Inventor/SbRotation.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/draggers/SoTranslate1Dragger.h>
#include <Inventor/draggers/SoTranslate2Dragger.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>


SO_KIT_SOURCE(SoDGBDragPointDragger);

void SoDGBDragPointDragger::initClass()
{
    SO_KIT_INIT_CLASS(SoDGBDragPointDragger, SoDragger, "Dragger" );
}


SoDGBDragPointDragger::SoDGBDragPointDragger()
{
    SO_KIT_CONSTRUCTOR(SoDGBDragPointDragger);

    SO_KIT_ADD_CATALOG_ENTRY(noRotSep, SoSeparator, FALSE, topSeparator, 
	    rotXSep, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(planeFeedbackSep, SoSeparator, FALSE, 
	    topSeparator, geomSeparator, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(planeFeedbackSwitch, SoSwitch, FALSE, 
	    planeFeedbackSep, "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(planeFeedbackTranslation, SoTranslation, FALSE, 
	    planeFeedbackSep, planeFeedbackSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(rotX, SoRotation, TRUE, rotXSep, 
	    xzTranslatorSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(rotXSep, SoSeparator, FALSE, topSeparator, 
	    rotYSep, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(rotY, SoRotation, TRUE, rotYSep, 
	    zTranslatorSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(rotYSep, SoSeparator, FALSE, topSeparator, 
	    rotZSep, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(rotZ, SoRotation, TRUE, rotZSep, 
	    yTranslatorSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(rotZSep, SoSeparator, FALSE, topSeparator, 
	    xFeedbackSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(xFeedback, SoSeparator, TRUE, xFeedbackSep, 
	    "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(xFeedbackSep, SoSeparator, FALSE, xFeedbackSwitch, 
	    "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(xFeedbackSwitch, SoSwitch, FALSE, topSeparator, 
	    yFeedbackSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(xFeedbackTranslation, SoTranslation, FALSE, 
	    xFeedbackSep, xFeedback, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(xTranslator, SoSeparator, TRUE, xTranslatorSwitch, 
	    "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(xTranslatorSwitch, SoSwitch, FALSE, noRotSep, 
	    xyTranslatorSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(xyFeedback, SoSeparator, TRUE, 
	    planeFeedbackSwitch, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(xyTranslator, SoSeparator, TRUE, 
	    xyTranslatorSwitch, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(xyTranslatorSwitch, SoSwitch, FALSE, noRotSep, 
	    "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(xzFeedback, SoSeparator, TRUE, 
	    planeFeedbackSwitch, xyFeedback, TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(xzTranslator, SoSeparator, TRUE, 
	    xzTranslatorSwitch, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(xzTranslatorSwitch, SoSwitch, FALSE, rotXSep, 
	    "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(yFeedback, SoSeparator, TRUE, yFeedbackSep, 
	    "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yFeedbackSep, SoSeparator, FALSE, yFeedbackSwitch, 
	    "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(yFeedbackSwitch, SoSwitch, FALSE, topSeparator, 
	    zFeedbackSwitch, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(yFeedbackTranslation, SoTranslation, FALSE, 
	    yFeedbackSep, yFeedback, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(yTranslator, SoSeparator, TRUE, yTranslatorSwitch, 
	    "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yTranslatorSwitch, SoSwitch, FALSE, rotZSep, 
	    "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(yzFeedback, SoSeparator, TRUE, 
	    planeFeedbackSwitch, xzFeedback, TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yzTranslator, SoSeparator, TRUE, 
	    yzTranslatorSwitch, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(yzTranslatorSwitch, SoSwitch, FALSE, rotYSep, 
	    "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(zFeedback, SoSeparator, TRUE, zFeedbackSep, 
	    "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zFeedbackSep, SoSeparator, FALSE, zFeedbackSwitch, 
	    "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(zFeedbackSwitch, SoSwitch, FALSE, topSeparator, 
	    planeFeedbackSep, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(zFeedbackTranslation, SoTranslation, FALSE, 
	    zFeedbackSep, zFeedback, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(zTranslator, SoSeparator, TRUE, 
	    zTranslatorSwitch, "", TRUE);
    SO_KIT_ADD_CATALOG_ENTRY(zTranslatorSwitch, SoSwitch, FALSE, rotYSep, 
	    yzTranslatorSwitch, FALSE);

    if (SO_KIT_IS_FIRST_INSTANCE())
    {
	SoInteractionKit::readDefaultParts("dragPointDragger.iv",
		draggergeometry_,
		static_cast<int>(strlen(draggergeometry_)));
    }

    SO_KIT_ADD_FIELD(translation, (0.0f, 0.0f, 0.0f));
    SO_KIT_INIT_INSTANCE();

    // initialize default parts not contained in simple draggers
    this->setPartAsDefault("xFeedback", "dragPointXFeedback");
    this->setPartAsDefault("yFeedback", "dragPointYFeedback");
    this->setPartAsDefault("zFeedback", "dragPointZFeedback");
    this->setPartAsDefault("xyFeedback", "dragPointXYFeedback");
    this->setPartAsDefault("xzFeedback", "dragPointXZFeedback");
    this->setPartAsDefault("yzFeedback", "dragPointYZFeedback");

    // initialise the translators to inactive geometry states
    this->setPartAsDefault("xTranslator", "dragPointXTranslatorTranslator");
    this->setPartAsDefault("yTranslator", "dragPointYTranslatorTranslator");
    this->setPartAsDefault("zTranslator", "dragPointZTranslatorTranslator");
    this->setPartAsDefault("xyTranslator", "dragPointXYTranslatorTranslator");
    this->setPartAsDefault("xzTranslator", "dragPointXZTranslatorTranslator");
    this->setPartAsDefault("yzTranslator", "dragPointYZTranslatorTranslator");

    // set rotations to align draggers to their respective axis/planes
    SoRotation* xrot = new SoRotation;
    xrot->rotation.setValue(SbRotation(
		SbVec3f(1.0f, 0.0f, 0.0f), (static_cast<float>(M_PI))*0.5f));
    this->setAnyPartAsDefault("rotX", xrot);
    SoRotation* yrot = new SoRotation;
    yrot->rotation.setValue(SbRotation(
		SbVec3f(0.0f, 1.0f, 0.0f), (static_cast<float>(M_PI))*0.5f));
    this->setAnyPartAsDefault("rotY", yrot);
    SoRotation* zrot = new SoRotation;
    zrot->rotation.setValue(SbRotation(
		SbVec3f(0.0f, 0.0f, 1.0f), (static_cast<float>(M_PI))*0.5f));
    this->setAnyPartAsDefault("rotZ", zrot);

    // initialize switch nodes
    SoSwitch* sw;
    sw = SO_GET_ANY_PART(this, "planeFeedbackSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
    sw = SO_GET_ANY_PART(this, "xFeedbackSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
    sw = SO_GET_ANY_PART(this, "yFeedbackSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
    sw = SO_GET_ANY_PART(this, "zFeedbackSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);

    this->currAxis = 1;
    this->updateSwitchNodes();

    this->addStartCallback(SoDGBDragPointDragger::startCB, this);
    this->addMotionCallback(SoDGBDragPointDragger::motionCB, this);
    this->addFinishCallback(SoDGBDragPointDragger::finishCB, this);

    this->addValueChangedCallback(SoDGBDragPointDragger::valueChangedCB);
    this->fieldSensor = new SoFieldSensor(
	    SoDGBDragPointDragger::fieldSensorCB, this);
    this->fieldSensor->setPriority(0);

    this->setUpConnections(TRUE, TRUE);
}


SoDGBDragPointDragger::~SoDGBDragPointDragger()
{
    delete this->fieldSensor;
}


SbBool SoDGBDragPointDragger::setUpConnections( SbBool onoff, 
	SbBool doitalways )
{
    if (!doitalways && this->connectionsSetUp == onoff) return onoff;
    SoDragger::setUpConnections(onoff, doitalways);
    return !(this->connectionsSetUp = onoff);
}


void SoDGBDragPointDragger::setDefaultOnNonWritingFields()
{
    this->xTranslator.setDefault(TRUE);
    this->yTranslator.setDefault(TRUE);
    this->zTranslator.setDefault(TRUE);

    this->xyTranslator.setDefault(TRUE);
    this->xzTranslator.setDefault(TRUE);
    this->yzTranslator.setDefault(TRUE);

    this->planeFeedbackTranslation.setDefault(TRUE);
    this->xFeedbackTranslation.setDefault(TRUE);
    this->yFeedbackTranslation.setDefault(TRUE);
    this->zFeedbackTranslation.setDefault(TRUE);

    setDefaultOnNonWritingFields();
}


void SoDGBDragPointDragger::fieldSensorCB( void* d, SoSensor* )
{
    SoDGBDragPointDragger* thisp = static_cast<SoDGBDragPointDragger*>(d);
    SbMatrix matrix = thisp->getMotionMatrix();
    thisp->workFieldsIntoTransform(matrix);
    thisp->setMotionMatrix(matrix);
}


void SoDGBDragPointDragger::valueChangedCB( void*, SoDragger* d )
{
    SoDGBDragPointDragger* thisp = static_cast<SoDGBDragPointDragger*>(d);

    SbMatrix matrix = thisp->getMotionMatrix();
    SbVec3f t;
    t[0] = matrix[3][0];
    t[1] = matrix[3][1];
    t[2] = matrix[3][2];

    thisp->fieldSensor->detach();
    if (thisp->translation.getValue() != t) {
	thisp->translation = t;
    }
    thisp->fieldSensor->attach(&thisp->translation);
}


/*!
  Circulate the dragger's three different sets of geometry, to
  circulate the orientation of the translation axis and translation
  plane through the three principal axes.

  This function is triggered when the user taps the \c CTRL key.
 */
void SoDGBDragPointDragger::showNextDraggerSet()
{
    this->currAxis = (this->currAxis + 1) % 3;
    this->updateSwitchNodes();
}


/*! 
  Called when dragger is selected (picked) by the user.
*/
void SoDGBDragPointDragger::dragStart()
{
    // determine if the cylinder was picked or the cube
    SoCylinder* cyl = 0;
    SoCube* cube = 0;

    switch (this->currAxis) 
    {
        case 0:
		cyl = SO_GET_ANY_PART(this, "xTranslator", SoCylinder);
		cube = SO_GET_ANY_PART(this, "xyTranslator", SoCube);
		break;

	case 1:
		cyl = SO_GET_ANY_PART(this, "yTranslator", SoCylinder);
		cube = SO_GET_ANY_PART(this, "xzTranslator", SoCube);
		break;

	case 2:
		cyl = SO_GET_ANY_PART(this, "zTranslator", SoCylinder);
		cube = SO_GET_ANY_PART(this, "yzTranslator", SoCube);
		break;

	default:
		cyl = NULL;
		cube = NULL;
    }
	
    if ( !cyl || !cube )
	return;
	
    // find the orientation of the dragger to the Z-axis
    SbViewVolume vw = getViewVolume();
    SbVec3f worldprojdir = vw.getProjectionDirection();
    const SbMatrix& mat = getWorldToLocalMatrix();
    SbVec3f localprojdir;
    mat.multDirMatrix( worldprojdir, localprojdir );
    localprojdir.normalize();
    //const float angletoz = localprojdir.dot( SbVec3f( 0, 0, 1 ) );
    const float angletoz = fabs( localprojdir[2] );
    const float upperlimit = 0.8;
    const float lowerlimit = 0.2;
    bool movecyl = false;


    // When the cylinder is lying flat (almost along the Z axis), restrict 
    // picking the cylinder. User probably wants to move just the rectangle 
    // but has picked the cylinder by mistake.
    //
    // When the cylinder is upright, restrict picking the rectangle. User 
    // probably wants to move just the cylinder but has picked the rectangle 
    // by mistake.

    if ( angletoz <= lowerlimit )
	; // Do nothing. movecyl is already false.
    else if ( angletoz >= upperlimit )
	movecyl = true;
    else
    {
        // Let the user drag as desired. Find which object the user has picked.
	SbVec3f localstartpt = getLocalStartingPoint();
	static SoGetBoundingBoxAction* bboxAction = 
		new SoGetBoundingBoxAction( SbViewportRegion() );
	bboxAction->apply( cyl );
	SbBox3f cylbbox = bboxAction->getBoundingBox();
	bboxAction->resetCenter();
	bboxAction->apply( cube );
	SbBox3f cubebbox = bboxAction->getBoundingBox();
    
	bool cylhit = cylbbox.intersect( localstartpt );
	bool cubehit = cubebbox.intersect( localstartpt );

	if ( !cylhit && !cubehit )
	    return;

	if ( cylhit && cubehit )
	{
	    // to do: find out which one to pick
	}
	else if ( cylhit )
	    movecyl = true;

	/*SoRayPickAction rpa( this->getViewportRegion() );
	rpa.setPoint( localstartpt );*/	
	}
	
	SoSwitch* sw;
    
	if ( movecyl )
	{
	    // move the cylinder
	    switch (this->currAxis)
	    {
		case 0:
			sw = SO_GET_ANY_PART(this, "xFeedbackSwitch", SoSwitch);
			break;
		case 1:
			sw = SO_GET_ANY_PART(this, "yFeedbackSwitch", SoSwitch);
			break;
		case 2:
			sw = SO_GET_ANY_PART(this, "zFeedbackSwitch", SoSwitch);
			break;
		default:
			sw = NULL; 
				// Dummy assignment to avoid compiler warning.
			break;
	    };  

	    SoInteractionKit::setSwitchValue( sw, 0 );
	}
	else
	{
	    sw = SO_GET_ANY_PART(this, "planeFeedbackSwitch", SoSwitch);
	    SoInteractionKit::setSwitchValue( sw, this->currAxis );
	}	
}


/*!
  Called when user drags the mouse after picking the dragger.
*/
void SoDGBDragPointDragger::drag()
{
    // FIXME: update feedback information, pederb 20000202
    // Must do the actual moving here?? See SoDepthTabPlaneDragger::drag
}


/*!
  Called when mouse button is released after picking and interacting
  with the dragger.
*/
void SoDGBDragPointDragger::dragFinish()
{
    SoSwitch* sw;
    sw = SO_GET_ANY_PART(this, "planeFeedbackSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
    sw = SO_GET_ANY_PART(this, "xFeedbackSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
    sw = SO_GET_ANY_PART(this, "yFeedbackSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
    sw = SO_GET_ANY_PART(this, "zFeedbackSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_NONE);
}


void SoDGBDragPointDragger::startCB( void* d, SoDragger* )
{
    SoDGBDragPointDragger* thisp = static_cast<SoDGBDragPointDragger* >(d);
    thisp->dragStart();
}


void SoDGBDragPointDragger::motionCB( void* d, SoDragger* )
{
     SoDGBDragPointDragger* thisp = static_cast<SoDGBDragPointDragger* >(d);
     thisp->drag();
}


void SoDGBDragPointDragger::finishCB( void* d, SoDragger* )
{
    SoDGBDragPointDragger* thisp = static_cast<SoDGBDragPointDragger* >(d);
    thisp->dragFinish();
}


void SoDGBDragPointDragger::updateSwitchNodes()
{
    SoSwitch *sw;
    sw = SO_GET_ANY_PART(this, "xTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, this->currAxis == 0 ? 
	    0 : SO_SWITCH_NONE);
    sw = SO_GET_ANY_PART(this, "yTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, this->currAxis == 1 ? 
	    0 : SO_SWITCH_NONE);
    sw = SO_GET_ANY_PART(this, "zTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, this->currAxis == 2 ? 
	    0 : SO_SWITCH_NONE);

    sw = SO_GET_ANY_PART(this, "xyTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, this->currAxis == 2 ? 
	    0 : SO_SWITCH_NONE);
    sw = SO_GET_ANY_PART(this, "xzTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, this->currAxis == 1 ? 
	    0 : SO_SWITCH_NONE);
    sw = SO_GET_ANY_PART(this, "yzTranslatorSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, this->currAxis == 0 ? 
	    0 : SO_SWITCH_NONE);
}


const char* SoDGBDragPointDragger::draggergeometry_ =
  "#Inventor V2.1 ascii\n"
  "\n"
  "\n"
  "\n"
  "DEF DRAGPOINT_INACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0.5  emissiveColor 0.5 0.5 0.5 }\n"
  "DEF DRAGPOINT_ACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0  emissiveColor 0.5 0.5 0 }\n"
  "DEF DRAGPOINT_FEEDBACK_MATERIAL Material { diffuseColor 0.5 0 0.5  emissiveColor  0.5 0 0.5  transparency 0.2 }\n"
  "\n"
  "\n"
  "\n"
  "DEF DRAGPOINT_STICK Group {\n"
  "   Rotation { rotation 0 0 1  1.57 }\n"
  "   Cylinder { height 1.5 radius 0.2 }\n"
  "}\n"
  "\n"
  "DEF DRAGPOINT_INACTIVE_STICK Separator {\n"
  "   USE DRAGPOINT_INACTIVE_MATERIAL\n"
  "   USE DRAGPOINT_STICK\n"
  "}\n"
  "DEF DRAGPOINT_ACTIVE_STICK Separator {\n"
  "   USE DRAGPOINT_ACTIVE_MATERIAL\n"
  "   USE DRAGPOINT_STICK\n"
  "}\n"
  "\n"
  "DEF dragPointXTranslatorTranslator Separator { USE DRAGPOINT_INACTIVE_STICK } \n"
  "DEF dragPointXTranslatorTranslatorActive Separator { USE DRAGPOINT_ACTIVE_STICK }\n"
  "DEF dragPointYTranslatorTranslator Separator { USE DRAGPOINT_INACTIVE_STICK }\n"
  "DEF dragPointYTranslatorTranslatorActive Separator { USE DRAGPOINT_ACTIVE_STICK }\n"
  "DEF dragPointZTranslatorTranslator Separator { USE DRAGPOINT_INACTIVE_STICK }\n"
  "DEF dragPointZTranslatorTranslatorActive Separator { USE DRAGPOINT_ACTIVE_STICK }\n"
  "\n"
  "\n"
  "DEF DRAGPOINT_PLANE Group { Cube { width 1  height 1  depth .1 } }\n"
  "\n"
  "DEF DRAGPOINT_INACTIVE_PLANE Separator {\n"
  "   USE DRAGPOINT_INACTIVE_MATERIAL\n"
  "   USE DRAGPOINT_PLANE\n"
  "}\n"
  "DEF DRAGPOINT_ACTIVE_PLANE Separator {\n"
  "   USE DRAGPOINT_ACTIVE_MATERIAL\n"
  "   USE DRAGPOINT_PLANE\n"
  "}\n"
  "\n"
  "DEF dragPointXYTranslatorTranslator Separator { USE DRAGPOINT_INACTIVE_PLANE }\n"
  "DEF dragPointXYTranslatorTranslatorActive Separator { USE DRAGPOINT_ACTIVE_PLANE }\n"
  "DEF dragPointXZTranslatorTranslator Separator { USE DRAGPOINT_INACTIVE_PLANE }\n"
  "DEF dragPointXZTranslatorTranslatorActive Separator { USE DRAGPOINT_ACTIVE_PLANE }\n"
  "DEF dragPointYZTranslatorTranslator Separator { USE DRAGPOINT_INACTIVE_PLANE }\n"
  "DEF dragPointYZTranslatorTranslatorActive Separator { USE DRAGPOINT_ACTIVE_PLANE }\n"
  "\n"
  "\n"
  "DEF DRAGPOINT_FEEDBACK_LINE Group {\n"
  "   Coordinate3 { point [ 0 -10 0, 0 10 0 ] }\n"
  "   LineSet { }\n"
  "\n"
  "   Transform { translation 0 10 0 }\n"
  "   DEF DRAGPOINT_FEEDBACK_ARROWHEAD Cone { height 0.5 bottomRadius 0.5 }\n"
  "   Transform { translation 0 -20 0 }\n"
  "   Rotation { rotation 0 0 1  3.14 }\n"
  "   USE DRAGPOINT_FEEDBACK_ARROWHEAD\n"
  "}\n"
  "\n"
  "DEF dragPointXFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   Rotation { rotation 0 0 1 1.57 }\n"
  "   USE DRAGPOINT_FEEDBACK_LINE\n"
  "}\n"
  "DEF dragPointYFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   USE DRAGPOINT_FEEDBACK_LINE\n"
  "}\n"
  "DEF dragPointZFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   Rotation { rotation 1 0 0 1.57 }\n"
  "   USE DRAGPOINT_FEEDBACK_LINE\n"
  "}\n"
  "\n"
  "\n"
  "DEF DRAGPOINT_FEEDBACK_PLANE Group {\n"
  "   ShapeHints { shapeType UNKNOWN_SHAPE_TYPE }\n"
  "   Coordinate3 { point [ -10 0 -10, -10 0 10, 10 0 10, 10 0 -10, -10 0 -10 ] }\n"
  "   FaceSet { }\n"
  "   Scale { scaleFactor 1.05 1 1.05 }\n"
  "   LineSet { }\n"
  "}\n"
  "\n"
  "DEF dragPointXYFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   Rotation { rotation 1 0 0  1.57 }\n"
  "   USE DRAGPOINT_FEEDBACK_PLANE\n"
  "}\n"
  "DEF dragPointXZFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   USE DRAGPOINT_FEEDBACK_PLANE\n"
  "}\n"
  "DEF dragPointYZFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   Rotation { rotation 0 0 1  1.57 }\n"
  "   USE DRAGPOINT_FEEDBACK_PLANE\n"
  "}\n";

