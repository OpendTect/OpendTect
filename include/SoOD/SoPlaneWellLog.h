#ifndef SoPlaneWellLog_h
#define SoPlaneWellLog_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoPlaneWellLog.h,v 1.9 2008-08-25 10:28:56 cvsnanne Exp $
________________________________________________________________________

-*/

#include <Inventor/nodekits/SoBaseKit.h>

#include <Inventor/fields/SoMFVec3f.h>
#include <Inventor/fields/SoMFFloat.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFShort.h>
#include <Inventor/SbLinear.h>

class SoBaseColor;
class SoCoordinate3;
class SoDrawStyle;
class SoFieldSensor;
class SoLineSet;
class SoSensor;
class SoState;
class SoSwitch;

/*!\brief

*/

class SoPlaneWellLog : public SoBaseKit
{
    typedef SoBaseKit inherited;
    SO_KIT_HEADER(SoPlaneWellLog);

public:
    				SoPlaneWellLog();
    static void			initClass();

    void			setLineColor(const SbVec3f&,int);
    const SbVec3f&		lineColor(int) const;
    void			setLineWidth(float,int);
    float			lineWidth(int) const;
    void			showLog(bool,int);
    bool			logShown(int) const;
    void			clearLog(int);
    void			setRevScale( bool yn, int lognr ) 
    				{ (lognr == 1 ? revscale1 : revscale2) = yn; }
    void			setLogValue(int,const SbVec3f&,float,int);

    SoMFVec3f			path1;
    SoMFVec3f			path2;
    SoMFFloat			log1;
    SoMFFloat			log2;
    SoSFFloat			maxval1;
    SoSFFloat			maxval2;
    SoSFFloat			screenWidth;

    SO_KIT_CATALOG_ENTRY_HEADER(topSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(line1Switch);
    SO_KIT_CATALOG_ENTRY_HEADER(group1);
    SO_KIT_CATALOG_ENTRY_HEADER(col1);
    SO_KIT_CATALOG_ENTRY_HEADER(drawstyle1);
    SO_KIT_CATALOG_ENTRY_HEADER(coords1);
    SO_KIT_CATALOG_ENTRY_HEADER(lineset1);
    SO_KIT_CATALOG_ENTRY_HEADER(line2Switch);
    SO_KIT_CATALOG_ENTRY_HEADER(group2);
    SO_KIT_CATALOG_ENTRY_HEADER(col2);
    SO_KIT_CATALOG_ENTRY_HEADER(drawstyle2);
    SO_KIT_CATALOG_ENTRY_HEADER(coords2);
    SO_KIT_CATALOG_ENTRY_HEADER(lineset2);

    void			GLRender(SoGLRenderAction*);

protected:
    				~SoPlaneWellLog();

    bool			valchanged;
    int				currentres;
    float			worldwidth;
    bool			revscale1, revscale2;

    SoFieldSensor*		valuesensor;
    static void			valueChangedCB(void*,SoSensor*);

    void			buildLog(int,const SbVec3f&,int);
    SbVec3f			getNormal(const SbVec3f&,const SbVec3f&,
	    				  const SbVec3f&);

    bool			shouldGLRender(int);
    int				getResolution(SoState*);
};

#endif

