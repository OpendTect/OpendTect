#ifndef SoDepthTabPlaneDragger_h
#define SoDepthTabPlaneDragger_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoDepthTabPlaneDragger.h,v 1.2 2003-09-22 13:16:16 kristofer Exp $
________________________________________________________________________


-*/

#include <Inventor/draggers/SoDragger.h>


class SbLineProjector;
class SbPlaneProjector;

/*!\brief
A TabPlaneDragger where the moving on moves the dragger in z direction
(instead of xy as with SoTabPlaneDragger). In addition, there are limits
to how small the dragger is allowed to be.
*/

class SoDepthTabPlaneDragger : public SoDragger
{
    typedef SoDragger inherited;
    SO_KIT_HEADER(SoDepthTabPlaneDragger);

    SO_KIT_CATALOG_ENTRY_HEADER(tabForegroundLifter);
    SO_KIT_CATALOG_ENTRY_HEADER(planeForegroundLifter);
    SO_KIT_CATALOG_ENTRY_HEADER(cornerScaleCoords);
    SO_KIT_CATALOG_ENTRY_HEADER(cornerScaleTab0);
    SO_KIT_CATALOG_ENTRY_HEADER(cornerScaleTab1);
    SO_KIT_CATALOG_ENTRY_HEADER(cornerScaleTab2);
    SO_KIT_CATALOG_ENTRY_HEADER(cornerScaleTab3);
    SO_KIT_CATALOG_ENTRY_HEADER(edgeScaleCoords);
    SO_KIT_CATALOG_ENTRY_HEADER(edgeScaleTab0);
    SO_KIT_CATALOG_ENTRY_HEADER(edgeScaleTab1);
    SO_KIT_CATALOG_ENTRY_HEADER(edgeScaleTab2);
    SO_KIT_CATALOG_ENTRY_HEADER(edgeScaleTab3);
    SO_KIT_CATALOG_ENTRY_HEADER(planeSwitch);
    SO_KIT_CATALOG_ENTRY_HEADER(scaleTabHints);
    SO_KIT_CATALOG_ENTRY_HEADER(scaleTabMaterial);
    SO_KIT_CATALOG_ENTRY_HEADER(scaleTabMaterialBinding);
    SO_KIT_CATALOG_ENTRY_HEADER(scaleTabNormal);
    SO_KIT_CATALOG_ENTRY_HEADER(scaleTabNormalBinding);
    SO_KIT_CATALOG_ENTRY_HEADER(scaleTabs);
    SO_KIT_CATALOG_ENTRY_HEADER(translator);

public:
    static void		initClass();
    			SoDepthTabPlaneDragger();

    SoSFVec3f		translation;
    SoSFVec3f		scaleFactor;

    SoSFVec3f		minSize;
    SoSFVec3f		maxSize;

    SoSFVec3f		minPos;
    SoSFVec3f		maxPos;

protected:
    			~SoDepthTabPlaneDragger();
    virtual void	GLRender(SoGLRenderAction*);

    virtual SbBool	setUpConnections(SbBool onoff,
	    				 SbBool doitalways = false);
    virtual void	setDefaultOnNonWritingFields();

    void		reallyAdjustScaleTabSize(SoGLRenderAction*);

    void		dragStart(void);
    void		drag(void);
    void		dragFinish(void);

private:

    static void		startCB(void* f, SoDragger * d);
    static void		motionCB(void* f, SoDragger * d);
    static void		finishCB(void* f, SoDragger * d);
    static void		fieldSensorCB(void* f, SoSensor * s);
    static void		valueChangedCB(void* f, SoDragger * d);

    void		createPrivateParts();
    SoNode*		getNodeFieldNode(const char *fieldname);

    SoFieldSensor*	scaleFieldSensor;
    SoFieldSensor*	translFieldSensor;
    SbLineProjector*	lineProj;
    int			whatkind;
    int			constraintState;
    float		prevsizex;
    float		prevsizey;
    SbVec3f		worldRestartPt;
    SbVec3f		scaleCenter;
};

#endif

