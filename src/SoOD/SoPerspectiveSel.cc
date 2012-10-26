/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Kristofer Tingdahl
 * DATE     : Sep 2000
-*/
static const char* rcsID mUsedVar = "$Id$";

#include <SoPerspectiveSel.h>

#include <Inventor/misc/SoChildList.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/elements/SoViewVolumeElement.h>

#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>

SO_NODE_SOURCE(SoPerspectiveSel);


SoPerspectiveSel::SoPerspectiveSel(void)
{
    this->commonConstructor();
}


SoPerspectiveSel::SoPerspectiveSel(int numchildren)
    : inherited(numchildren)
{
    this->commonConstructor();
}


void SoPerspectiveSel::commonConstructor(void)
{
    SO_NODE_CONSTRUCTOR(SoPerspectiveSel);

    SO_NODE_ADD_FIELD(perspectives, (SbVec3f(0, 0, 0)));
    SO_NODE_ADD_FIELD(center, (SbVec3f(0, 0, 0)));
    this->perspectives.setNum(0);
}


SoPerspectiveSel::~SoPerspectiveSel()
{ }


void SoPerspectiveSel::initClass(void)
{
    SO_NODE_INIT_CLASS(SoPerspectiveSel, SoGroup, "Group" );
}


void SoPerspectiveSel::doAction(SoAction *action)
{
    int numindices;
    const int * indices;
    if (action->getPathCode(numindices, indices) == SoAction::IN_PATH)
    {
	this->children->traverseInPath(action, numindices, indices);
    }
    else
    {
	int idx = this->whichToTraverse(action);;
	if (idx >= 0) this->children->traverse(action, idx);
    }
}


void SoPerspectiveSel::callback(SoCallbackAction *action)
{
    SoPerspectiveSel::doAction((SoAction*)action);
}

void SoPerspectiveSel::GLRender(SoGLRenderAction * action)
{
    switch (action->getCurPathCode())
    {
	case SoAction::NO_PATH:
	case SoAction::BELOW_PATH:
	    SoPerspectiveSel::GLRenderBelowPath(action);
	    break;
	case SoAction::IN_PATH:
	    SoPerspectiveSel::GLRenderInPath(action);
	    break;
	case SoAction::OFF_PATH:
	    SoPerspectiveSel::GLRenderOffPath(action);
	    break;
	default:
	    assert(0 && "unknown path code.");
	    break;
    }
}


void SoPerspectiveSel::GLRenderBelowPath(SoGLRenderAction * action)
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


void SoPerspectiveSel::GLRenderInPath(SoGLRenderAction * action)
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
	SoPerspectiveSel::GLRenderBelowPath(action);
    }
}


void SoPerspectiveSel::GLRenderOffPath(SoGLRenderAction * action)
{
    int idx = this->whichToTraverse(action);;
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


void SoPerspectiveSel::rayPick(SoRayPickAction *action)
{
    SoPerspectiveSel::doAction((SoAction*)action);
}


void SoPerspectiveSel::getBoundingBox(SoGetBoundingBoxAction * action)
{
    inherited::getBoundingBox(action);
}


void SoPerspectiveSel::getPrimitiveCount(SoGetPrimitiveCountAction *action)
{
    SoPerspectiveSel::doAction((SoAction*)action);
}


int SoPerspectiveSel::whichToTraverse(SoAction *action)
{
  SoState *state = action->getState();
  const SbMatrix &mat = SoModelMatrixElement::get(state);
  const SbViewVolume &vv = SoViewVolumeElement::get(state);

  SbVec3f worldcenter;
  mat.multVecMatrix(this->center.getValue(), worldcenter);

  SbVec3f vec = (vv.getProjectionPoint() - worldcenter);
  vec.normalize();

  int n = this->perspectives.getNum();
  float maxangle=-1;
  int mini=-1;

  for ( int i=0; i<n; i++)
  {
      SbVec3f pvec = this->perspectives[i];
      SbVec3f worldpvec;
      mat.multVecMatrix(pvec, worldpvec);
      worldpvec -= worldcenter;
      worldpvec.normalize();

      float cosangle = fabs(worldpvec.dot(vec));
      if ( !i || cosangle>maxangle )
      {
	  maxangle = cosangle;
	  mini = i;
      }
  }

  if (mini >= this->getNumChildren()) mini = this->getNumChildren() - 1;
  return mini;
}
