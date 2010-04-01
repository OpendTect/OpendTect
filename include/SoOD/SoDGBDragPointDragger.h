#ifndef SoDGBDragPointDragger_h
#define SoDGBDragPointDragger_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          March 2010
 RCS:           $Id: SoDGBDragPointDragger.h,v 1.2 2010-04-01 13:52:58 cvskris Exp $
________________________________________________________________________


-*/

#include "soodbasic.h"


/*!\brief
This class is basically a SoDGBDragPointDragger, which overcomes the undesirable 
effects of the SoDGBDragPointDragger when it is very small. See src file for more 
details.
*/

#include <Inventor/draggers/SoDragger.h>
#include <Inventor/tools/SbLazyPimplPtr.h>
#include <Inventor/fields/SoSFVec3f.h>

class SoSensor;
class SoFieldSensor;
class SoDGBDragPointDraggerP;

mClass SoDGBDragPointDragger : public SoDragger {
  typedef SoDragger inherited;

  SO_KIT_HEADER(SoDGBDragPointDragger);
  SO_KIT_CATALOG_ENTRY_HEADER(noRotSep);
  SO_KIT_CATALOG_ENTRY_HEADER(planeFeedbackSep);
  SO_KIT_CATALOG_ENTRY_HEADER(planeFeedbackSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(planeFeedbackTranslation);
  SO_KIT_CATALOG_ENTRY_HEADER(rotX);
  SO_KIT_CATALOG_ENTRY_HEADER(rotXSep);
  SO_KIT_CATALOG_ENTRY_HEADER(rotY);
  SO_KIT_CATALOG_ENTRY_HEADER(rotYSep);
  SO_KIT_CATALOG_ENTRY_HEADER(rotZ);
  SO_KIT_CATALOG_ENTRY_HEADER(rotZSep);
  SO_KIT_CATALOG_ENTRY_HEADER(xFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(xFeedbackSep);
  SO_KIT_CATALOG_ENTRY_HEADER(xFeedbackSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(xFeedbackTranslation);
  SO_KIT_CATALOG_ENTRY_HEADER(xTranslator);
  SO_KIT_CATALOG_ENTRY_HEADER(xTranslatorSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(xyFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(xyTranslator);
  SO_KIT_CATALOG_ENTRY_HEADER(xyTranslatorSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(xzFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(xzTranslator);
  SO_KIT_CATALOG_ENTRY_HEADER(xzTranslatorSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(yFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(yFeedbackSep);
  SO_KIT_CATALOG_ENTRY_HEADER(yFeedbackSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(yFeedbackTranslation);
  SO_KIT_CATALOG_ENTRY_HEADER(yTranslator);
  SO_KIT_CATALOG_ENTRY_HEADER(yTranslatorSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(yzFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(yzTranslator);
  SO_KIT_CATALOG_ENTRY_HEADER(yzTranslatorSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(zFeedback);
  SO_KIT_CATALOG_ENTRY_HEADER(zFeedbackSep);
  SO_KIT_CATALOG_ENTRY_HEADER(zFeedbackSwitch);
  SO_KIT_CATALOG_ENTRY_HEADER(zFeedbackTranslation);
  SO_KIT_CATALOG_ENTRY_HEADER(zTranslator);
  SO_KIT_CATALOG_ENTRY_HEADER(zTranslatorSwitch);

public:
  static void initClass(void);
  SoDGBDragPointDragger(void);

  SoSFVec3f translation;

  void setJumpLimit(const float limit);
  float getJumpLimit(void) const;
  void showNextDraggerSet(void);

protected:
  virtual ~SoDGBDragPointDragger(void);
  virtual SbBool setUpConnections(SbBool onoff, SbBool doitalways = FALSE);
  virtual void setDefaultOnNonWritingFields(void);

  void dragStart(void);
  void drag(void);
  void dragFinish(void);

  static void startCB(void * f, SoDragger * d);
  static void motionCB(void * f, SoDragger * d);
  static void finishCB(void * f, SoDragger * d);
  static void fieldSensorCB(void * f, SoSensor * s);
  static void valueChangedCB(void * f, SoDragger * d);

  SoFieldSensor * fieldSensor;

private:
  void registerDragger(SoDragger *dragger);
  void unregisterDragger(const char *name);
  void updateSwitchNodes();
  int currAxis;
  float jumpLimit;

private:
  static const char* 	draggergeometry_;

  // NOT IMPLEMENTED:
  SoDGBDragPointDragger(const SoDGBDragPointDragger & rhs);
  SoDGBDragPointDragger & operator = (const SoDGBDragPointDragger & rhs);
}; // SoDGBDragPointDragger

#endif

