/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Kristofer Tingdahl
 * DATE     : Sep 2000
-*/
static const char* rcsID mUsedVar = "$Id$";

#include <SoManLevelOfDetail.h>

#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/caches/SoBoundingBoxCache.h>
#include <Inventor/elements/SoLocalBBoxMatrixElement.h>
#include <Inventor/elements/SoComplexityElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/misc/SoChildList.h>
#include <stdlib.h>


SO_NODE_SOURCE(SoManLevelOfDetail);

SoManLevelOfDetail::SoManLevelOfDetail(void)
{
  this->commonConstructor();
}


SoManLevelOfDetail::SoManLevelOfDetail(int numchildren)
  : inherited(numchildren)
{
  this->commonConstructor();
}

void SoManLevelOfDetail::commonConstructor(void)
{
    SoManLevelOfDetail::initClass();
    SoManLevelOfDetail::classinstances++;

    bboxcache = NULL;

    assert(SoManLevelOfDetail::classTypeId != SoType::badType() &&
	   "you forgot init()!");

    if (!SoManLevelOfDetail::fieldData)
    {
	SoManLevelOfDetail::fieldData =
	new SoFieldData( SoManLevelOfDetail::parentFieldData ?
			 *SoManLevelOfDetail::parentFieldData : NULL );
    }

    this->isBuiltIn = TRUE;

    this->screenArea.setContainer( this );
    this->whichChild.setContainer( this );

    if ( classinstances == 1 )
    {
	fieldData->addField(this, "screenArea", &this->screenArea );
	fieldData->addField(this, "whichChild", &this->whichChild );
    }

    this->screenArea.setNum(0);
    this->whichChild.setValue(SO_MANLEVELOFDETAIL_AUTO);
}

SoManLevelOfDetail::~SoManLevelOfDetail()
{
    if ( bboxcache ) bboxcache->unref();
}

void SoManLevelOfDetail::initClass(void)
{
    static SbBool first = TRUE;
    if ( !first )
	return;

    first = FALSE;
    const char * classname = "SoManLevelOfDetail";
    assert(SoManLevelOfDetail::classTypeId == SoType::badType() &&
	               "don't init() twice!");
    assert(SoGroup::getClassTypeId() != SoType::badType() &&
		       "you forgot init() on parentclass!");

    SoManLevelOfDetail::classTypeId = SoType::createType(
	    				    SoGroup::getClassTypeId(),
					    &classname[2],
					    &SoManLevelOfDetail::createInstance,
					    SoNode::getNextActionMethodIndex());

    SoNode::incNextActionMethodIndex();
    SoManLevelOfDetail::parentFieldData = SoGroup::getFieldDataPtr();
    SoNode::setCompatibilityTypes( SoManLevelOfDetail::getClassTypeId(),
				   SoNode::COIN_2_0|SoNode::VRML1);
}


void SoManLevelOfDetail::doAction(SoAction *action)
{
    switch (action->getCurPathCode())
    {
	case SoAction::IN_PATH:
	    inherited::doAction(action);
	    return;
	case SoAction::OFF_PATH:
	    return; 
	case SoAction::BELOW_PATH:
	case SoAction::NO_PATH:
	    break; 
	default:
	    assert(0 && "unknown path code");
	return;
    }

    SoState * state = action->getState();
    int n = this->getNumChildren();
    if (n == 0) return;

    SbVec2s size;
    SbBox3f bbox;
    int idx = -1;
    float projarea = 0.0f;

    SoComplexityTypeElement::Type complext =
				SoComplexityTypeElement::get(state);
    float complexity = SbClamp(SoComplexityElement::get(state), 0.0f, 1.0f);

    SoType actiontype = action->getTypeId();
    if ( actiontype==SoRayPickAction::getClassTypeId() ||
	 actiontype==SoPickAction::getClassTypeId() )
    {
	idx = 0;
	goto traverse;
    }

    if ( this->whichChild.getValue()>SO_MANLEVELOFDETAIL_AUTO )
    {
	idx =  this->whichChild.getValue();
	if ( idx>n-1 ) idx=n-1;
	goto traverse;
    }

    if (n == 1) { idx = 0; goto traverse; }
    if (complext == SoComplexityTypeElement::BOUNDING_BOX)
    { idx = n - 1; goto traverse; }
    if (complexity == 0.0f) { idx = n - 1; goto traverse; }
    if (complexity == 1.0f) { idx = 0; goto traverse; }
    if (this->screenArea.getNum() == 0) { idx = 0; goto traverse; }

    if (!this->bboxcache || !this->bboxcache->isValid(state))
    {
	static SoGetBoundingBoxAction* bboxAction = 0;
	if ( !bboxAction )
	{
	    bboxAction = new SoGetBoundingBoxAction(SbViewportRegion());
	}

	bboxAction->setViewportRegion(SoViewportRegionElement::get(state));
	bboxAction->setResetPath(action->getCurPath());
	bboxAction->apply((SoPath*) action->getCurPath());
	bbox = bboxAction->getBoundingBox();
    }
    else
    {
	bbox = this->bboxcache->getProjectedBox();
    }

    SoShape::getScreenSize(state, bbox, size);
    projarea = float(size[0]) * float(size[1]) * (complexity + 0.5f);
    n = SbMin(n, this->screenArea.getNum());

    for (int i = 0; i < n; i++)
    {
	if (projarea > this->screenArea[i]) { idx = i; goto traverse; }
    }

    idx = this->getNumChildren() - 1;

traverse:
    state->push();
    this->getChildren()->traverse(action, idx);
    state->pop();
    return;
}

void SoManLevelOfDetail::callback(SoCallbackAction *action)
{
    SoManLevelOfDetail::doAction((SoAction*)action);
}

void SoManLevelOfDetail::GLRender(SoGLRenderAction *action)
{
    SoManLevelOfDetail::doAction((SoAction*)action);
}

void SoManLevelOfDetail::rayPick(SoRayPickAction *action)
{
    SoManLevelOfDetail::doAction((SoAction*)action);
}


void  SoManLevelOfDetail::pick( SoPickAction* action )
{
    SoManLevelOfDetail::doAction((SoAction*)action);
}


void SoManLevelOfDetail::getBoundingBox(SoGetBoundingBoxAction * action)
{
    SoState * state = action->getState();

    SbXfBox3f childrenbbox;
    SbBool childrencenterset;
    SbVec3f childrencenter;

    SbBool iscaching = TRUE;
    switch (action->getCurPathCode())
    {
	case SoAction::OFF_PATH:
	case SoAction::IN_PATH:
	    iscaching = FALSE;
	    break;
	    return;
	case SoAction::BELOW_PATH:
	case SoAction::NO_PATH:
	    if (action->isInCameraSpace()) iscaching = FALSE;
	    break;
	default:
	    iscaching = FALSE;
	    assert(0 && "unknown path code");
	    break;
    }

    SbBool validcache = this->bboxcache && this->bboxcache->isValid(state);
    if (iscaching && validcache)
    {
	SoCacheElement::addCacheDependency(state, this->bboxcache);
	childrenbbox = this->bboxcache->getBox();
	childrencenterset = this->bboxcache->isCenterSet();
	childrencenter = this->bboxcache->getCenter();
	if (this->bboxcache->hasLinesOrPoints())
	{
	    SoBoundingBoxCache::setHasLinesOrPoints(state);
	}
    }
    else
    {
	SbBool storedinvalid = FALSE;
	if (iscaching)
	{
	    storedinvalid = SoCacheElement::setInvalid(FALSE);
	    state->push();
	    if (this->bboxcache) this->bboxcache->unref();
	    this->bboxcache = new SoBoundingBoxCache(state);
	    this->bboxcache->ref();
	    SoCacheElement::set(state, this->bboxcache);
	}
	
	SoLocalBBoxMatrixElement::makeIdentity(state);
	action->getXfBoundingBox().makeEmpty();
	inherited::getBoundingBox(action);

	childrenbbox = action->getXfBoundingBox();
	childrencenterset = action->isCenterSet();
	if (childrencenterset) childrencenter = action->getCenter();

	if (iscaching)
	{
	    this->bboxcache->set(childrenbbox, childrencenterset,
		    		 childrencenter);
	    state->pop();
	    SoCacheElement::setInvalid(storedinvalid);
	}
    }

    if (!childrenbbox.isEmpty()) 
    {
	action->extendBy(childrenbbox);
	if (childrencenterset)
	{
	    action->resetCenter();
	    action->setCenter(childrencenter, TRUE);
	}
    }
}






	
