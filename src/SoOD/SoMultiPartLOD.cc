/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: SoMultiPartLOD.cc,v 1.3 2002-07-09 12:33:01 kristofer Exp $";

#include "SoMultiPartLOD.h"
#include <Inventor/misc/SoState.h>
#include <Inventor/SbLinear.h>
#include <Inventor/actions/SoAction.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoGLCacheContextElement.h>


SO_NODE_SOURCE(SoMultiPartLOD);

SoMultiPartLOD::SoMultiPartLOD(void)
{
    this->commonConstructor();
}


void SoMultiPartLOD::commonConstructor(void)
{
    SoMultiPartLOD::initClass();
    SoMultiPartLOD::classinstances++;

    assert(SoMultiPartLOD::classTypeId != SoType::badType() &&
	   "you forgot init()!");
    if (!SoMultiPartLOD::fieldData)
    {
	SoMultiPartLOD::fieldData =
	    new SoFieldData( SoMultiPartLOD::parentFieldData ?
		    	     *SoMultiPartLOD::parentFieldData : NULL );
    }
    
    this->isBuiltIn = TRUE;

    this->centers.setContainer(this);
    this->whichChild.setContainer(this);
    this->range.setContainer(this);

    if ( classinstances == 1 )
    {
	fieldData->addField(this, "centers", &this->centers);
	fieldData->addField(this, "range", &this->range);
	fieldData->addField(this, "whichChild", &this->whichChild);
    }

    this->centers.setNum(0);
    this->whichChild.setValue(-1);
}


SoMultiPartLOD::~SoMultiPartLOD()
{}


void SoMultiPartLOD::initClass(void)
{
    static SbBool first = TRUE;
    if ( !first )
	return;

    first = FALSE;

    const char * classname = "SoMultiPartLOD";
    assert(SoMultiPartLOD::classTypeId == SoType::badType() &&
	   "don't init() twice!");
    assert(SoGroup::getClassTypeId() != SoType::badType() &&
	   "you forgot init() on parentclass!");
    SoMultiPartLOD::classTypeId = SoType::createType( SoGroup::getClassTypeId(),
					  &classname[2],
					  &SoMultiPartLOD::createInstance,
					  SoNode::getNextActionMethodIndex());
    SoNode::incNextActionMethodIndex();
    SoMultiPartLOD::parentFieldData = SoGroup::getFieldDataPtr();

    /* Should be uncommented when we switch to Coin-2
    SoNode::setCompatibilityTypes( SoMultiPartLOD::getClassTypeId(),
				   SoNode::COIN_2_0|SoNode::VRML1);
   */

}


void SoMultiPartLOD::doAction(SoAction *action)
{
    int numindices;
    const int * indices;
    if (action->getPathCode(numindices, indices) == SoAction::IN_PATH)
    {
	this->children->traverseInPath(action, numindices, indices);
    }
    else
    {
	int idx = this->whichToTraverse(action);
	if (idx >= 0) this->children->traverse(action, idx);
    }
}


void SoMultiPartLOD::callback(SoCallbackAction *action)
{ SoMultiPartLOD::doAction((SoAction*)action); }


void SoMultiPartLOD::GLRender(SoGLRenderAction * action)
{
    switch (action->getCurPathCode())
    {
	case SoAction::NO_PATH:
	case SoAction::BELOW_PATH:
	    SoMultiPartLOD::GLRenderBelowPath(action);
	    break;
	case SoAction::IN_PATH:
	    SoMultiPartLOD::GLRenderInPath(action);
	    break;
	case SoAction::OFF_PATH:
	    SoMultiPartLOD::GLRenderOffPath(action);
	    break;
	default:
	assert(0 && "unknown path code.");
	break;
    }
}


void SoMultiPartLOD::GLRenderBelowPath(SoGLRenderAction * action)
{
    int idx = this->whichToTraverse(action);
    if (idx >= 0)
    {
	if (!action->abortNow())
       	{
	    SoNode * child = (SoNode*) this->children->get(idx);
	    action->pushCurPath(idx, child);
	    child->GLRenderBelowPath(action);
	    action->popCurPath();
	}
    }

    SoGLCacheContextElement::shouldAutoCache(action->getState(),
				    SoGLCacheContextElement::DONT_AUTO_CACHE);
}


void SoMultiPartLOD::GLRenderInPath(SoGLRenderAction * action)
{
    int numindices;
    const int * indices;
    SoAction::PathCode pathcode = action->getPathCode(numindices, indices);

    if (pathcode == SoAction::IN_PATH)
    {
	for (int i = 0; i < numindices; i++)
	{
	    if (action->abortNow()) break;
	    int idx = indices[i];
	    SoNode * node = this->getChild(idx);
	    action->pushCurPath(idx, node);
	    node->GLRenderInPath(action);
	    action->popCurPath(pathcode);
	}
    }
    else
    {
	assert(pathcode == SoAction::BELOW_PATH);
	SoMultiPartLOD::GLRenderBelowPath(action);
    }
}


void SoMultiPartLOD::GLRenderOffPath(SoGLRenderAction * action)
{
    int idx = this->whichToTraverse(action);
    if (idx >= 0)
    {
	SoNode * node = this->getChild(idx);
	if (node->affectsState())
	{
	    action->pushCurPath(idx, node);
	    node->GLRenderOffPath(action);
	    action->popCurPath();
	}
    }
}


void SoMultiPartLOD::rayPick(SoRayPickAction *action)
{
    SoMultiPartLOD::doAction((SoAction*)action);
}

// Documented in superclass.
void SoMultiPartLOD::getBoundingBox(SoGetBoundingBoxAction * action)
{
    // FIXME: SGI OIV seems to do some extra work here, but the manual
    // pages states that it should do a normal SoGroup traversal.
    // we should _not_ use whichToTraverse() to calculate bbox as
    // this would cause cache dependencies on the camera and
    // the model matrix.                       pederb, 2001-02-21
    inherited::getBoundingBox(action);
}


void SoMultiPartLOD::getPrimitiveCount(SoGetPrimitiveCountAction *action)
{
    SoMultiPartLOD::doAction((SoAction*)action);
}


int SoMultiPartLOD::whichToTraverse(SoAction *action)
{
    const int whichchild = whichChild.getValue();
    if ( whichchild!=-1 ) return whichchild;

    SoState *state = action->getState();
    const SbMatrix &mat = SoModelMatrixElement::get(state);
    const SbViewVolume &vv = SoViewVolumeElement::get(state);
    
    const int nrcenters = centers.getNum();
    if ( !nrcenters ) return -1;

    SbVec3f worldcenter;
    float dist;

    for ( int idx=0; idx<nrcenters; idx++ )
    {
	mat.multVecMatrix(this->centers[idx].getValue(), worldcenter);
	const float newdist = (vv.getProjectionPoint() - worldcenter).length();
	if ( !idx || newdist<dist ) dist = newdist;
    }

    int i;
    const int n = this->range.getNum();
    for (i = 0; i < n; i++)
    {
	if (dist < this->range[i]) break;
    }

    if (i >= this->getNumChildren()) i = this->getNumChildren() - 1;
    return i;
}


