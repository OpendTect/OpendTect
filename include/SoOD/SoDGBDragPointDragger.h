#ifndef SoDGBDragPointDragger_h
#define SoDGBDragPointDragger_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          March 2010
 RCS:           $Id: SoDGBDragPointDragger.h,v 1.7 2010-04-20 12:20:14 cvskarthika Exp $
________________________________________________________________________


-*/

#include "soodbasic.h"


/*!\brief
This class is basically a SoDGBDragPointDragger, which overcomes the 
undesirable effects of the SoDGBDragPointDragger when it is very small. See 
src file for more details.
*/

#include <Inventor/draggers/SoDragger.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/fields/SoSFBool.h>

class SoSensor;
class SoFieldSensor;
class SbLineProjector;
class SbPlaneProjector;

mClass SoDGBDragPointDragger : public SoDragger {

    SO_KIT_HEADER(SoDGBDragPointDragger);
    SO_KIT_CATALOG_ENTRY_HEADER(noRotSep);
    SO_KIT_CATALOG_ENTRY_HEADER(planeFeedbackSep);
    SO_KIT_CATALOG_ENTRY_HEADER(planeFeedbackSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(planeFeedbackTranslation);
    SO_KIT_CATALOG_ENTRY_HEADER(rotX);
    SO_KIT_CATALOG_ENTRY_HEADER(rotXSep);
    SO_KIT_CATALOG_ENTRY_HEADER(rotY);
    SO_KIT_CATALOG_ENTRY_HEADER(rotYSep);
    SO_KIT_CATALOG_ENTRY_HEADER(rotZ);
    SO_KIT_CATALOG_ENTRY_HEADER(rotZSep);
    SO_KIT_CATALOG_ENTRY_HEADER(xFeedback);
    SO_KIT_CATALOG_ENTRY_HEADER(xFeedbackSep);
    SO_KIT_CATALOG_ENTRY_HEADER(xFeedbackSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(xFeedbackTranslation);
    SO_KIT_CATALOG_ENTRY_HEADER(xTranslator);
    SO_KIT_CATALOG_ENTRY_HEADER(xTranslatorSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(xyFeedback);
    SO_KIT_CATALOG_ENTRY_HEADER(xyTranslator);
    SO_KIT_CATALOG_ENTRY_HEADER(xyTranslatorSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(xzFeedback);
    SO_KIT_CATALOG_ENTRY_HEADER(xzTranslator);
    SO_KIT_CATALOG_ENTRY_HEADER(xzTranslatorSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(yFeedback);
    SO_KIT_CATALOG_ENTRY_HEADER(yFeedbackSep);
    SO_KIT_CATALOG_ENTRY_HEADER(yFeedbackSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(yFeedbackTranslation);
    SO_KIT_CATALOG_ENTRY_HEADER(yTranslator);
    SO_KIT_CATALOG_ENTRY_HEADER(yTranslatorSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(yzFeedback);
    SO_KIT_CATALOG_ENTRY_HEADER(yzTranslator);
    SO_KIT_CATALOG_ENTRY_HEADER(yzTranslatorSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(zFeedback);
    SO_KIT_CATALOG_ENTRY_HEADER(zFeedbackSep);
    SO_KIT_CATALOG_ENTRY_HEADER(zFeedbackSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(zFeedbackTranslation);
    SO_KIT_CATALOG_ENTRY_HEADER(zTranslator);
    SO_KIT_CATALOG_ENTRY_HEADER(zTranslatorSwitch);

public:
    static void		initClass(void);
    			SoDGBDragPointDragger(void);

    SoSFVec3f		translation;

    // This field specifies whether dragging should be restricted to the plane
    // or cylinder depending on the view angle. Behaves like COIN3D's 
    // SoDragPointDragger if false. Default is true.
    SoSFBool		restrictdragging;

protected:
    virtual 		~SoDGBDragPointDragger(void);
    virtual SbBool 	setUpConnections(SbBool onoff, 
	    			SbBool doitalways = FALSE);
    virtual void 	setDefaultOnNonWritingFields(void);

    void 		dragStart();
    void 		drag();
    void 		dragFinish();

    static void 	startCB(void * f, SoDragger * d);
    static void 	motionCB(void * f, SoDragger * d);
    static void 	finishCB(void * f, SoDragger * d);
    static void 	fieldSensorCB(void * f, SoSensor * s);
    static void 	valueChangedCB(void * f, SoDragger * d);
 
    SoFieldSensor* 	fieldSensor;

private:
    void 		updateSwitchNodes();
    bool		setObjectToDrag(SbVec3f);

    int 		curraxis_;
    SbLineProjector*	lineproj_;
    SbPlaneProjector*	planeproj_;
    SbVec3f		worldrestartpt_;
    SbVec3f		lastmotion_;
    bool		movecyl_;
    
    static const char* 	draggergeometry_;
    static const char*	linefbswitchnames_[];
    static const char*	linetranslatornames_[];
    static const char*	planetranslatornames_[];

};

#endif

