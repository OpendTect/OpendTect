#ifndef SoRandomTrackLineDragger_h
#define SoRandomTrackLineDragger_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoRandomTrackLineDragger.h,v 1.1 2003-01-01 09:23:00 kristofer Exp $
________________________________________________________________________


-*/

#include "Inventor/draggers/SoDragger.h"
#include "Inventor/fields/SoMFVec2f.h"
#include "Inventor/fields/SoSFFloat.h"

/*!\brief

*/

class SoRandomTrackLineDragger : public SoDragger
{
    SO_KIT_HEADER(SoRandomTrackLineDragger);
    SO_KIT_CATALOG_ENTRY_HEADER(subDraggers);
    SO_KIT_CATALOG_ENTRY_HEADER(feedbackSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(feedback);
    SO_KIT_CATALOG_ENTRY_HEADER(feedbackCoords);
    SO_KIT_CATALOG_ENTRY_HEADER(feedbackMaterial);
    SO_KIT_CATALOG_ENTRY_HEADER(feedbackStrip);

public:
    				SoRandomTrackLineDragger();
    static void			initClass();

    SoMFVec2f			knots;
    SoSFFloat			z0;
    SoSFFloat			z1;

protected:

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

