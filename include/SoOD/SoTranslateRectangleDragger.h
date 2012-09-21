#ifndef SoTranslateRectangleDragger_h
#define SoTranslateRectangleDragger_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

/*!\brief
Is a dragger that is made up of a plane that can be dragged back and forth
along the rectangle's normal. If certain properties should be set on the plane
(such as texture coordinates (as done in the sliding planes), nodes can be
inserted in the prefixgroup.
*/

#include "soodmod.h"
#include <Inventor/draggers/SoDragger.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/sensors/SoFieldSensor.h>

#include "soodbasic.h"

class SbLineProjector;

mSoODClass SoTranslateRectangleDragger : public SoDragger
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

