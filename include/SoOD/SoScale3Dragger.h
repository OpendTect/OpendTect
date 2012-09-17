#ifndef SoScale3Dragger_h
#define SoScale3Dragger_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoScale3Dragger.h,v 1.7 2010/08/04 14:49:36 cvsbert Exp $
________________________________________________________________________


-*/

/*!\brief
Box shaped dragger with draggable markers in the corners and on the middle
of each face.
*/

#include <Inventor/draggers/SoDragger.h>

#include "soodbasic.h"

class SoMaterial;
class SoSwitch;
class SoFieldSensor;

class SbLineProjector;

mClass SoScale3Dragger : public SoDragger
{
    SO_KIT_HEADER( SoScale3Dragger );

    SO_KIT_CATALOG_ENTRY_HEADER(xMaxTransSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(xMinTransSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(yMaxTransSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(yMinTransSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(zMaxTransSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(zMinTransSwitch);

    SO_KIT_CATALOG_ENTRY_HEADER(trans000Switch);
    SO_KIT_CATALOG_ENTRY_HEADER(trans001Switch);
    SO_KIT_CATALOG_ENTRY_HEADER(trans010Switch);
    SO_KIT_CATALOG_ENTRY_HEADER(trans011Switch);
    SO_KIT_CATALOG_ENTRY_HEADER(trans100Switch);
    SO_KIT_CATALOG_ENTRY_HEADER(trans101Switch);
    SO_KIT_CATALOG_ENTRY_HEADER(trans110Switch);
    SO_KIT_CATALOG_ENTRY_HEADER(trans111Switch);

    SO_KIT_CATALOG_ENTRY_HEADER(xMaxTrans);
    SO_KIT_CATALOG_ENTRY_HEADER(xMinTrans);
    SO_KIT_CATALOG_ENTRY_HEADER(yMaxTrans);
    SO_KIT_CATALOG_ENTRY_HEADER(yMinTrans);
    SO_KIT_CATALOG_ENTRY_HEADER(zMaxTrans);
    SO_KIT_CATALOG_ENTRY_HEADER(zMinTrans);

    SO_KIT_CATALOG_ENTRY_HEADER(trans000);
    SO_KIT_CATALOG_ENTRY_HEADER(trans001);
    SO_KIT_CATALOG_ENTRY_HEADER(trans010);
    SO_KIT_CATALOG_ENTRY_HEADER(trans011);
    SO_KIT_CATALOG_ENTRY_HEADER(trans100);
    SO_KIT_CATALOG_ENTRY_HEADER(trans101);
    SO_KIT_CATALOG_ENTRY_HEADER(trans110);
    SO_KIT_CATALOG_ENTRY_HEADER(trans111);

    SO_KIT_CATALOG_ENTRY_HEADER(xMaxTransActive);
    SO_KIT_CATALOG_ENTRY_HEADER(xMinTransActive);
    SO_KIT_CATALOG_ENTRY_HEADER(yMaxTransActive);
    SO_KIT_CATALOG_ENTRY_HEADER(yMinTransActive);
    SO_KIT_CATALOG_ENTRY_HEADER(zMaxTransActive);
    SO_KIT_CATALOG_ENTRY_HEADER(zMinTransActive);

    SO_KIT_CATALOG_ENTRY_HEADER(trans000Active);
    SO_KIT_CATALOG_ENTRY_HEADER(trans001Active);
    SO_KIT_CATALOG_ENTRY_HEADER(trans010Active);
    SO_KIT_CATALOG_ENTRY_HEADER(trans011Active);
    SO_KIT_CATALOG_ENTRY_HEADER(trans100Active);
    SO_KIT_CATALOG_ENTRY_HEADER(trans101Active);
    SO_KIT_CATALOG_ENTRY_HEADER(trans110Active);
    SO_KIT_CATALOG_ENTRY_HEADER(trans111Active);
    SO_KIT_CATALOG_ENTRY_HEADER(wireframeMaterial);
    SO_KIT_CATALOG_ENTRY_HEADER(wireframePickStyle);
    SO_KIT_CATALOG_ENTRY_HEADER(wireframeCoords);
    SO_KIT_CATALOG_ENTRY_HEADER(wireframe);

public:
    				SoScale3Dragger();
    static void			initClass();

    SoSFVec3f			scale;
    SoSFVec3f			minScale;
    SoSFVec3f			maxScale;

protected:

    SbLineProjector*		lineProj_;

    static void			startCB( void*, SoDragger* );
    static void			motionCB( void*, SoDragger* );
    static void			valueChangedCB( void*, SoDragger* );
    static void			finishCB( void*, SoDragger* );

    void			dragStart();
    void			drag();
    void			finish();

    SoFieldSensor*		fieldsensor_;
    static void			fieldsensorCB(void*, SoSensor* );

private:
    				~SoScale3Dragger();
    SoSeparator*		createMarker( const SbVec3f&, SoMaterial*);
    void			createDefaultParts();
    SbBool			setUpConnections( SbBool, SbBool );

    SbList<SoSwitch*>		switches_;
};

#endif
