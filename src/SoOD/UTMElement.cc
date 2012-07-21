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
  \class UTMElement UTMElement.h SmallChange/elements/UTMElement.h
  \brief The UTMElement class is yet to be documented.

  FIXME: write doc.
*/
static const char* rcsID mUnusedVar = "$Id: UTMElement.cc,v 1.5 2012-07-21 22:46:08 cvskris Exp $";

#include "UTMElement.h"

SO_ELEMENT_SOURCE(UTMElement);


// doc from parent
void
UTMElement::initClass()
{
  SO_ELEMENT_INIT_CLASS(UTMElement, inherited);
}

// doc from parent
void
UTMElement::init(SoState *state)
{
  inherited::init(state);
  this->easting = 0.0;
  this->northing = 0.0;
  this->elevation = 0.0;
  this->currtrans.setValue(0.0f, 0.0f, 0.0f);
  this->gtransform = SbMatrix::identity();
}

void 
UTMElement::push(SoState * state)
{
  inherited::push(state);
  UTMElement * prev = (UTMElement*) this->getNextInStack();
  this->easting = prev->easting;
  this->northing = prev->northing;
  this->elevation = prev->elevation;
  this->currtrans = prev->currtrans;
  this->gtransform = prev->gtransform;
}

/*!
  The destructor.
*/
UTMElement::~UTMElement()
{
}

void 
UTMElement::setGlobalTransform(SoState * state,
                               const SbMatrix & m)
{
  UTMElement * elem = (UTMElement*)
    SoElement::getElement(state, classStackIndex);
  elem->gtransform = m;
}

const SbMatrix & 
UTMElement::getGlobalTransform(SoState * state)
{
  const UTMElement * elem = (const UTMElement*)
    SoElement::getConstElement(state, classStackIndex);
  return elem->gtransform;
}


/*!
  Set current reference position.
*/
void 
UTMElement::setReferencePosition(SoState * const state,
                                 double easting, double northing,
                                 double elevation)
{
  UTMElement * elem = (UTMElement*)
    SoElement::getElement(state, classStackIndex);

  elem->easting = easting;
  elem->northing = northing;
  elem->elevation = elevation;
  elem->currtrans = SbVec3f(0.0f, 0.0f, 0.0f);
}
  
void 
UTMElement::getReferencePosition(SoState * const state,
                                 double & easting, double & northing,
                                 double & elevation)
{
  const UTMElement * elem = (const UTMElement*)
    SoElement::getConstElement(state, classStackIndex);
  
  easting = elem->easting;
  northing = elem->northing;
  elevation = elem->elevation;
}

SbVec3f 
UTMElement::setPosition(SoState * state,
                        double easting,
                        double northing,
                        double elevation)
{
  UTMElement * elem = (UTMElement*)
    SoElement::getElement(state, classStackIndex);
  
  SbVec3f newtrans = SbVec3f(float(easting - elem->easting), 
                             float(northing - elem->northing),
                             float(elevation - elem->elevation));
  
  elem->currtrans = newtrans;
  return newtrans;
}

const SbVec3f & 
UTMElement::getCurrentTranslation(SoState * state)
{
  const UTMElement * elem = (const UTMElement*)
    SoElement::getConstElement(state, classStackIndex);
  return elem->currtrans;
}

SbBool 
UTMElement::matches(const SoElement * element) const
{
#if 1
  return TRUE;
#else
  UTMElement * elem = (UTMElement*) element;
  SbBool ret = 
    this->easting == elem->easting &&
    this->northing == elem->northing &&
    this->elevation == elem->elevation;
  if (ret == FALSE) {
    //    fprintf(stderr,"blown\n");
  }
  return ret;
#endif
}

SoElement * 
UTMElement::copyMatchInfo(void) const
{
  UTMElement * element =
    (UTMElement *)(this->getTypeId().createInstance());
  element->easting = this->easting;
  element->northing = this->northing;
  element->elevation = this->elevation;
  return (SoElement *)element;
}
