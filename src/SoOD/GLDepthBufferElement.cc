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
  \class GLDepthBufferElement Inventor/elements/GLDepthBufferElement.h
  \brief The SoGLDrawStyleElement controls the OpenGL depth buffer.
*/

#include "GLDepthBufferElement.h"

#if HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef __COIN__
#include <Inventor/system/gl.h>
#else // SGI/TGS Inventor
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif // HAVE_WINDOWS_H
#include <GL/gl.h>
#endif // SGI/TGS Inventor

#include <assert.h>

SO_ELEMENT_SOURCE(GLDepthBufferElement);

/*!
  This static method initializes static data for the
  GLDepthBufferElement class.
*/

void
GLDepthBufferElement::initClass(void)
{
  static int first = 1;
  if (first) {
    first = 0;
    SO_ELEMENT_INIT_CLASS(GLDepthBufferElement, inherited);
  }
}

/*!
  The destructor.
*/

GLDepthBufferElement::~GLDepthBufferElement(void)
{
}

/*!
  Internal Coin method.
*/
void
GLDepthBufferElement::init(SoState * state)
{
  inherited::init(state);
  this->enable = TRUE;
  this->func = LEQUAL;
  this->updategl();
}


void
GLDepthBufferElement::push(SoState * state)
{
  GLDepthBufferElement * prev = (GLDepthBufferElement*)this->getNextInStack();
  this->enable = prev->enable;
  this->func = prev->func;
  prev->capture(state);
}
/*!
  Internal Coin method.
*/
void
GLDepthBufferElement::pop(SoState * state,
                          const SoElement * prevTopElement)
{
  GLDepthBufferElement * prev = (GLDepthBufferElement*)prevTopElement;
  if (this->enable != prev->enable || this->func != prev->func) {
    this->updategl();
  }
}

/*!
  Set this element's values.
*/
void 
GLDepthBufferElement::set(SoState * state, const Func func, const SbBool enable)
{
  GLDepthBufferElement * elem = (GLDepthBufferElement*)
    SoElement::getElement(state, classStackIndex);

  if (func != elem->func || enable != elem->enable) {
    elem->enable = enable;
    elem->func = func;
    elem->updategl();
  }
}

/*!
  Internal Coin method.
*/
SbBool 
GLDepthBufferElement::matches(const SoElement * element) const
{
  const GLDepthBufferElement * elem = (const GLDepthBufferElement*) element;
  return (elem->func == this->func) && (elem->enable == this->enable); 
}

/*!
  Internal Coin method.
*/
SoElement * 
GLDepthBufferElement::copyMatchInfo(void) const
{
  GLDepthBufferElement * elem = (GLDepthBufferElement*)
    this->getTypeId().createInstance();
  elem->func = this->func;
  elem->enable = this->enable;
  return elem;
}

void 
GLDepthBufferElement::updategl(void) const
{
  if (this->enable) {
    glEnable(GL_DEPTH_TEST);
    switch (this->func) {
    case NEVER: glDepthFunc(GL_NEVER); break;
    case ALWAYS: glDepthFunc(GL_ALWAYS); break;
    case LESS: glDepthFunc(GL_LESS); break;
    case LEQUAL: glDepthFunc(GL_LEQUAL); break;
    case EQUAL: glDepthFunc(GL_EQUAL); break;
    case GEQUAL: glDepthFunc(GL_GEQUAL); break;
    case GREATER: glDepthFunc(GL_GREATER); break;
    case NOTEQUAL: glDepthFunc(GL_NOTEQUAL); break; 
    }
  }
  else glDisable(GL_DEPTH_TEST);
}
