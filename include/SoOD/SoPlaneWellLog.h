#ifndef SoPlaneWellLog_h
#define SoPlaneWellLog_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoPlaneWellLog.h,v 1.1 2003-09-29 10:18:18 kristofer Exp $
________________________________________________________________________

-*/

#include "Inventor/nodekits/SoBaseKit.h"

#include "Inventor/fields/SoMFVec3f.h"
#include "Inventor/fields/SoMFVec2f.h"
#include "Inventor/fields/SoSFFloat.h"

class SoFieldSensor;
class SoSensor;

/*!\brief

*/

class SoPlaneWellLog : public SoBaseKit
{
			SO_KIT_HEADER(SoPlaneWellLog);
public:
    SoMFVec3f		wellpath;
    SoMFVec2f		values;
    SoSFFloat		maxRadius;
    SoSFFloat		clipRate;

    static void		initClass();
    			SoPlaneWellLog();

    SO_KIT_CATALOG_ENTRY_HEADER(topSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(coords);
    SO_KIT_CATALOG_ENTRY_HEADER(materials);
    SO_KIT_CATALOG_ENTRY_HEADER(faceset1);
    SO_KIT_CATALOG_ENTRY_HEADER(faceset2);

    void		GLRender(SoGLRenderAction*);

protected:
    			~SoPlaneWellLog();

    SoFieldSensor*	valuesensor;
    static void		valueChangedCB( void*, SoSensor* );

    SbList<float>	radius1;
    SbList<float>	radius2;

};

#endif

