#ifndef SoTranslateRectangleDragger_h
#define SoTranslateRectangleDragger_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoTranslateRectangleDragger.h,v 1.1 2002-11-08 12:22:25 kristofer Exp $
________________________________________________________________________


-*/

/*!\brief


*/

#include <Inventor/draggers/SoDragger.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/sensors/SoFieldSensor.h>

class SbLineProjector;

class SoTranslateRectangleDragger : public SoDragger
{
    SO_KIT_HEADER( SoTranslateRectangleDragger );
    
    SO_KIT_CATALOG_ENTRY_HEADER(translator);
    SO_KIT_CATALOG_ENTRY_HEADER(prefixgroup);

public:
    				SoTranslateRectangleDragger();
    static void			initClass();

    SoSFVec3f			translation;
    SoSFFloat			min;
    SoSFFloat			max;

protected:
    SbLineProjector*		lineproj;

    static void			startCB( void*, SoDragger* );
    static void			motionCB( void*, SoDragger* );

    void			dragStart();
    void			drag();

    SoFieldSensor*		fieldsensor;
    static void			fieldsensorCB(void*, SoSensor* );
    static void			valueChangedCB( void*, SoDragger* );

    virtual SbBool		setUpConnections( SbBool onoff,
	    					  SbBool doitalways = false );

private:
    static const char		geombuffer[];
    				~SoTranslateRectangleDragger();
};

#endif
