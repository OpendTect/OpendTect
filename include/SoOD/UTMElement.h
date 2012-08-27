#ifndef SMALLCHANGE_UTMELEMENT_H
#define SMALLCHANGE_UTMELEMENT_H

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
#include <Inventor/elements/SoElement.h>
#include <Inventor/elements/SoSubElement.h>
#include <Inventor/SbLinear.h>

#include "soodbasic.h"


mSoODClass UTMElement : public SoElement {
  typedef SoElement inherited;

  SO_ELEMENT_HEADER(UTMElement);

public:
  static void initClass(void);

  virtual void init(SoState * state);
  virtual void push(SoState * state);

  static void setGlobalTransform(SoState * state,
                                 const SbMatrix & m);
  static const SbMatrix & getGlobalTransform(SoState * state);

  static void setReferencePosition(SoState * state,
                                   double easting, double northing,
                                   double elevation);
  
  static void getReferencePosition(SoState * state,
                                   double & easting, double & northing,
                                   double & elevation);

  static SbVec3f setPosition(SoState * state,
                             double easting,
                             double northing,
                             double elevation);
  
  static const SbVec3f & getCurrentTranslation(SoState * state);

  virtual SbBool matches(const SoElement * element) const;
  virtual SoElement * copyMatchInfo(void) const;

protected:
  virtual ~UTMElement();

  double easting;
  double northing;
  double elevation;

  SbVec3f currtrans;
  SbMatrix gtransform;
};

#endif // !SMALLCHANGE_UTMELEMENT_H

