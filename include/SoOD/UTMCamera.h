#ifndef SMALLCHANGE_UTMCAMERA_H
#define SMALLCHANGE_UTMCAMERA_H

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

#include "soodmod.h"
#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/fields/SoSFString.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFVec3d.h>

#include "soodbasic.h"

class SoState;


mClass(SoOD) UTMCamera : public SoPerspectiveCamera {
  typedef SoPerspectiveCamera inherited;

  SO_NODE_HEADER(UTMCamera);

public:
  static void initClass(void);
  UTMCamera(void);

  SoSFVec3d utmposition;
  SoSFBool moveTransform;

  virtual void callback(SoCallbackAction * action);
  virtual void GLRender(SoGLRenderAction * action);
  virtual void audioRender(class SoAudioRenderAction * action);
  virtual void getBoundingBox(SoGetBoundingBoxAction * action);
  virtual void handleEvent(SoHandleEventAction * action);
  virtual void rayPick(SoRayPickAction * action);
  virtual void getPrimitiveCount(SoGetPrimitiveCountAction * action);
  virtual void getMatrix(SoGetMatrixAction * action);
  
  void getPosition(double & easting, double & northing, double & elevation);
    
protected:
  virtual ~UTMCamera();
  virtual void notify(SoNotList * nl);

private:
  // for backwards compatibility
  SoSFString easting;
  SoSFString northing;
  SoSFString elevation;

  void setReferencePosition(SoState * state);
};

#endif // !SMALLCHANGE_UTMCAMERA_H

