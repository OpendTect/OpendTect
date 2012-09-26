/**************************************************************************\
 *
 *  This file is part of the SmallChange extension library for Coin.
 *  Copyright (C) 1998-2003 by Systems in Motion.  All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  ("GPL") version 2 as published by the Free Software Foundation.
 *  See the file LICENSE.GPL at the root directory of this source
 *  distribution for additional information about the GNU GPL.
 *
 *  For using SmallChange with software that can not be combined with the
 *  GNU GPL, and for taking advantage of the additional benefits of our
 *  support services, please contact Systems in Motion about acquiring
 *  a Coin Professional Edition License.
 *
 *  See <URL:http://www.coin3d.org> for  more information.
 *
 *  Systems in Motion, Teknobyen, Abels Gate 5, 7030 Trondheim, NORWAY.
 *  <URL:http://www.sim.no>.
 *
\**************************************************************************/

/*!
  \class UTMPosition UTMPosition.h SmallChange/nodes/UTMPosition.h
  \brief The UTMPosition class is a node type for specifying geometry translations.
  \ingroup nodes

  It should be used in conjunction with the UTMCamera node. The
  UTMPosition node will not be useful unless your camera is an
  UTMCamera.

  This node makes it possible for you to stop thinking about floating
  point precision when building a, for instance, real-world model
  using UTM coordinates. As long as your objects are placed in the
  scene using an UTMPosition node, they will, internally, always be
  placed relative to the position of the UTMCamera.  This way it
  doesn't matter if you're using large coordinates.

  If you use this node, do not have transformation nodes over any
  UTMPosition nodes in your scene graph. This will make it impossible
  to calculate the new relative position. You are free to have
  transformation nodes under UTMPositon nodes (as long as they are
  inside an SoSeparator node), of course.

  BUGS: This node will currently not work as expected when used inside
  a SoTransformSeparator. For this to work, the current relative
  translation would need to be stored and restored in the
  SoTransformSeparator traversal code. And, because of SGI's silly
  decision not to support action methods for GLRenderAction after
  Inventor 2.1, I cannot replace SoTransformSeparator::GLRender with
  an improved one...

*/
static const char* rcsID mUsedVar = "$Id$";

// *************************************************************************

#include "UTMPosition.h"
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoAudioRenderAction.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/misc/SoState.h>
#include <UTMElement.h>
#include <stdlib.h>

// *************************************************************************

/*!
  \var SoSFVec3d UTMPosition::utmposition

  Decides the translation of the geometry following this node.

  The positioning will \e replace preceding values, and not accumulate
  with them as for the standard transform nodes.
*/

// *************************************************************************

SO_NODE_SOURCE(UTMPosition);

/*!
  Constructor.
*/
UTMPosition::UTMPosition(void)
{
  SO_NODE_CONSTRUCTOR(UTMPosition);

  SO_NODE_ADD_FIELD(utmposition, (0.0, 0.0, 0.0));
  SO_NODE_ADD_FIELD(easting,   ("0.0"));
  SO_NODE_ADD_FIELD(northing,  ("0.0"));
  SO_NODE_ADD_FIELD(elevation, ("0.0"));
}

/*!
  Destructor.
*/
UTMPosition::~UTMPosition()
{
}

/*!
  Required Coin method.
*/
void
UTMPosition::initClass(void)
{
  SO_NODE_INIT_CLASS(UTMPosition, SoTransformation, "Transformation");

  SO_ENABLE(SoGetBoundingBoxAction, UTMElement);
  SO_ENABLE(SoGLRenderAction, UTMElement);
  SO_ENABLE(SoGetMatrixAction, UTMElement);
  SO_ENABLE(SoPickAction, UTMElement);
  SO_ENABLE(SoCallbackAction, UTMElement);
  SO_ENABLE(SoGetPrimitiveCountAction, UTMElement);
  SO_ENABLE(SoAudioRenderAction, UTMElement);
}

/*!
  Coin method.
*/
void
UTMPosition::doAction(SoAction * action)
{
  SoState * state = action->getState();

  SbVec3d utm = this->utmposition.getValue();
  SbMatrix oldm = SoModelMatrixElement::get(state);
  // eliminate translation
  oldm[3][0] = 0.0f;
  oldm[3][1] = 0.0f;
  oldm[3][2] = 0.0f;
  SbVec3f trans = UTMElement::setPosition(action->getState(),
                                          utm[0], utm[1], utm[2]);
  
  
  if (SoModelMatrixElement::get(state) != SbMatrix::identity()) {
    if (action->isOfType(SoGetBoundingBoxAction::getClassTypeId())) {
      SoModelMatrixElement::mult(state, 
                                 this, 
                                 SoModelMatrixElement::get(state).inverse());
    }
    else {
      SoModelMatrixElement::makeIdentity(action->getState(), this);
    }
  }
  SoModelMatrixElement::translateBy(action->getState(), this, trans);
  
  const SbMatrix & gtransform = UTMElement::getGlobalTransform(action->getState());
  if (gtransform != SbMatrix::identity()) {
    SoModelMatrixElement::mult(action->getState(), this, gtransform);
  }
  if (oldm != SbMatrix::identity()) {
    SoModelMatrixElement::mult(action->getState(), this, oldm); 
  }
}

/*!
  Coin method.
*/
void
UTMPosition::GLRender(SoGLRenderAction * action)
{
  SoCacheElement::invalidate(action->getState());
  UTMPosition::doAction((SoAction *)action);
}

/*!
  Coin method.
*/
void
UTMPosition::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoCacheElement::invalidate(action->getState());
  UTMPosition::doAction(action);
}

/*!
  Coin method.
*/
void
UTMPosition::callback(SoCallbackAction * action)
{
  UTMPosition::doAction((SoAction *)action);
}

/*!
  Coin method.
*/
void
UTMPosition::getMatrix(SoGetMatrixAction * action)
{
  SbMatrix oldm = action->getMatrix();
  
  // eliminate translation
  oldm[3][0] = 0.0f;
  oldm[3][1] = 0.0f;
  oldm[3][2] = 0.0f;

  SbVec3d utm = this->utmposition.getValue();
  SbVec3f trans = UTMElement::setPosition(action->getState(),
                                          utm[0], utm[1], utm[2]);

  SbMatrix mat;
  mat.setTranslate(trans);
  action->getMatrix() = mat;
  mat.setTranslate(-trans);
  action->getInverse() = mat;

  SbMatrix m = UTMElement::getGlobalTransform(action->getState());
  if (m != SbMatrix::identity()) {
    action->getMatrix().multLeft(m);
    action->getInverse().multRight(m.inverse());
  }
  if (oldm != SbMatrix::identity()) {
    action->getMatrix().multLeft(oldm);
    action->getInverse().multRight(oldm.inverse());    
  }
}

/*!
  Coin method.
*/
void
UTMPosition::pick(SoPickAction * action)
{
  UTMPosition::doAction((SoAction *)action);
}

/*!
  Coin method.
*/
void
UTMPosition::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  UTMPosition::doAction((SoAction *)action);
}

/*!
  Overridden to invalidate cache when some field change.
*/
void 
UTMPosition::notify(SoNotList * nl)
{
  SoField * f = nl->getLastField();
  SbVec3d utm = this->utmposition.getValue();
  SbBool update = FALSE;
  if (f == &this->easting) {
    utm[0] = atof(this->easting.getValue().getString());
    update = TRUE;
  }
  else if (f == &this->northing) {
    utm[1] = atof(this->northing.getValue().getString());
    update = TRUE;
  }
  else if (f == &this->elevation) {
    utm[2] = atof(this->elevation.getValue().getString());
    update = TRUE;
  }
  if (update) {
    SbBool old = this->enableNotify(FALSE);
    this->utmposition = utm;
    (void) this->enableNotify(old);
  }
  inherited::notify(nl);
}
