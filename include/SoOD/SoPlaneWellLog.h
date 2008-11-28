#ifndef SoPlaneWellLog_h
#define SoPlaneWellLog_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoPlaneWellLog.h,v 1.12 2008-11-28 14:50:29 cvsbruno Exp $
________________________________________________________________________

-*/

#include <Inventor/nodekits/SoBaseKit.h>

#include <Inventor/fields/SoSFBool.h>
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
class SoTriangleStripSet;
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
    				~SoPlaneWellLog();

    static void			initClass();

    void			setMaterial();
    void			setLineColor(const SbVec3f&,int);
    const SbVec3f&		lineColor(int) const;
    void			setLogFillColor(const SbVec3f&,int);
    const SbVec3f&		logFillColor(int) const;
    void			setSeisFillColor(const SbVec3f&,int);
    const SbVec3f&		seisFillColor(int) const;
    void			setLineWidth(float,int);
    float			lineWidth(int) const;
    void			showLog(bool,int);
    bool			logShown(int) const;
    void			clearLog(int);
    void			setRevScale( bool yn, int lognr ) 
    				{ (lognr == 1 ? revscale1 : revscale2) = yn; }
    void			setLogValue(int,const SbVec3f&,float,int);
    void   			setLogStyle(bool,int);
    bool 		        getLogStyle() const;
    void			setShift(float,int);
    void   			setLogFill(bool,int);

    SoMFVec3f			path1;
    SoMFVec3f			path2;
    SoMFFloat			log1;
    SoMFFloat			log2;
    SoSFFloat			maxval1;
    SoSFFloat			maxval2;
    SoSFFloat			minval1;
    SoSFFloat			minval2;
    SoSFFloat			shift1;
    SoSFFloat			shift2;
    SoSFBool  			style1;
    SoSFBool  			style2;
    SoSFBool  			filling1;
    SoSFBool  			filling2;
    SoSFFloat			screenWidth;

    SO_KIT_CATALOG_ENTRY_HEADER(topSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(lineshape1);
    SO_KIT_CATALOG_ENTRY_HEADER(lineshape2);
    SO_KIT_CATALOG_ENTRY_HEADER(line1Switch);
    SO_KIT_CATALOG_ENTRY_HEADER(group1);
    SO_KIT_CATALOG_ENTRY_HEADER(col1);
    SO_KIT_CATALOG_ENTRY_HEADER(coltri1);
    SO_KIT_CATALOG_ENTRY_HEADER(drawstyle1);
    SO_KIT_CATALOG_ENTRY_HEADER(coords1);
    SO_KIT_CATALOG_ENTRY_HEADER(lineset1);
    SO_KIT_CATALOG_ENTRY_HEADER(trishape1);
    SO_KIT_CATALOG_ENTRY_HEADER(trishape2);
    SO_KIT_CATALOG_ENTRY_HEADER(coordtri1);
    SO_KIT_CATALOG_ENTRY_HEADER(triset1);
    SO_KIT_CATALOG_ENTRY_HEADER(material1);
    SO_KIT_CATALOG_ENTRY_HEADER(line2Switch);
    SO_KIT_CATALOG_ENTRY_HEADER(group2);
    SO_KIT_CATALOG_ENTRY_HEADER(col2);
    SO_KIT_CATALOG_ENTRY_HEADER(drawstyle2);
    SO_KIT_CATALOG_ENTRY_HEADER(coords2);
    SO_KIT_CATALOG_ENTRY_HEADER(lineset2);
    SO_KIT_CATALOG_ENTRY_HEADER(coordtri2);
    SO_KIT_CATALOG_ENTRY_HEADER(triset2);
    SO_KIT_CATALOG_ENTRY_HEADER(material2);

    void			GLRender(SoGLRenderAction*);

protected:

    bool			valchanged;
    int				currentres;
    float			worldwidth;
    bool			revscale1, revscale2;
    int 			lognr;

    
    SoFieldSensor*		valuesensor;
    static void			valueChangedCB(void*,SoSensor*);
    
    
    void			buildLog(int,const SbVec3f&,int);
    void			fillTriangles(const int, const bool,float,float,
	   				      SoCoordinate3*,SbVec3f&,SbVec3f&);
    void			fillLogTriangles(const int,SoCoordinate3*,SbVec3f&,SbVec3f&);
    SbVec3f 			getProjCoords(const SoMFVec3f&,const int,
	  				      const SbVec3f&, const SoSFFloat&,
					      const float, int lognr);
    SbVec3f			getNormal(const SbVec3f&,const SbVec3f&,
	    				  const SbVec3f&);
    bool			shouldGLRender(int);
    int				getResolution(SoState*);
};

#endif

