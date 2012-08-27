#ifndef SoPlaneWellLog_h
#define SoPlaneWellLog_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoPlaneWellLog.h,v 1.32 2012-08-27 13:16:48 cvskris Exp $
________________________________________________________________________

-*/

#include "soodmod.h"
#include <Inventor/nodekits/SoBaseKit.h>

#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoMFVec3f.h>
#include <Inventor/fields/SoMFFloat.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFShort.h>
#include <Inventor/SbLinear.h>
#include <Inventor/SbTime.h>

#include "soodbasic.h"

class SoBaseColor;
class SoCoordinate3;
class SoDrawStyle;
class SoFieldSensor;
class SoTimerSensor;
class SoLineSet;
class SoTriangleStripSet;
class SoSensor;
class SoState;
class SoSwitch;

/*!\brief

*/

mSoODClass SoPlaneWellLog : public SoBaseKit
{
    typedef SoBaseKit inherited;
    SO_KIT_HEADER(SoPlaneWellLog);

public:
    				SoPlaneWellLog();
    				~SoPlaneWellLog();

    static void			initClass();

    void			setMaterial();
    void			resetLogData(int);
    void			setLineColor(const SbVec3f&,int);
    const SbVec3f&		lineColor(int) const;
    void			setFilledLogColorTab(const float[][3],int);
    void			setLineWidth(float,int);
    float			lineWidth(int) const;
    bool			lineDisp(int) const;
    void			showLog(bool,int);
    bool			logShown(int) const;
    void			clearLog(int);
    void 			setLineDisplayed(bool, int);
    void			setLogValue(int,const SbVec3f&,float,int);
    void   			setLogStyle(bool,int);
    bool 		        getLogStyle() const;
    void			setShift(float,int);
    void   			setLogFill(bool,int);
    void 			setFillLogValue(int,float,int);
    void 			setFillExtrValue(float,float,int);
    void			setRevScale( bool yn, int log_nr ) 
    				{ (log_nr == 1 ? revscale1 
					      : revscale2) = yn; }
    void			setFillRevScale( bool yn, int log_nr ) 
    				{ (log_nr == 1 ? fillrevscale1 
					      : fillrevscale2) = yn; }
    void 			setLogConstantSize(bool);
    bool 			logConstantSize() const;
    void 			setLogConstantSizeFactor(float);
    float 			logConstantSizeFactor() const;
    
    SoMFVec3f			path1;
    SoMFVec3f			path2;
    SoMFFloat			log1;
    SoMFFloat			log2;
    SoMFFloat			filllog1;
    SoMFFloat			filllog2;
    SoSFFloat			maxval1;
    SoSFFloat			maxval2;
    SoSFFloat			fillmaxval1;
    SoSFFloat			fillmaxval2;
    SoSFFloat			minval1;
    SoSFFloat			minval2;
    SoSFFloat			fillminval1;
    SoSFFloat			fillminval2;
    SoSFFloat			shift1;
    SoSFFloat			shift2;
    SoSFFloat			screenWidth1;
    SoSFFloat			screenWidth2;

    SO_KIT_CATALOG_ENTRY_HEADER(topSeparator);
    SO_KIT_CATALOG_ENTRY_HEADER(lineshape1);
    SO_KIT_CATALOG_ENTRY_HEADER(lineshape2);
    SO_KIT_CATALOG_ENTRY_HEADER(line1Switch);
    SO_KIT_CATALOG_ENTRY_HEADER(group1);
    SO_KIT_CATALOG_ENTRY_HEADER(col1);
    SO_KIT_CATALOG_ENTRY_HEADER(lineset1);
    SO_KIT_CATALOG_ENTRY_HEADER(coltri1);
    SO_KIT_CATALOG_ENTRY_HEADER(coltriseis1);
    SO_KIT_CATALOG_ENTRY_HEADER(drawstyle1);
    SO_KIT_CATALOG_ENTRY_HEADER(coords1);
    SO_KIT_CATALOG_ENTRY_HEADER(trishape1);
    SO_KIT_CATALOG_ENTRY_HEADER(coordtri1);
    SO_KIT_CATALOG_ENTRY_HEADER(triset1);
    SO_KIT_CATALOG_ENTRY_HEADER(material1);
    SO_KIT_CATALOG_ENTRY_HEADER(mbinding1);
    SO_KIT_CATALOG_ENTRY_HEADER(hints1);
    SO_KIT_CATALOG_ENTRY_HEADER(line2Switch);
    SO_KIT_CATALOG_ENTRY_HEADER(group2);
    SO_KIT_CATALOG_ENTRY_HEADER(col2);
    SO_KIT_CATALOG_ENTRY_HEADER(lineset2);
    SO_KIT_CATALOG_ENTRY_HEADER(drawstyle2);
    SO_KIT_CATALOG_ENTRY_HEADER(coords2);
    SO_KIT_CATALOG_ENTRY_HEADER(trishape2);
    SO_KIT_CATALOG_ENTRY_HEADER(coordtri2);
    SO_KIT_CATALOG_ENTRY_HEADER(triset2);
    SO_KIT_CATALOG_ENTRY_HEADER(material2);
    SO_KIT_CATALOG_ENTRY_HEADER(mbinding2);
    SO_KIT_CATALOG_ENTRY_HEADER(hints2);

    void			GLRender(SoGLRenderAction*);


protected:

    bool			valchanged;
    bool			resizewhenzooming;
    int				currentres;
    float			worldwidth;
    bool			revscale1, revscale2;
    bool			fillrevscale1, fillrevscale2;
    bool  			seisstyle1, seisstyle2;
    bool  			isfilled1, isfilled2;
    bool  			islinedisp1, islinedisp2;
    int 			lognr;
    float			constantsizefactor;
    
    SbVec2s 			screensize;
    SbTime 			time;
    
    SoFieldSensor*		valuesensor;
    SoTimerSensor*		timesensor;
    static void			valueChangedCB(void*,SoSensor*);
    
    void			buildLog(int,const SbVec3f&,int);
    void			buildSimpleLog(int,const SbVec3f&,int);
    void			buildSeismicLog(int,const SbVec3f&,int);
    void			buildFilledLog(int,const SbVec3f&,int);
    void			fillLogTriangles(const int,SoCoordinate3*,
	    						SbVec3f&,SbVec3f&);
    SbVec3f 			getProjCoords(const SoMFVec3f&,const int, 
					  const SbVec3f&, const SoSFFloat&,
					  const float, int lognr);
    SbVec3f			getNormal(const SbVec3f&,const SbVec3f&,
	    				  const SbVec3f&);
    bool			shouldGLRender(int);
    int				getResolution(SoState*);
    bool			isZooming(SoState*);
};

#endif


