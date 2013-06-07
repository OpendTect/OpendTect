#ifndef SoDGBDragPointDragger_h
#define SoDGBDragPointDragger_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          March 2010
 RCS:           $Id$
________________________________________________________________________


-*/

#include "soodbasic.h"


/*!\brief
This class is basically a SoDragPointDragger, which determines whether the user wants to move the cylinder or the plane depending on the view angle. This is 
helpful especially when the dragger is very small and the user cannot 
differentiate between its parts. The cylinder lies along the Z axis and the 
cube in the XY plane.
*/

#include <Inventor/draggers/SoDragger.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/fields/SoSFBool.h>

class SoSensor;
class SoFieldSensor;
class SbLineProjector;
class SbPlaneProjector;
class SoCylinder;
class SoCube;

mClass SoDGBDragPointDragger : public SoDragger {

    SO_KIT_HEADER(SoDGBDragPointDragger);
    SO_KIT_CATALOG_ENTRY_HEADER(zTranslator);
    SO_KIT_CATALOG_ENTRY_HEADER(xyTranslator);
    SO_KIT_CATALOG_ENTRY_HEADER(feedbackSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(zFeedback);
    SO_KIT_CATALOG_ENTRY_HEADER(xyFeedback);
    
public:
    static void		initClass(void);
    			SoDGBDragPointDragger(void);

    SoSFVec3f		translation;

protected:
    virtual 		~SoDGBDragPointDragger(void);
    virtual SbBool 	setUpConnections(SbBool onoff, SbBool doitalways);
    virtual void 	setDefaultOnNonWritingFields(void);

    void 		dragStart();
    void 		drag();
    void 		dragFinish();

    static void 	startCB(void*, SoDragger*);
    static void 	motionCB(void*, SoDragger*);
    static void 	finishCB(void*, SoDragger*);
    static void 	fieldSensorCB(void*, SoSensor*);
    static void 	valueChangedCB(void*, SoDragger*);
 
    SoFieldSensor* 	fieldsensor_;

private:
    bool		determineDragDirection(const SoCylinder*, 
		    				const SoCube*);

    SbLineProjector*	lineproj_;
    SbPlaneProjector*	planeproj_;
    bool		movecyl_;
    
    static const char* 	draggergeometry_;
    static const char*	ztranslatorname_;
    static const char*	xytranslatorname_;

};

#endif

