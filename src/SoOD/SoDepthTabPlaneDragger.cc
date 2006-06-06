/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: SoDepthTabPlaneDragger.cc,v 1.10 2006-06-06 14:27:36 cvskris Exp $";


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
#include "Inventor/nodes/SoPolygonOffset.h"
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
"   PolygonOffset { styles FILLED factor -1.0 units -1.0}\n"
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

    SO_KIT_ADD_CATALOG_ENTRY(planeSwitch, SoSwitch, true,
			    geomSeparator, "", false);
    SO_KIT_ADD_CATALOG_ENTRY(translator, SoSeparator, true,
			    planeSwitch, greenTabsSep, true);
    SO_KIT_ADD_CATALOG_ENTRY(greenTabsSep, SoSeparator, true,
			    planeSwitch, "", true);
    SO_KIT_ADD_CATALOG_ENTRY(greenTabsMaterial, SoMaterial, true,
			    greenTabsSep, greenTabsHints, true);
    SO_KIT_ADD_CATALOG_ENTRY(greenTabsHints, SoShapeHints, true,
			    greenTabsSep, greenTabsMaterialBinding, true);
    SO_KIT_ADD_CATALOG_ENTRY(greenTabsMaterialBinding, SoMaterialBinding,
			    true, greenTabsSep, greenTabsNormalBinding, false);
    SO_KIT_ADD_CATALOG_ENTRY(greenTabsNormalBinding, SoNormalBinding,
			    true, greenTabsSep, greenTabsNormal, false);
    SO_KIT_ADD_CATALOG_ENTRY(greenTabsNormal, SoNormal, true,
			    greenTabsSep, greenTabsCoords, false);
    SO_KIT_ADD_CATALOG_ENTRY(greenTabsCoords, SoCoordinate3, true,
			    greenTabsSep, greenTabsOffset, false);
    SO_KIT_ADD_CATALOG_ENTRY(greenTabsOffset, SoPolygonOffset, true,
			    greenTabsSep, greenTabs, false);
    SO_KIT_ADD_CATALOG_ENTRY(greenTabs, SoIndexedFaceSet, true,
			    greenTabsSep, "", false);

    if ( SO_KIT_IS_FIRST_INSTANCE() )
    {
	SoInteractionKit::readDefaultParts("tabPlaneDragger.iv",
			    DEPTHTABPLANEDRAGGER_draggergeometry,
			    strlen(DEPTHTABPLANEDRAGGER_draggergeometry));
    }


    SO_KIT_ADD_FIELD(translation, (0.0f, 0.0f, 0.0f));
    SO_KIT_ADD_FIELD(scaleFactor, (1.0f, 1.0f, 1.0f));

    SO_KIT_ADD_FIELD( minSize, ( 0.1, 0.1, 0));
    SO_KIT_ADD_FIELD( maxSize, ( 1e30, 1e30, 1e30));
    SO_KIT_ADD_FIELD( minPos, (-1e30, -1e30, -1e30) );
    SO_KIT_ADD_FIELD( maxPos, (1e30, 1e30, 1e30));

    SO_KIT_INIT_INSTANCE();

    setPartAsDefault("translator", "tabPlaneTranslator");
    setPartAsDefault("greenTabsMaterial", "tabPlaneScaleTabMaterial");
    setPartAsDefault("greenTabsHints", "tabPlaneScaleTabHints");

    SoSwitch* sw = SO_GET_ANY_PART(this, "planeSwitch", SoSwitch);
    SoInteractionKit::setSwitchValue(sw, SO_SWITCH_ALL);

    createPrivateParts();
    prevsizex_ = prevsizey_ = 0.0f;
    reallyAdjustScaleTabSize(NULL);
    constraintState_ = CONSTRAINT_OFF;
    whatkind_ = WHATKIND_NONE;

    addStartCallback(SoDepthTabPlaneDragger::startCB);
    addMotionCallback(SoDepthTabPlaneDragger::motionCB);
    addFinishCallback(SoDepthTabPlaneDragger::finishCB);
    addValueChangedCallback(SoDepthTabPlaneDragger::valueChangedCB);

    lineProj_ = new SbLineProjector;

    translFieldSensor_ =
		new SoFieldSensor(SoDepthTabPlaneDragger::fieldSensorCB, this);
    translFieldSensor_->setPriority(0);
    scaleFieldSensor_ =
		new SoFieldSensor(SoDepthTabPlaneDragger::fieldSensorCB, this);
    scaleFieldSensor_->setPriority(0);

    setUpConnections(true, true);
}


SoDepthTabPlaneDragger::~SoDepthTabPlaneDragger()
{
    delete translFieldSensor_;
    delete scaleFieldSensor_;
    delete lineProj_;
}


SbBool SoDepthTabPlaneDragger::setUpConnections(SbBool onoff, SbBool doitalways)
{
    if ( !doitalways && connectionsSetUp==onoff )
	return onoff;

    if ( onoff )
    {
	inherited::setUpConnections(onoff, doitalways);

	SoDepthTabPlaneDragger::fieldSensorCB(this, NULL);

	if ( translFieldSensor_->getAttachedField()!=&translation )
	    translFieldSensor_->attach(&translation);

	if ( scaleFieldSensor_->getAttachedField()!=&scaleFactor )
	    scaleFieldSensor_->attach(&scaleFactor);
    }
    else
    {
	if ( translFieldSensor_->getAttachedField()!=NULL )
	    translFieldSensor_->detach();
	if ( scaleFieldSensor_->getAttachedField()!=NULL )
	    scaleFieldSensor_->detach();

	inherited::setUpConnections(onoff, doitalways);
    }

    return !( connectionsSetUp = onoff);
}


void SoDepthTabPlaneDragger::setDefaultOnNonWritingFields(void)
{
    greenTabsCoords.setDefault(true);

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
}


void SoDepthTabPlaneDragger::valueChangedCB(void*, SoDragger* d)
{
    SoDepthTabPlaneDragger* thisp = (SoDepthTabPlaneDragger*)d;
    SbMatrix matrix = thisp->getMotionMatrix();
    SbVec3f trans, scale;
    SbRotation rot, scaleOrient;
    matrix.getTransform(trans, rot, scale, scaleOrient);

    thisp->translFieldSensor_->detach();
    if ( thisp->translation.getValue()!=trans )
	thisp->translation = trans;
    thisp->translFieldSensor_->attach(&thisp->translation);

    thisp->scaleFieldSensor_->detach();
    if ( thisp->scaleFactor.getValue()!=scale )
	thisp->scaleFactor = scale;
    thisp->scaleFieldSensor_->attach(&thisp->scaleFactor);
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
	sizex = fabs(sizex/scale[0]);
	sizey = fabs(sizey/scale[1]);
	if ( isnan(sizex) ) sizex = 0.08f;
	if ( isnan(sizey) ) sizey = 0.08f;
    }

    if ( sizex==prevsizex_ && prevsizey_==sizey )
	return;

    prevsizex_ = sizex;
    prevsizey_ = sizey;

    float halfx = sizex * 0.5f;
    float halfy = sizey * 0.5f;

    SoCoordinate3* coordnode =
	SO_GET_ANY_PART(this, "greenTabsCoords", SoCoordinate3);

    coordnode->point.setNum(32);
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

	coords[16].setValue(1.0f, 1.0f, 0);
	coords[17].setValue(1.0f-sizex, 1.0f, 0);
	coords[18].setValue(1.0f-sizex, 1.0f-sizey, 0);
	coords[19].setValue(1.0f, 1.0f-sizey, 0);

	coords[20].setValue(1.0f, -1.0f, 0);
	coords[21].setValue(1.0f, -1.0f+sizey, 0);
	coords[22].setValue(1.0f-sizex, -1.0f+sizey, 0);
	coords[23].setValue(1.0f-sizex, -1.0f, 0);

	coords[24].setValue(-1.0f, -1.0f, 0);
	coords[25].setValue(-1.0f+sizex, -1.0f, 0);
	coords[26].setValue(-1.0f+sizex, -1.0f+sizey, 0);
	coords[27].setValue(-1.0f, -1.0f+sizey, 0);

	coords[28].setValue(-1.0f, 1.0f, 0);
	coords[29].setValue(-1.0f, 1.0f-sizey, 0);
	coords[30].setValue(-1.0f+sizex, 1.0f-sizey, 0);
	coords[31].setValue(-1.0f+sizex, 1.0f, 0);
    }

    coordnode->point.finishEditing();
}


void SoDepthTabPlaneDragger::dragStart(void)
{
    const SoPath* pickpath = getPickPath();
    const SoEvent* event = getEvent();

    SbBool found = FALSE;
    SbVec3f startpt = getLocalStartingPoint();

    constraintState_ = CONSTRAINT_OFF;
    char xside = -2;
    const float absx = fabs(startpt[0]);
    if ( absx>1-prevsizex_ )
	xside = startpt[0]>0 ? 1 : -1;
    else if ( absx<prevsizex_ )
	xside = 0;

    char yside = -2;
    const float absy = fabs(startpt[1]);
    if ( absy>1-prevsizey_ )
	yside = startpt[1]>0 ? 1 : -1;
    else if ( absy<prevsizey_ )
	yside = 0;

    if ( xside!=-2 && yside!=-2 )
    {
	if ( !xside )
	    constraintState_ = CONSTRAINT_Y;
	else if ( !yside )
	    constraintState_ = CONSTRAINT_X;

	whatkind_ = WHATKIND_SCALE;
	scaleCenter_.setValue(-xside,-yside,0);
	lineProj_->setLine(SbLine(scaleCenter_, startpt));
    }
    else
    {
	whatkind_ = WHATKIND_TRANSLATE;
	lineProj_->setLine(SbLine(startpt, startpt + SbVec3f(0, 0, 1)));
    }
}



void SoDepthTabPlaneDragger::drag(void)
{
    if ( whatkind_==WHATKIND_SCALE )
    {
	SbVec3f startpt = getLocalStartingPoint();
	lineProj_->setViewVolume(getViewVolume());
	lineProj_->setWorkingSpace(getLocalToWorldMatrix());
	SbVec3f projpt = lineProj_->project(getNormalizedLocaterPosition());

	float orglen = (startpt-scaleCenter_).length();
	float currlen = (projpt-scaleCenter_).length();
	float scale = 0.0f;

	if ( orglen>0.0f )
	    scale = currlen / orglen;

	if ( scale>0.0f &&
		(startpt-scaleCenter_).dot(projpt-scaleCenter_)<=0.0f)
	    scale = 0.0f;

	SbVec3f scalevec(scale, scale, 1.0f);
	if ( constraintState_==CONSTRAINT_X )
	    scalevec[1] = 1.0f;
	else if ( constraintState_==CONSTRAINT_Y )
	    scalevec[0] = 1.0f;

	SbMatrix motmat =
	    appendScale(getStartMotionMatrix(), scalevec, scaleCenter_);

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

	lineProj_->setViewVolume(getViewVolume());
	lineProj_->setWorkingSpace(getLocalToWorldMatrix());
	const SbVec3f newhitpt =
	    		lineProj_->project(getNormalizedLocaterPosition());

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
{ whatkind_ = WHATKIND_NONE; }


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
    SoPolygonOffset* po = 
	SO_GET_ANY_PART(this, "greenTabsOffset", SoPolygonOffset);
    po->styles.setValue(SoPolygonOffset::FILLED );
    po->factor.setValue( -2 );
    po->units.setValue( -2 );
    greenTabsOffset.setDefault(TRUE);

    SoMaterialBinding* mb =
	SO_GET_ANY_PART(this, "greenTabsMaterialBinding", SoMaterialBinding);
    mb->value = SoMaterialBinding::OVERALL;
    greenTabsMaterialBinding.setDefault(TRUE);

    SoNormalBinding* nb =
	SO_GET_ANY_PART(this, "greenTabsNormalBinding", SoNormalBinding);
    nb->value = SoNormalBinding::OVERALL;
    greenTabsNormalBinding.setDefault(TRUE);

    SoNormal* normal = SO_GET_ANY_PART(this, "greenTabsNormal", SoNormal);
    normal->vector.setValue(SbVec3f(0.0f, 0.0f, 1.0f));
    greenTabsNormal.setDefault(TRUE);

    SoIndexedFaceSet* tabs = SO_GET_ANY_PART(this, "greenTabs",
	    				     SoIndexedFaceSet);
    int cii = 0;
    int ci = 0;
    tabs->coordIndex.setNum(40);
    int32_t* ptr = tabs->coordIndex.startEditing();
    for ( int i=0; i<8; i++ )
    {
	for ( int j=0; j<4; j++ )
	    ptr[cii++] = ci++;

	ptr[cii++] = -1;
    }

    tabs->coordIndex.finishEditing();
    tabs->normalIndex.setValue(0);
    tabs->materialIndex.setValue(0);
    greenTabs.setDefault(TRUE);

    SoSeparator *sep = SO_GET_ANY_PART(this, "translator", SoSeparator);
    sep->renderCaching = SoSeparator::OFF;
}


SoNode* SoDepthTabPlaneDragger::getNodeFieldNode(const char *fieldname)
{
    SoField* field = getField(fieldname);
    return ((SoSFNode*)field)->getValue();
}

