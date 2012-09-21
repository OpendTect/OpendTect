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
  \class UTMCamera UTMCamera SmallChange/nodes/UTMCamera.h
  \brief The UTMCamera class defines a camera node with perspective rendering and an UTM position.
  \ingroup nodes

  This node adds a field \a utmposition to the perspective camera
  node. \a utmposition is a vector which contains 3 values for
  \e easting, \e northing and \e elevation.

  The position vector moves the camera relative to the UTM
  position. Use this camera instead of a normal SoPerspectiveCamera if
  you plan to operate on large floating point coordinates that might
  cause floating point precision to become too low. This is typically
  useful if you want to place objects using, for instance, UTM
  coordinates.

  To perhaps better understand how it works, think of it this way:
  when scene graph traversal arrives at an UTMCamera, all
  transformation matrices will be set to identity, i.e. no
  transformations, so the camera position (for OpenGL) will be set at
  world origo (0,0,0). When a UTMPosition node is then later
  encountered during traversal, a transformation will be added which
  translates geometry to its correct position relative to the
  UTMCamera setting.

  As the camera will always be positioned in (0,0,0) for OpenGL with
  this scheme, we get the advantage of maximum precision for floating
  point numbers.

  \sa UTMPosition, UTMCoordinate 
*/
static const char* rcsID mUnusedVar = "$Id$";

// *************************************************************************

#include "UTMCamera.h"
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoAudioRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoState.h>
#include <UTMElement.h>

// *************************************************************************

/*!
  \var SoSFVec3d UTMCamera::utmposition
  
  First value of vector is easting (+X) position of the camera, second
  is northing (+Y) position, and third ist he elevation (+Z) position
  of the camera.

  Default value is [0, 0, 0].
*/

/*!
  \var SoSFBool UTMCamera::moveTransform

  This field is to enable a hack to move transforms in front of the
  camera to after an UTMPosition node.

  Default value is \c FALSE.
*/

// *************************************************************************

SO_NODE_SOURCE(UTMCamera);

// *************************************************************************

UTMCamera::UTMCamera(void)
{
  SO_NODE_CONSTRUCTOR(UTMCamera);

  SO_NODE_ADD_FIELD(utmposition, (0.0, 0.0, 0.0)); 

  // these are present for backwards compatibility
  SO_NODE_ADD_FIELD(easting, ("0.0"));
  SO_NODE_ADD_FIELD(northing, ("0.0"));
  SO_NODE_ADD_FIELD(elevation, ("0.0"));

  SO_NODE_ADD_FIELD(moveTransform, (FALSE)); 
}

UTMCamera::~UTMCamera()
{
}

void
UTMCamera::initClass(void)
{
  SO_NODE_INIT_CLASS(UTMCamera, SoPerspectiveCamera, "PerspectiveCamera");
}

void 
UTMCamera::callback(SoCallbackAction * action)
{
  this->setReferencePosition(action->getState());
  SoCamera::callback(action);
}

void 
UTMCamera::GLRender(SoGLRenderAction * action)
{
  SoCacheElement::invalidate(action->getState());
  this->setReferencePosition(action->getState());
  SoCamera::GLRender(action);
}

void 
UTMCamera::audioRender(SoAudioRenderAction * action)
{
  this->setReferencePosition(action->getState());
  SoCamera::audioRender(action);
}

void 
UTMCamera::getBoundingBox(SoGetBoundingBoxAction * action)
{
  this->setReferencePosition(action->getState());
  SoCamera::getBoundingBox(action);
}

void 
UTMCamera::handleEvent(SoHandleEventAction * action)
{
  SoCamera::handleEvent(action);
}

void 
UTMCamera::rayPick(SoRayPickAction * action)
{
  this->setReferencePosition(action->getState());
  SoCamera::rayPick(action);
}

void 
UTMCamera::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  this->setReferencePosition(action->getState());
  SoCamera::getPrimitiveCount(action);
}

/*!
  This is a helper function to return the three components of the
  UTMCamera::utmposition vector field.

  FIXME: function is redundant and should be made obsolete.
*/
void 
UTMCamera::getPosition( double& east, double& north, double& elev )
{
  SbVec3d utm = this->utmposition.getValue();
  east = utm[0];
  north = utm[1];
  elev = utm[2];
}

/*!
  Internal method that will update UTMElement to set the
  reference position.
*/
void 
UTMCamera::setReferencePosition(SoState * state)
{
  SbVec3d utm = this->utmposition.getValue();
  UTMElement::setReferencePosition(state,
                                   utm[0], utm[1], utm[2]);
  (void) UTMElement::setPosition(state, utm[0], utm[1], utm[2]);

  SoAction * action = state->getAction();

  if (this->moveTransform.getValue() && state->isElementEnabled(SoModelMatrixElement::getClassStackIndex())) {
    SbMatrix m = SoModelMatrixElement::get(state);
    if (action->isOfType(SoGetBoundingBoxAction::getClassTypeId())) {
      SoModelMatrixElement::mult(state, this, m.inverse());
    }
    else {
      SoModelMatrixElement::makeIdentity(state, this);
    }
    UTMElement::setGlobalTransform(state, m);
  }
  else if (state->isElementEnabled(SoModelMatrixElement::getClassStackIndex())) {
    if (action->isOfType(SoGetBoundingBoxAction::getClassTypeId())) {
      SbMatrix m = SoModelMatrixElement::get(state);
      SoModelMatrixElement::mult(state, this, m.inverse());
    }
    else {
      SoModelMatrixElement::makeIdentity(state, this);
    }
  }
}

void 
UTMCamera::getMatrix(SoGetMatrixAction * action)
{
  this->setReferencePosition(action->getState());
  if (this->moveTransform.getValue()) {
    SbMatrix m = action->getMatrix();
    action->getMatrix() = SbMatrix::identity();
    action->getInverse() = SbMatrix::identity();
    UTMElement::setGlobalTransform(action->getState(), m);
  }
  else {
    action->getMatrix() = SbMatrix::identity();
    action->getInverse() = SbMatrix::identity();
  }
}

/*!
  Overridden to recalculate cached values when something change.
*/
void 
UTMCamera::notify(SoNotList * nl)
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
  else if (f == &this->position) {
    SbVec3f v = this->position.getValue();
    if (v != SbVec3f(0.0f, 0.0f, 0.0f)) {
#if 0 // disabled, as this should be ok, at least for the View'EM app. mortene.
      SoDebugError::postWarning("UTMCamera::notify",
                                "Don't use SoPerspectiveCamera::position field, "
                                "but UTMCamera::utmposition (%g %g %g)\n",
                                v[0], v[1], v[2]);
#endif // disabled
    }
  }

  if (update) {
    SbBool old = this->enableNotify(FALSE);
    this->utmposition = utm;
    (void) this->enableNotify(old);
  }

  inherited::notify(nl);
}
