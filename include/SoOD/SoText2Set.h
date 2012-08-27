#ifndef SMALLCHANGE_SOTEXT2SET_H
#define SMALLCHANGE_SOTEXT2SET_H

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
#include <Inventor/nodes/SoShape.h>
#include <Inventor/fields/SoMFFloat.h>
#include <Inventor/fields/SoSFInt32.h>
#include <Inventor/fields/SoMFString.h>
#include <Inventor/fields/SoMFEnum.h>
#include <Inventor/fields/SoMFVec3f.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbBox3f.h>

#include "soodbasic.h"


mSoODClass SoText2Set : public SoShape
{
  typedef SoShape inherited;

  SO_NODE_HEADER(SoText2Set);

public:

  enum Justification {
    LEFT = 1,
    RIGHT,
    CENTER
  };
  
  // Fields
  SoMFEnum justification;
  SoMFString string;
  SoMFVec3f position;
  SoMFFloat rotation;
  SoSFBool renderOutline;
  SoSFInt32 maxStringsToRender;

  static void initClass(void);
  virtual void GLRender(SoGLRenderAction * action);
  virtual void rayPick(SoRayPickAction * action);
  virtual void getPrimitiveCount(SoGetPrimitiveCountAction * action);

  SoText2Set(void);

protected:
  virtual ~SoText2Set();
  
  virtual void notify(SoNotList * list);
  virtual void generatePrimitives(SoAction * action);
  virtual void computeBBox(SoAction * action, SbBox3f & box, SbVec3f & center);

private:
  friend class SoText2SetP;
  class SoText2SetP * pimpl;
};

#endif // !SMALLCHANGE_SOTEXT2SET_H

