#ifndef SMALLCHANGE_LEGENDKIT_H
#define SMALLCHANGE_LEGENDKIT_H

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
#include <Inventor/nodekits/SoSubKit.h>
#include <Inventor/nodekits/SoBaseKit.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFVec2f.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFString.h>
#include <Inventor/fields/SoMFString.h>
#include <Inventor/SbLinear.h> 

#include "soodbasic.h"


#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <SoWinLeaveScope.h>
#endif // win


class SbViewport;
class SoState;
class SbColor;
class SbVec2s;


mClass(SoOD) LegendKit : public SoBaseKit {
  typedef SoBaseKit inherited;

  SO_KIT_HEADER(LegendKit);

  SO_KIT_CATALOG_ENTRY_HEADER(topSeparator);
  SO_KIT_CATALOG_ENTRY_HEADER(viewport);
  SO_KIT_CATALOG_ENTRY_HEADER(resetTransform);
  SO_KIT_CATALOG_ENTRY_HEADER(position);
  SO_KIT_CATALOG_ENTRY_HEADER(depthBuffer);
  SO_KIT_CATALOG_ENTRY_HEADER(lightModel);
  SO_KIT_CATALOG_ENTRY_HEADER(camera);
  SO_KIT_CATALOG_ENTRY_HEADER(texture);
  SO_KIT_CATALOG_ENTRY_HEADER(shapeHints);
  SO_KIT_CATALOG_ENTRY_HEADER(pickStyle);
  SO_KIT_CATALOG_ENTRY_HEADER(backgroundMaterial);
  SO_KIT_CATALOG_ENTRY_HEADER(backgroundShape);
  SO_KIT_CATALOG_ENTRY_HEADER(imageSeparator);
  SO_KIT_CATALOG_ENTRY_HEADER(imageTransform);
  SO_KIT_CATALOG_ENTRY_HEADER(imageMaterial);
  SO_KIT_CATALOG_ENTRY_HEADER(imageSwitch);     
  SO_KIT_CATALOG_ENTRY_HEADER(imageGroup);
  SO_KIT_CATALOG_ENTRY_HEADER(imageTranslation);
  SO_KIT_CATALOG_ENTRY_HEADER(image);
  SO_KIT_CATALOG_ENTRY_HEADER(textureGroup);
  SO_KIT_CATALOG_ENTRY_HEADER(textureQuality);
  SO_KIT_CATALOG_ENTRY_HEADER(textureImage);
  SO_KIT_CATALOG_ENTRY_HEADER(textureShape);
  SO_KIT_CATALOG_ENTRY_HEADER(tickMaterial);
  SO_KIT_CATALOG_ENTRY_HEADER(renderCallbackLines);
  SO_KIT_CATALOG_ENTRY_HEADER(textMaterial);
  SO_KIT_CATALOG_ENTRY_HEADER(renderCallbackText);
  SO_KIT_CATALOG_ENTRY_HEADER(extraNodes);

public:
  LegendKit(void);

  static void initClass(void);

public:

  SoSFBool on;
  SoSFFloat imageWidth;
  SoSFFloat space;
  SoSFFloat bigTickSize;
  SoSFFloat smallTickSize;
  SoSFString tickValueFormat;
  SoSFFloat tickValueOffset;
  SoMFString minvalue;
  SoMFString maxvalue;
  SoSFBool delayedRender;
  SoSFFloat topSpace;
  SoSFBool discreteUseLower;
  SoSFBool threadSafe;
  SbVec2s size;
  bool istop;
  bool isleft;
  float lenmin;


  void preRender(SoAction * action);
  
  void setImageTransparency(const float transparency = 0.0f);
  void useTextureNotImage(const SbBool onoff);
  void setColorCB(uint32_t (*colorCB)(double, void*), void * userdata = NULL);
  void setColorCB(uint32_t (*colorCB)(double));
  
  void clearTicks(void);
  void addSmallTick(double nval);
  void addBigTick(double nval, double tickvalue, const SbString * discretestring = NULL);
  void addBigTick(double nval, const SbString & string, const SbString * discretestring = NULL);

  void clearColors(void);
  void setDiscreteMode(const SbBool onoff);
  void addDiscreteColor(double uppernval, uint32_t color);
  void addDiscreteColor(double uppernval);
  
  void clearData(void);
  void enableImage(const SbBool onoff);

  float getLegendWidth(void) const;

public:
  // convenience methods for setting part attributes
  void setTickAndLinesColor(const SbColor & color, const float transparency = 0.0f);
  void setTextColor(const SbColor & color, const float transparency = 0.0f);
  void setPosition(const SbVec2s & pos);
  void setBackgroundColor(const SbColor & color, const float transparency = 0.0f);
  void enableBackground(const SbBool onoff);

protected:
  virtual void GLRender(SoGLRenderAction * action);
  virtual void getBoundingBox(SoGetBoundingBoxAction * action);
  virtual void handleEvent(SoHandleEventAction * action);
  virtual void search(SoSearchAction * action);
  virtual void callback(SoCallbackAction * action);
  virtual void getMatrix(SoGetMatrixAction * action);
  virtual void pick(SoPickAction * action);
  virtual void rayPick(SoRayPickAction * action);
  virtual void audioRender(SoAudioRenderAction * action);
  virtual void getPrimitiveCount(SoGetPrimitiveCountAction * action);

  virtual SbBool affectsState(void) const;

  virtual ~LegendKit();
  virtual void notify(SoNotList * list);
  
  virtual void setDefaultOnNonWritingFields();

private:

  void recalcSize(SoState * state);
  void initBackground(const SbBool force = FALSE);
  void initTextureImage(void);
  void initImage(void);
  void reallyInitImage(unsigned char * data, unsigned char * rowdata = NULL);
  void fillImageAlpha(void);
  static void renderCBlines(void * userdata, SoAction * action);
  static void renderCBtext(void * userdata, SoAction * action);
  void render(SoGLRenderAction * action, const SbBool lines);
  void renderLines(SoGLRenderAction * action);
  void renderText(SoGLRenderAction * action);
  void renderString(const char * str, int xpos, int ypos);

  void setSwitchValue(const char * part, const int value);

  class LegendKitP * pimpl;
  friend class LegendKitP;
};

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <SoWinEnterScope.h>
#endif // win

#endif // !SMALLCHANGE_LEGENDKIT_H

