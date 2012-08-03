#ifndef SMALLCHANGE_VIEWPORTREGION_H
#define SMALLCHANGE_VIEWPORTREGION_H

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
#include <Inventor/nodes/SoNode.h>
#include <Inventor/fields/SoSFVec2f.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFBool.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <SoWinLeaveScope.h>
#endif // win

#include "soodbasic.h"

class ViewportRegionP;

mClass(SoOD) ViewportRegion : public SoNode {
  typedef SoNode inherited;

  SO_NODE_HEADER(ViewportRegion);

public:
  static void initClass(void);

  SoSFVec2f origin;
  SoSFVec2f size;
  SoSFBool flipX;
  SoSFBool flipY;
  SoSFBool clampSize;

  SoSFVec2f pixelOrigin;
  SoSFVec2f pixelSize;
  SoSFBool clearDepthBuffer;
  SoSFBool clearColorBuffer;
  SoSFColor clearColor;

  ViewportRegion(void);

  virtual void doAction(SoAction * action);
  virtual void GLRender(SoGLRenderAction * action);
  virtual void getMatrix(SoGetMatrixAction * action);
  virtual void handleEvent(SoHandleEventAction * action);
  virtual void pick(SoPickAction * action);
  virtual void callback(SoCallbackAction * action);

protected:
  virtual ~ViewportRegion();
  virtual void notify(SoNotList * list);

private:
  ViewportRegionP * pimpl;
};

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <SoWinEnterScope.h>
#endif // win

#endif // !SMALLCHANGE_VIEWPORTREGION_H

