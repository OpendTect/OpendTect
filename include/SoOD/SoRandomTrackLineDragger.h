#ifndef SoRandomTrackLineDragger_h
#define SoRandomTrackLineDragger_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoRandomTrackLineDragger.h,v 1.2 2003-01-02 11:58:47 kristofer Exp $
________________________________________________________________________


-*/

#include "Inventor/nodekits/SoBaseKit.h"
#include "Inventor/fields/SoMFVec2f.h"
#include "Inventor/fields/SoSFFloat.h"
#include "Inventor/fields/SoSFVec3f.h"

class SoDragger;
class SoFieldSensor;
class SoSensor;

/*!\brief

*/

class SoRandomTrackLineDragger : public SoBaseKit
{
    SO_KIT_HEADER(SoRandomTrackLineDragger);
    SO_KIT_CATALOG_ENTRY_HEADER(subDraggerSep);
    SO_KIT_CATALOG_ENTRY_HEADER(subDraggerRot);
    SO_KIT_CATALOG_ENTRY_HEADER(subDraggerScale);
    SO_KIT_CATALOG_ENTRY_HEADER(subDraggers);
    SO_KIT_CATALOG_ENTRY_HEADER(feedbackSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(feedback);
    SO_KIT_CATALOG_ENTRY_HEADER(feedbackCoords);
    SO_KIT_CATALOG_ENTRY_HEADER(feedbackMaterial);
    SO_KIT_CATALOG_ENTRY_HEADER(feedbackShapeHints);
    SO_KIT_CATALOG_ENTRY_HEADER(feedbackStrip);

public:
    				SoRandomTrackLineDragger();
    static void			initClass();

    SoMFVec2f			knots;
    SoSFFloat			z0;
    SoSFFloat			z1;

    SoSFVec3f			xyzStart;
    SoSFVec3f			xyzStop;
    SoSFVec3f			xyzStep;

protected:
    float			xyzSnap( int dim, float ) const;

    static void			startCB( void*, SoDragger* );
    static void			motionCB( void*, SoDragger* );
    static void			finishCB( void*, SoDragger* );
    static void			fieldChangeCB( void*, SoSensor* );

    void			dragStart();
    void			drag(SoDragger*);
    void			dragFinish();

    void			updateDraggers();
    SbBool			setUpConnections(SbBool, SbBool);

    SoFieldSensor*		knotsfieldsensor;
    SoFieldSensor*		z0fieldsensor;
    SoFieldSensor*		z1fieldsensor;

private:
    				~SoRandomTrackLineDragger();
};

#endif

