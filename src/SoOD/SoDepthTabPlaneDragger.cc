/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: SoDepthTabPlaneDragger.cc,v 1.3 2003-11-07 12:22:02 bert Exp $";


#include "SoDepthTabPlaneDragger.h"

#include "Inventor/SbRotation.h"

#include "Inventor/actions/SoGLRenderAction.h"

#include "Inventor/elements/SoCacheElement.h"
#include "Inventor/elements/SoModelMatrixElement.h"
#include "Inventor/elements/SoViewportRegionElement.h"
#include "Inventor/elements/SoViewVolumeElement.h"

#include "Inventor/events/SoKeyboardEvent.h"

#include "Inventor/nodes/SoCoordinate3.h"
#include "SoForegroundTranslation.h"
#include "Inventor/nodes/SoIndexedFaceSet.h"
#include "Inventor/nodes/SoMaterial.h"
#include "Inventor/nodes/SoMaterialBinding.h"
#include "Inventor/nodes/SoNormal.h"
#include "Inventor/nodes/SoNormalBinding.h"
#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoShapeHints.h"
#include "Inventor/nodes/SoSwitch.h"

#include <Inventor/projectors/SbLineProjector.h>
#include <Inventor/projectors/SbPlaneProjector.h>



#define WHATKIND_NONE      0
#define WHATKIND_SCALE     1
#define WHATKIND_TRANSLATE 2

#define CONSTRAINT_OFF  0
#define CONSTRAINT_WAIT 1
#define CONSTRAINT_X    2
#define CONSTRAINT_Y    3
#define CONSTRAINT_Z    4

#define Z_OFFSET 0.01f 
#define TABSIZE 10.0f 


static float edgetab_lookup[] =
{
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, -1.0f,
    -1.0f, 0.0f
};

static float cornertab_lookup[] =
{
    1.0f, 1.0f,
    1.0f, -1.0f,
    -1.0f, -1.0f,
    -1.0f, 1.0f
};


static const char DEPTHTABPLANEDRAGGER_draggergeometry[] =
"#Inventor V2.1 ascii\n"
"\n"
"DEF tabPlaneScaleTabMaterial Material { diffuseColor 0 0.5 0  emissiveColor 0 0.5 0 }\n"
"DEF tabPlaneScaleTabHints ShapeHints {\n"
"   vertexOrdering COUNTERCLOCKWISE\n"
"   faceType UNKNOWN_FACE_TYPE\n"
"   shapeType UNKNOWN_SHAPE_TYPE\n"
"}\n"
"DEF tabPlaneTranslator Separator {\n"
"   ShapeHints { vertexOrdering COUNTERCLOCKWISE\n}"
"   Coordinate3 { point [ -1 -1 0, 1 -1 0, 1 1 0, -1 1 0 ] }\n"
"   DrawStyle { style LINES }\n"
"   IndexedFaceSet { coordIndex [ 0, 1, 2, 3, -1, 3, 2, 1, 0 ] }\n"
"}\n";



SO_KIT_SOURCE(SoDepthTabPlaneDragger);

void SoDepthTabPlaneDragger::initClass()
{
    SO_KIT_INIT_CLASS(SoDepthTabPlaneDragger, SoDragger, "Dragger");
}


SoDepthTabPlaneDragger::SoDepthTabPlaneDragger()
{
    SO_KIT_CONSTRUCTOR( SoDepthTabPlaneDragger );

    SO_KIT_ADD_CATALOG_ENTRY(planeForegroundLifter,SoForegroundTranslation,true,
	    		    geomSeparator, planeSwitch, false );
    SO_KIT_ADD_CATALOG_ENTRY(planeSwitch, SoSwitch, true,
	    			geomSeparator, "", false);
    SO_KIT_ADD_CATALOG_ENTRY(translator, SoSeparator, true,
	    			planeSwitch, scaleTabs, true);
    SO_KIT_ADD_CATALOG_ENTRY(scaleTabs, SoSeparator, true,
	    			planeSwitch, "", false);
    SO_KIT_ADD_CATALOG_ENTRY(scaleTabMaterial, SoMaterial, true,
	    			scaleTabs, scaleTabHints, true);
    SO_KIT_ADD_CATALOG_ENTRY(scaleTabHints, SoShapeHints, true,
	    			scaleTabs, scaleTabMaterialBinding, true);
    SO_KIT_ADD_CATALOG_ENTRY(scaleTabMaterialBinding, SoMaterialBinding,
	    			true, scaleTabs, scaleTabNormalBinding, false);
    SO_KIT_ADD_CATALOG_ENTRY(scaleTabNormalBinding, SoNormalBinding,
	    			true, scaleTabs, scaleTabNormal, false);
    SO_KIT_ADD_CATALOG_ENTRY(scaleTabNormal, SoNormal, true,
	    			scaleTabs, tabForegroundLifter, false);
    SO_KIT_ADD_CATALOG_ENTRY(tabForegroundLifter, SoForegroundTranslation, true,
	    		        scaleTabs, edgeScaleCoords, false );
    SO_KIT_ADD_CATALOG_ENTRY(edgeScaleCoords, SoCoordinate3, true,
	    			scaleTabs, edgeScaleTab0, false);
    SO_KIT_ADD_CATALOG_ENTRY(edgeScaleTab0, SoIndexedFaceSet, true,
	    			scaleTabs, edgeScaleTab1, false);
    SO_KIT_ADD_CATALOG_ENTRY(edgeScaleTab1, SoIndexedFaceSet, true,
	    			scaleTabs, edgeScaleTab2, false);
    SO_KIT_ADD_CATALOG_ENTRY(edgeScaleTab2, SoIndexedFaceSet, true,
	    			scaleTabs, edgeScaleTab3, false);
    SO_KIT_ADD_CATALOG_ENTRY(edgeScaleTab3, SoIndexedFaceSet, true,
	    			scaleTabs, cornerScaleCoords, false);
    SO_KIT_ADD_CATALOG_ENTRY(cornerScaleCoords, SoCoordinate3, true,
	    			scaleTabs, cornerScaleTab0, false);
    SO_KIT_ADD_CATALOG_ENTRY(cornerScaleTab0, SoIndexedFaceSet, true,
	    			scaleTabs, cornerScaleTab1, false);
    SO_KIT_ADD_CATALOG_ENTRY(cornerScaleTab1, SoIndexedFaceSet, true,
	    			scaleTabs, cornerScaleTab2, false);
    SO_KIT_ADD_CATALOG_ENTRY(cornerScaleTab2, SoIndexedFaceSet, true,
	    			scaleTabs, cornerScaleTab3, false);
    SO_KIT_ADD_CATALOG_ENTRY(cornerScaleTab3, SoIndexedFaceSet, true,
	    			scaleTabs, "", false);


    if ( SO_KIT_IS_FIRST_INSTANCE() )
    {
	SoInteractionKit::readDefaultParts("tabPlaneDragger.iv",
			    DEPTHTABPLANEDRAGGER_draggergeometry,
			    sizeof(DEPTHTABPLANEDRAGGER_draggergeometry));
    }


    SO_KIT_ADD_FIELD(translation, (0.0f, 0.0f, 0.0f));
    SO_KIT_ADD_FIELD(scaleFactor, (1.0f, 1.0f, 1.0f));

    SO_KIT_ADD_FIELD( minSize, ( 0.1, 0.1, 0));
    SO_KIT_ADD_FIELD( maxSize, ( 1e30, 1e30, 1e30));
    SO_KIT_ADD_FIELD( minPos, (-1e30, -1e30, -1e30) );
    SO_KIT_ADD_FIELD( maxPos, (1e30, 1e30, 1e30));

    SO_KIT_INIT_INSTANCE();

    setPartAsDefault("translator", "tabPlaneTranslator");
    setPartAsDefault("scaleTabMaterial", "tabPlaneScaleTabMaterial");
    setPartAsDefault("scaleTabHints", "tabPlaneScaleTabHints");

    SoSwitch* sw = SO_GET_ANY_PART(this, "planeSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);

    createPrivateParts();
    prevsizex = prevsizey = 0.0f;
    reallyAdjustScaleTabSize(NULL);
    constraintState = CONSTRAINT_OFF;
    whatkind = WHATKIND_NONE;

    addStartCallback(SoDepthTabPlaneDragger::startCB);
    addMotionCallback(SoDepthTabPlaneDragger::motionCB);
    addFinishCallback(SoDepthTabPlaneDragger::finishCB);
    addValueChangedCallback(SoDepthTabPlaneDragger::valueChangedCB);

    lineProj = new SbLineProjector;

    translFieldSensor =
		new SoFieldSensor(SoDepthTabPlaneDragger::fieldSensorCB, this);
    translFieldSensor->setPriority(0);
    scaleFieldSensor =
		new SoFieldSensor(SoDepthTabPlaneDragger::fieldSensorCB, this);
    scaleFieldSensor->setPriority(0);

    setUpConnections(true, true);
}


SoDepthTabPlaneDragger::~SoDepthTabPlaneDragger()
{
    delete translFieldSensor;
    delete scaleFieldSensor;
    delete lineProj;
}


SbBool SoDepthTabPlaneDragger::setUpConnections(SbBool onoff, SbBool doitalways)
{
    if ( !doitalways && connectionsSetUp==onoff )
	return onoff;

    if ( onoff )
    {
	inherited::setUpConnections(onoff, doitalways);

	SoDepthTabPlaneDragger::fieldSensorCB(this, NULL);

	if ( translFieldSensor->getAttachedField()!=&translation )
	    translFieldSensor->attach(&translation);

	if ( scaleFieldSensor->getAttachedField()!=&scaleFactor )
	    scaleFieldSensor->attach(&scaleFactor);
    }
    else
    {
	if ( translFieldSensor->getAttachedField()!=NULL )
	    translFieldSensor->detach();
	if ( scaleFieldSensor->getAttachedField()!=NULL )
	    scaleFieldSensor->detach();

	inherited::setUpConnections(onoff, doitalways);
    }

    return !( connectionsSetUp = onoff);
}


void SoDepthTabPlaneDragger::setDefaultOnNonWritingFields(void)
{
    edgeScaleCoords.setDefault(true);
    cornerScaleCoords.setDefault(true);

    inherited::setDefaultOnNonWritingFields();
}


void SoDepthTabPlaneDragger::fieldSensorCB(void* d, SoSensor*)
{
    SoDepthTabPlaneDragger*thisp = (SoDepthTabPlaneDragger*)d;
    SbMatrix matrix = thisp->getMotionMatrix();
    thisp->workFieldsIntoTransform(matrix);
    thisp->setMotionMatrix(matrix);

    const SbVec3f scale = thisp->scaleFactor.getValue();
    float avgscale = (scale[0]+scale[1])/2;

    SoForegroundTranslation* tablifter =
	SO_GET_ANY_PART(thisp, "tabForegroundLifter",SoForegroundTranslation);
    tablifter->lift.setValue(avgscale*0.02);

    SoForegroundTranslation* planelifter =
	SO_GET_ANY_PART(thisp, "planeForegroundLifter",SoForegroundTranslation);
    planelifter->lift.setValue(avgscale*0.01);
}


void SoDepthTabPlaneDragger::valueChangedCB(void*, SoDragger* d)
{
    SoDepthTabPlaneDragger* thisp = (SoDepthTabPlaneDragger*)d;
    SbMatrix matrix = thisp->getMotionMatrix();
    SbVec3f trans, scale;
    SbRotation rot, scaleOrient;
    matrix.getTransform(trans, rot, scale, scaleOrient);

    thisp->translFieldSensor->detach();
    if ( thisp->translation.getValue()!=trans )
	thisp->translation = trans;
    thisp->translFieldSensor->attach(&thisp->translation);

    thisp->scaleFieldSensor->detach();
    if ( thisp->scaleFactor.getValue()!=scale )
	thisp->scaleFactor = scale;
    thisp->scaleFieldSensor->attach(&thisp->scaleFactor);
}


void SoDepthTabPlaneDragger::GLRender(SoGLRenderAction* action)
{
    SbBool oldnotify = enableNotify(false);
    SoCacheElement::invalidate(action->getState());
 
    reallyAdjustScaleTabSize(action);

    enableNotify(oldnotify);

    inherited::GLRender(action);
}


void SoDepthTabPlaneDragger::reallyAdjustScaleTabSize(SoGLRenderAction* action)
{
    float sizex = 0.08f;
    float sizey = 0.08f;
    if ( action!=NULL )
    {
	SoState *state = action->getState();
	SbMatrix toworld = SoModelMatrixElement::get(state);
	toworld.multLeft(getMotionMatrix());
	const SbViewVolume &vv = SoViewVolumeElement::get(state);
	const SbViewportRegion &vp = SoViewportRegionElement::get(state);
	SbVec3f center(0.0f, 0.0f, 0.0f);
	toworld.multVecMatrix(center, center);
	sizex = sizey = vv.getWorldToScreenScale(center,
		TABSIZE/float(vp.getViewportSizePixels()[0]));

	SbVec3f scale;
	{
	    SbRotation r, so;
	    SbVec3f t;
	    toworld.getTransform(t, r, scale, so);
	}
	sizex /= scale[0];
	sizey /= scale[1];
    }

    if ( sizex==prevsizex && prevsizey==sizey )
	return;

    prevsizex = sizex;
    prevsizey = sizey;

    float halfx = sizex * 0.5f;
    float halfy = sizey * 0.5f;

    SoCoordinate3* coordnode =
	SO_GET_ANY_PART(this, "edgeScaleCoords", SoCoordinate3);

    coordnode->point.setNum(16);
    SbVec3f* coords = coordnode->point.startEditing();
    {
	coords[0].setValue(halfx, 1.0f, 0);
	coords[1].setValue(-halfx, 1.0f, 0);
	coords[2].setValue(-halfx, 1.0f-sizey, 0);
	coords[3].setValue(halfx, 1.0f-sizey, 0);

	coords[4].setValue(1.0f, -halfy, 0);
	coords[5].setValue(1.0f, halfy, 0);
	coords[6].setValue(1.0f-sizex, halfy, 0);
	coords[7].setValue(1.0f-sizex, -halfy, 0);

	coords[8].setValue(-halfx, -1.0f, 0);
	coords[9].setValue(halfx, -1.0f, 0);
	coords[10].setValue(halfx, -1.0f+sizey, 0);
	coords[11].setValue(-halfx, -1.0f+sizey, 0);

	coords[12].setValue(-1.0f, halfy, 0);
	coords[13].setValue(-1.0f, -halfy, 0);
	coords[14].setValue(-1.0f+sizex, -halfy, 0);
	coords[15].setValue(-1.0f+sizex, halfy, 0);
    }

    coordnode->point.finishEditing();

    coordnode = SO_GET_ANY_PART(this, "cornerScaleCoords", SoCoordinate3);
    coordnode->point.setNum(16);
    coords = coordnode->point.startEditing();
    {
	coords[0].setValue(1.0f, 1.0f, 0);
	coords[1].setValue(1.0f-sizex, 1.0f, 0);
	coords[2].setValue(1.0f-sizex, 1.0f-sizey, 0);
	coords[3].setValue(1.0f, 1.0f-sizey, 0);

	coords[4].setValue(1.0f, -1.0f, 0);
	coords[5].setValue(1.0f, -1.0f+sizey, 0);
	coords[6].setValue(1.0f-sizex, -1.0f+sizey, 0);
	coords[7].setValue(1.0f-sizex, -1.0f, 0);

	coords[8].setValue(-1.0f, -1.0f, 0);
	coords[9].setValue(-1.0f+sizex, -1.0f, 0);
	coords[10].setValue(-1.0f+sizex, -1.0f+sizey, 0);
	coords[11].setValue(-1.0f, -1.0f+sizey, 0);

	coords[12].setValue(-1.0f, 1.0f, 0);
	coords[13].setValue(-1.0f, 1.0f-sizey, 0);
	coords[14].setValue(-1.0f+sizex, 1.0f-sizey, 0);
	coords[15].setValue(-1.0f+sizex, 1.0f, 0);
    }

    coordnode->point.finishEditing();
}


void SoDepthTabPlaneDragger::dragStart(void)
{
    int i;
    const SoPath* pickpath = getPickPath();
    const SoEvent* event = getEvent();

    SbBool found = FALSE;
    SbVec3f startpt = getLocalStartingPoint();

    constraintState = CONSTRAINT_OFF;

    if ( !found )
    {
	int i=0;
	for ( ; i<4; i++ )
	{
	    SbString str;
	    str.sprintf("edgeScaleTab%d", i);

	    if ( pickpath->findNode(getNodeFieldNode(str.getString()))>=0 ||
		 getSurrogatePartPickedName()==str.getString())
		break;
	}

	if ( i<4 )
	{
	    found = TRUE;
	    constraintState = i & 1 ? CONSTRAINT_X : CONSTRAINT_Y;
	    whatkind = WHATKIND_SCALE;
	    scaleCenter.setValue(-edgetab_lookup[i*2],
		    		-edgetab_lookup[i*2+1], 0.0f);
	}
    }

    if ( !found )
    {
	int i=0; 
	for ( ; i<4; i++ )
	{
	    SbString str;
	    str.sprintf("cornerScaleTab%d", i);
	    if ( pickpath->findNode(getNodeFieldNode(str.getString()))>=0 ||
	         getSurrogatePartPickedName() == str.getString())
		break;
	}
	if ( i<4 )
	{
	    found = TRUE;
	    whatkind = WHATKIND_SCALE;
	    scaleCenter.setValue(-cornertab_lookup[i*2],
		    -cornertab_lookup[i*2+1], 0.0f);
	}
    }

    if ( !found )
    {
	found = TRUE;
	whatkind = WHATKIND_TRANSLATE;
    }

    if ( whatkind == WHATKIND_SCALE )
    {
	lineProj->setLine(SbLine(scaleCenter, startpt));
    }
    else
    { // depth
	lineProj->setLine(SbLine(startpt, startpt + SbVec3f(0, 0, 1)));
	constraintState = CONSTRAINT_OFF;
    }
}



void SoDepthTabPlaneDragger::drag(void)
{
    if ( whatkind==WHATKIND_SCALE )
    {
	SbVec3f startpt = getLocalStartingPoint();
	lineProj->setViewVolume(getViewVolume());
	lineProj->setWorkingSpace(getLocalToWorldMatrix());
	SbVec3f projpt = lineProj->project(getNormalizedLocaterPosition());

	SbVec3f center = scaleCenter;

	float orglen = (startpt-center).length();
	float currlen = (projpt-center).length();
	float scale = 0.0f;

	if ( orglen>0.0f )
	    scale = currlen / orglen;

	if ( scale>0.0f && (startpt-center).dot(projpt-center)<=0.0f)
	    scale = 0.0f;

	SbVec3f scalevec(scale, scale, 1.0f);
	if ( constraintState==CONSTRAINT_X )
	    scalevec[1] = 1.0f;
	else if ( constraintState==CONSTRAINT_Y )
	    scalevec[0] = 1.0f;

	SbMatrix motmat = appendScale(getStartMotionMatrix(), scalevec, center);

	SbVec3f trans, scalechange;
	SbRotation rot, scaleorient;
	motmat.getTransform(trans, rot, scalechange, scaleorient );

	const SbVec3f newscale = scalechange.getValue();
	const SbVec3f newstart = trans-newscale;
	const SbVec3f newstop = trans+newscale;
	const SbVec3f minsize = minSize.getValue();
	const SbVec3f maxsize = maxSize.getValue();
	const SbVec3f minpos = minPos.getValue();
	const SbVec3f maxpos = maxPos.getValue();

	if (    newscale[0]>=minsize[0] && newscale[0]<=maxsize[0] &&
		newscale[1]>=minsize[1] && newscale[1]<=maxsize[1] &&
		newstart[0]>=minpos[0] && newstart[0]<=maxpos[0] &&
		newstart[1]>=minpos[1] && newstart[1]<=maxpos[1] &&
		newstop[0]>=minpos[0] && newstop[0]<=maxpos[0] &&
		newstop[1]>=minpos[1] && newstop[1]<=maxpos[1] )
	{
	    setMotionMatrix(motmat);
	}
    }
    else
    { // translate
	const SbVec3f startpt = getLocalStartingPoint();

	lineProj->setViewVolume(getViewVolume());
	lineProj->setWorkingSpace(getLocalToWorldMatrix());
	const SbVec3f newhitpt =
	    		lineProj->project(getNormalizedLocaterPosition());

	SbVec3f motion = newhitpt-startpt;
	SbMatrix motmat = appendTranslation(getStartMotionMatrix(), motion);

	SbVec3f trans, scale;
	SbRotation rot, scaleorient;
	motmat.getTransform(trans, rot, scale, scaleorient );

	SbVec3f mov = trans.getValue();
	const SbVec3f minpos = minPos.getValue();
	const SbVec3f maxpos = maxPos.getValue();
	if ( mov[2]<=maxpos[2] && mov[2]>=minpos[2] )
	{
	    setMotionMatrix(motmat);
	}
    }
}


void SoDepthTabPlaneDragger::dragFinish(void)
{ whatkind = WHATKIND_NONE; }


void SoDepthTabPlaneDragger::startCB(void* , SoDragger* d)
{
    SoDepthTabPlaneDragger* thisp = (SoDepthTabPlaneDragger*)d;
    thisp->dragStart();
}


void SoDepthTabPlaneDragger::motionCB(void*, SoDragger* d)
{
    SoDepthTabPlaneDragger* thisp = (SoDepthTabPlaneDragger*)d;
    thisp->drag();
}


void SoDepthTabPlaneDragger::finishCB(void*, SoDragger* d)
{
    SoDepthTabPlaneDragger* thisp = (SoDepthTabPlaneDragger*)d;
    thisp->dragFinish();
}


void SoDepthTabPlaneDragger::createPrivateParts()
{
    SoForegroundTranslation* lifter =
	SO_GET_ANY_PART(this, "tabForegroundLifter", SoForegroundTranslation);
    lifter->lift.setValue(Z_OFFSET);

    SoMaterialBinding* mb =
	SO_GET_ANY_PART(this, "scaleTabMaterialBinding", SoMaterialBinding);
    mb->value = SoMaterialBinding::OVERALL;
    scaleTabMaterialBinding.setDefault(TRUE);

    SoNormalBinding* nb =
	SO_GET_ANY_PART(this, "scaleTabNormalBinding", SoNormalBinding);
    nb->value = SoNormalBinding::OVERALL;
    scaleTabNormalBinding.setDefault(TRUE);

    SoNormal* normal = SO_GET_ANY_PART(this, "scaleTabNormal", SoNormal);
    normal->vector.setValue(SbVec3f(0.0f, 0.0f, 1.0f));
    scaleTabNormal.setDefault(TRUE);

    int idx = 0;
    for ( int i=0; i<8; i++ )
    {
	SbString str;
	if ( i==0 || i==4)
	    idx = 0;
	if ( i<4 )
	    str.sprintf("edgeScaleTab%d", i);
	else
	    str.sprintf("cornerScaleTab%d", i-4);

	SoIndexedFaceSet* fs =
	    (SoIndexedFaceSet*) getAnyPart(SbName(str.getString()), TRUE);

	fs->coordIndex.setNum(5);

	int32_t* ptr = fs->coordIndex.startEditing();
	for ( int j=0; j<4; j++ )
	    ptr[j] = idx++;

	ptr[4] = -1;

	fs->coordIndex.finishEditing();
	fs->normalIndex.setValue(0);
	fs->materialIndex.setValue(0);

	SoField* f = getField(SbName(str.getString()));
	f->setDefault(TRUE);
    }

    SoSeparator *sep = SO_GET_ANY_PART(this, "scaleTabs", SoSeparator);
    sep->renderCaching = SoSeparator::OFF;
}


SoNode* SoDepthTabPlaneDragger::getNodeFieldNode(const char *fieldname)
{
    SoField* field = getField(fieldname);
    return ((SoSFNode*)field)->getValue();
}

