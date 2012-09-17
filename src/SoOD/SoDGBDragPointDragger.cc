/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          March 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: SoDGBDragPointDragger.cc,v 1.15 2010/04/27 06:15:46 cvskarthika Exp $";

#include "SoDGBDragPointDragger.h"

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/sensors/SoFieldSensor.h>
#include <Inventor/projectors/SbLineProjector.h>
#include <Inventor/projectors/SbPlaneProjector.h>

const char* SoDGBDragPointDragger::ztranslatorname_ = "zTranslator";
const char* SoDGBDragPointDragger::xytranslatorname_ = "xyTranslator";

// Node kit structure (new entries versus parent class marked with arrow prefix)
// CLASS SoDGBDragPointDragger
// -->"this"
//        "callbackList"
//        "topSeparator"
//            "motionMatrix"
//            "geomSeparator"
// -->            "zTranslator"
// -->            "xyTranslator"
// -->            "feedbackSwitch"
// -->                "zFeedback"
// -->                "xyFeedback"

SO_KIT_SOURCE(SoDGBDragPointDragger);

void SoDGBDragPointDragger::initClass()
{
    SO_KIT_INIT_CLASS(SoDGBDragPointDragger, SoDragger, "Dragger");
}


SoDGBDragPointDragger::SoDGBDragPointDragger()
{
    SO_KIT_CONSTRUCTOR(SoDGBDragPointDragger);

    SO_KIT_ADD_CATALOG_ENTRY(zTranslator, SoSeparator, true, geomSeparator, 
		    xyTranslator, true);
    SO_KIT_ADD_CATALOG_ENTRY(xyTranslator, SoSeparator, true, geomSeparator, 
		    feedbackSwitch, true);
    SO_KIT_ADD_CATALOG_ENTRY(feedbackSwitch, SoSwitch, true, geomSeparator, 
		    "", false);
    SO_KIT_ADD_CATALOG_ENTRY(zFeedback, SoSeparator, true, feedbackSwitch, 
		    xyFeedback, true);
    SO_KIT_ADD_CATALOG_ENTRY(xyFeedback, SoSeparator, true, feedbackSwitch, 
		    "", true);
   
    if (SO_KIT_IS_FIRST_INSTANCE())
 	readDefaultParts( "", draggergeometry_, 
		static_cast <int> ( strlen( draggergeometry_ ) ) );
 
    SO_KIT_ADD_FIELD(translation, (0.0f, 0.0f, 0.0f));
    SO_KIT_INIT_INSTANCE();

    // initialise the translators to inactive geometry states
    setPartAsDefault( ztranslatorname_, "dragPointZTranslatorTranslator" );
    setPartAsDefault( xytranslatorname_, "dragPointXYTranslatorTranslator" );
 
    // initialize default parts pertaining to feedback
    setPartAsDefault( "zFeedback", "dragPointZFeedback" );
    setPartAsDefault( "xyFeedback", "dragPointXYFeedback" );
    
    SoSwitch* sw;
    sw = SO_GET_ANY_PART( this, "feedbackSwitch", SoSwitch );
    SoInteractionKit::setSwitchValue( sw, SO_SWITCH_NONE );
    
    lineproj_ = new SbLineProjector();
    planeproj_ = new SbPlaneProjector();
    movecyl_ = false;

    addStartCallback( SoDGBDragPointDragger::startCB, this );
    addMotionCallback( SoDGBDragPointDragger::motionCB, this );
    addFinishCallback( SoDGBDragPointDragger::finishCB, this );
    addValueChangedCallback( SoDGBDragPointDragger::valueChangedCB );
    fieldsensor_ = new SoFieldSensor( 
		SoDGBDragPointDragger::fieldSensorCB, this );
    fieldsensor_->setPriority( 0 );

    setUpConnections( true, true );
}


SoDGBDragPointDragger::~SoDGBDragPointDragger()
{
    delete fieldsensor_;
    delete lineproj_;
    delete planeproj_;
}


SbBool SoDGBDragPointDragger::setUpConnections( SbBool onoff, 
		SbBool doitalways )
{
    if ( !doitalways && connectionsSetUp==onoff ) return onoff;

    SbBool oldval = connectionsSetUp;

    if ( onoff )
    {
	SoDragger::setUpConnections( onoff, doitalways );

	SoDGBDragPointDragger::fieldSensorCB( this, NULL );

	if ( fieldsensor_->getAttachedField() != &translation )
	    fieldsensor_->attach( &translation );
    }
    else 
    {
        if ( fieldsensor_->getAttachedField() != NULL )
	    fieldsensor_->detach();
	SoDragger::setUpConnections( onoff, doitalways );
    }
   
    connectionsSetUp = onoff;
    return oldval;
}


void SoDGBDragPointDragger::setDefaultOnNonWritingFields()
{
    zTranslator.setDefault( true );
    xyTranslator.setDefault( true );
    SoDragger::setDefaultOnNonWritingFields();
}


void SoDGBDragPointDragger::fieldSensorCB( void* d, SoSensor* )
{
    SoDGBDragPointDragger* thisp = static_cast <SoDGBDragPointDragger*> ( d );
    SbMatrix matrix = thisp->getMotionMatrix();
    thisp->workFieldsIntoTransform( matrix );
    thisp->setMotionMatrix( matrix );
}


void SoDGBDragPointDragger::valueChangedCB( void*, SoDragger* d )
{
    SoDGBDragPointDragger* thisp = static_cast <SoDGBDragPointDragger*> ( d );

    SbMatrix matrix = thisp->getMotionMatrix();
    SbVec3f t;
    t[0] = matrix[3][0];
    t[1] = matrix[3][1];
    t[2] = matrix[3][2];

    thisp->fieldsensor_->detach();
    if ( thisp->translation.getValue() != t )
	thisp->translation = t;

    thisp->fieldsensor_->attach( &thisp->translation );
}


void SoDGBDragPointDragger::dragStart()
{
    SoCylinder* cyl = SO_GET_ANY_PART( this, ztranslatorname_, SoCylinder );
    SoCube* cube = SO_GET_ANY_PART( this, xytranslatorname_, SoCube );
    
    if ( !cyl || !cube )
	return;

    // determine if the cylinder was picked or the cube
    if ( !determineDragDirection( cyl, cube ) )
	return;

    SbVec3f hitpt = getLocalStartingPoint();
    SoSwitch* sw = SO_GET_ANY_PART( this, "feedbackSwitch", SoSwitch );

    if ( movecyl_ )
    {
	SoInteractionKit::setSwitchValue( sw, 0 );
	setPartAsDefault( ztranslatorname_, 
			"dragPointZTranslatorTranslatorActive" );
 
	const SbVec3f endpt( 0.0f, 0.0f, 1.0f );
	lineproj_->setLine( SbLine( hitpt, hitpt + endpt ) );
    }
    else
    {
	SoInteractionKit::setSwitchValue( sw, 1 );
	setPartAsDefault( xytranslatorname_, 
			"dragPointXYTranslatorTranslatorActive" );
	
	const SbVec3f endpt( 0.0f, 0.0f, 1.0f );
	planeproj_->setPlane( SbPlane( endpt, hitpt ) );
    }
}


bool SoDGBDragPointDragger::determineDragDirection( const SoCylinder* cyl, 
		const SoCube* cube )
{
    // find the orientation of the dragger to the 3 axes
    SbViewVolume vw = getViewVolume();
    SbVec3f worldprojdir = vw.getProjectionDirection();
    const SbMatrix& mat = getWorldToLocalMatrix();
    SbVec3f localprojdir;
    mat.multDirMatrix( worldprojdir, localprojdir );
    localprojdir.normalize();

    bool selected = false;
    const float angletoy = fabs( localprojdir[1] );
    const float angletoz = fabs( localprojdir[2] );
    const float upperlimit = 0.7;
    const float lowerlimit = 0.3;
    
    // When the cylinder is lying flat, almost aligned to the Y axis, 
    // restrict picking the cylinder. User probably wants to move just the 
    // rectangle, but has picked the cylinder by mistake.
    //
    // When the cylinder is upright (almost along the Z axis) or horizontal
    // (almost along the X axis), restrict picking the rectangle. User 
    // probably wants to move just the cylinder but has picked the rectangle 
    // by mistake.

    if ( (angletoy<=lowerlimit) && (angletoz>=upperlimit) )
    {
	movecyl_ = false;
	selected = true;
    }
    else if ( angletoz<=lowerlimit )
    {
	movecyl_ = true;
	selected = true;
    }
        
    if ( !selected ) 
    {
        // Let the user drag as desired. Find which object the user has picked.
	const SoPath* pickpath = getPickPath();
	if ( pickpath->containsNode( cyl ) )
	{
	    movecyl_ = true;
	    selected = true;
	}
	else if ( pickpath->containsNode( cube ) )
	{
	    movecyl_ = false;
	    selected = true;
	}
    }
	
    return selected;
}


void SoDGBDragPointDragger::drag()
{
    SbVec3f projpt;
    SbProjector* proj = ( movecyl_ ) ? 
	    		(SbProjector*) lineproj_ : 
			(SbProjector*) planeproj_;
	
    proj->setViewVolume( getViewVolume() );
    proj->setWorkingSpace( getLocalToWorldMatrix() );
  
    if ( proj->tryProject( getNormalizedLocaterPosition(), 
		getProjectorEpsilon(), projpt ) )
	setMotionMatrix( appendTranslation( 
		getStartMotionMatrix(), projpt - getLocalStartingPoint() ) );
}


void SoDGBDragPointDragger::dragFinish()
{
    SoSwitch* sw = SO_GET_ANY_PART( this, "feedbackSwitch", SoSwitch );
    SoInteractionKit::setSwitchValue( sw, SO_SWITCH_NONE );

    // set the inactive part
    if ( movecyl_ )
        setPartAsDefault( ztranslatorname_, "dragPointZTranslatorTranslator" );
    else
	setPartAsDefault( xytranslatorname_, 
			"dragPointXYTranslatorTranslator" );
}


void SoDGBDragPointDragger::startCB( void* d, SoDragger* )
{
    SoDGBDragPointDragger* thisp = static_cast <SoDGBDragPointDragger*> ( d );
    thisp->dragStart();
}


void SoDGBDragPointDragger::motionCB( void* d, SoDragger* )
{
    SoDGBDragPointDragger* thisp = static_cast <SoDGBDragPointDragger*> ( d );
    thisp->drag();
}


void SoDGBDragPointDragger::finishCB( void* d, SoDragger* )
{
    SoDGBDragPointDragger* thisp = static_cast <SoDGBDragPointDragger*> ( d );
    thisp->dragFinish();
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
  "   Rotation { rotation 1 0 0  1.57 }\n"
  "   Cylinder { height 1.5 radius 0.2 }\n"
  "}\n"
  "\n"
  "DEF dragPointZTranslatorTranslator Separator {\n"
  "   USE DRAGPOINT_INACTIVE_MATERIAL\n"
  "   USE DRAGPOINT_STICK\n"
  "}\n"
  "DEF dragPointZTranslatorTranslatorActive Separator {\n"
  "   USE DRAGPOINT_ACTIVE_MATERIAL\n"
  "   USE DRAGPOINT_STICK\n"
  "}\n"
  "\n"
  "DEF DRAGPOINT_PLANE Group { Cube { width 1  height 1  depth .1 } }\n"
  "\n"
  "DEF dragPointXYTranslatorTranslator Separator {\n"
  "   USE DRAGPOINT_INACTIVE_MATERIAL\n"
  "   USE DRAGPOINT_PLANE\n"
  "}\n"
  "DEF dragPointXYTranslatorTranslatorActive Separator {\n"
  "   USE DRAGPOINT_ACTIVE_MATERIAL\n"
  "   USE DRAGPOINT_PLANE\n"
  "}\n"
  "\n"
  "DEF DRAGPOINT_FEEDBACK_LINE Group {\n"
  "   Rotation { rotation 1 0 0  1.57 }\n"
  "\n"
  "   Coordinate3 { point [ 0 -10 0, 0 10 0 ] }\n"
  "   LineSet { }\n"
  "   Transform { translation 0 10 0 }\n"
  "   DEF DRAGPOINT_FEEDBACK_ARROWHEAD Cone { height 0.5 bottomRadius 0.5 }\n"
  "   Transform { translation 0 -20 0 }\n"
  "   Rotation { rotation 0 0 1  3.14 }\n"
  "   USE DRAGPOINT_FEEDBACK_ARROWHEAD\n"
  "}\n"
  "\n"
  "DEF dragPointZFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   USE DRAGPOINT_FEEDBACK_LINE\n"
  "}\n"
  "\n"
  "DEF DRAGPOINT_FEEDBACK_PLANE Group {\n"
  "   ShapeHints { shapeType UNKNOWN_SHAPE_TYPE }\n"
  "   Coordinate3 { point [ -10 -10 0, -10 10 0, 10 10 0, 10 -10 0, -10 -10 0 ] }\n"
  "   FaceSet { }\n"
  "   Scale { scaleFactor 1.05 1 1.05 }\n"
  "   LineSet { }\n"
  "}\n"
  "\n"
  "DEF DRAGPOINT_FEEDBACK_PLANE_AXES Group {\n"
  "   DrawStyle { lineWidth 2 }\n"
  "   Coordinate3 { point [ -3 0 0, 3 0 0, 0 -3 0, 0 3 0 ] }\n"
  "   LineSet { numVertices [2, 2] }\n"
  "}\n"
  "\n"
  "DEF dragPointXYFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   USE DRAGPOINT_FEEDBACK_PLANE\n"
  "   USE DRAGPOINT_ACTIVE_MATERIAL\n"
  "   USE DRAGPOINT_FEEDBACK_PLANE_AXES\n"
  "}\n"
  "\n";

