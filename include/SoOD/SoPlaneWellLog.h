#ifndef SoPlaneWellLog_h
#define SoPlaneWellLog_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoPlaneWellLog.h,v 1.2 2003-10-17 14:58:44 nanne Exp $
________________________________________________________________________

-*/

#include <Inventor/nodekits/SoBaseKit.h>

#include <Inventor/fields/SoMFVec3f.h>
#include <Inventor/fields/SoMFFloat.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFShort.h>
#include <Inventor/SbLinear.h>

class SoCoordinate3;
class SoFieldSensor;
class SoLineSet;
class SoSensor;
class SoSwitch;
class SoBaseColor;

/*!\brief

*/

class SoPlaneWellLog : public SoBaseKit
{
    typedef SoBaseKit inherited;
    SO_KIT_HEADER(SoPlaneWellLog);

public:
    				SoPlaneWellLog();
    static void			initClass();

    void			setWidth(float);
    void			setLineColor(const SbVec3f&,int);
    const SbVec3f&		lineColor(int) const;
    void			showLog(bool,int);
    bool			logShown(int) const;
    void			clearLog(int);
    void			setLogValue(int,const SbVec3f&,float,int);

    SoMFVec3f			path1;
    SoMFVec3f			path2;
    SoMFFloat			log1;
    SoMFFloat			log2;
    SoSFFloat			maxval1;
    SoSFFloat			maxval2;
    SoSFFloat			width;

    SO_KIT_CATALOG_ENTRY_HEADER(topSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(line1Switch);
    SO_KIT_CATALOG_ENTRY_HEADER(group1);
    SO_KIT_CATALOG_ENTRY_HEADER(col1);
    SO_KIT_CATALOG_ENTRY_HEADER(coords1);
    SO_KIT_CATALOG_ENTRY_HEADER(lineset1);
    SO_KIT_CATALOG_ENTRY_HEADER(line2Switch);
    SO_KIT_CATALOG_ENTRY_HEADER(group2);
    SO_KIT_CATALOG_ENTRY_HEADER(col2);
    SO_KIT_CATALOG_ENTRY_HEADER(coords2);
    SO_KIT_CATALOG_ENTRY_HEADER(lineset2);

    void			GLRender(SoGLRenderAction*);

protected:
    				~SoPlaneWellLog();

    SoSwitch*			sw1ptr;
    SoSwitch*			sw2ptr;
    SoBaseColor*		col1ptr;
    SoBaseColor*		col2ptr;
    SoCoordinate3*		coord1ptr;
    SoCoordinate3*		coord2ptr;
    SoLineSet*			line1ptr;
    SoLineSet*			line2ptr;

    SoFieldSensor*		valuesensor;
    static void			valueChangedCB(void*,SoSensor*);

    void			buildLog(int,const SbVec3f&);
    SbVec3f			getNormal(const SbVec3f&,const SbVec3f&,
	    				  const SbVec3f&);
};

#endif

