/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          March 2010
________________________________________________________________________

-*/


#include "SoDGBDragPointDragger.h"

#include "Inventor/elements/SoViewVolumeElement.h"
#include "Inventor/SbRotation.h"
#include "iostream"

void SoDGBDragPointDragger::initClass()
{
	SoDragPointDragger::initClass();
}


SoDGBDragPointDragger::SoDGBDragPointDragger()
{
    this->removeStartCallback(SoDragPointDragger::startCB, this);
    this->addStartCallback(SoDGBDragPointDragger::startCB, this);
}


void SoDGBDragPointDragger::startCB(void *d, SoDragger *)
{
  SoDGBDragPointDragger * thisp = static_cast<SoDGBDragPointDragger *>(d);
  thisp->dragStart();
}


void SoDGBDragPointDragger::dragStart(void)
{
    SoDragPointDragger::dragStart();  // to do: remove later

    // In the top-down view, restrict picking the cylinder. User probably wants
    // to move just the rectangle but has picked the cylinder by mistake.
    //
    // In the front view, restrict picking the rectangle. User probably wants
    // to move just the cylinder but has picked the rectangle by mistake.
 
	SbViewVolume vw = getViewVolume();
	SbVec3f worldprojdir = vw.getProjectionDirection();
	const SbMatrix& mat = getWorldToLocalMatrix();
	SbVec3f localprojdir;
	mat.multDirMatrix( worldprojdir, localprojdir );
	localprojdir.normalize();
	//const float angletoz = localprojdir.dot( SbVec3f( 0, 0, 1 ) );
	const float angletoz = localprojdir[2];	


/*SoDragger * activechild = this->getActiveChildDragger();

	assert(activechild != NULL);

	SoSwitch * sw;
	if (activechild->isOfType(SoTranslate2Dragger::getClassTypeId())) {
	sw = SO_GET_ANY_PART(this, "planeFeedbackSwitch", SoSwitch);
	SoInteractionKit::setSwitchValue(sw, this->currAxis);
	}
	else {
	switch (this->currAxis) {
	case 0:
	  sw = SO_GET_ANY_PART(this, "xFeedbackSwitch", SoSwitch);
	  break;
	case 1:
	  sw = SO_GET_ANY_PART(this, "yFeedbackSwitch", SoSwitch);
	  break;
	case 2:
	  sw = SO_GET_ANY_PART(this, "zFeedbackSwitch", SoSwitch);
	  break;
	default:
	  assert(0); sw = NULL; // Dummy assignment to avoid compiler warning.
	  break;
	}
	SoInteractionKit::setSwitchValue(sw, 0);
	}*/

}