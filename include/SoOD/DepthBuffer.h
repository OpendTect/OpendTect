#ifndef SMALLCHANGE_SODEPTHBUFFER_H
#define SMALLCHANGE_SODEPTHBUFFER_H

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

#include <GLDepthBufferElement.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/nodes/SoSubNode.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <SoWinLeaveScope.h>
#endif // win


class DepthBuffer : public SoNode {
  typedef SoNode inherited;

  SO_NODE_HEADER(DepthBuffer);

public:
  static void initClass(void);
  DepthBuffer(void);

  enum Func {
    NEVER = GLDepthBufferElement::NEVER,
    ALWAYS = GLDepthBufferElement::ALWAYS,
    LESS = GLDepthBufferElement::LESS,
    LEQUAL = GLDepthBufferElement::LEQUAL,
    EQUAL = GLDepthBufferElement::EQUAL,
    GEQUAL = GLDepthBufferElement::GEQUAL,
    GREATER = GLDepthBufferElement::GREATER,
    NOTEQUAL = GLDepthBufferElement::NOTEQUAL
  };

  SoSFEnum func;
  SoSFBool enable;
  SoSFBool clearNow;

  virtual void GLRender(SoGLRenderAction * action);

protected:
  virtual ~DepthBuffer();
};

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <SoWinEnterScope.h>
#endif // win

#endif // !SMALLCHANGE_SODEPTHBUFFER_H
