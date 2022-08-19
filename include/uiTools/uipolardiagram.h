#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigraphicsview.h"

class uiCircleItem;
class uiLineItem;
class uiMarkerItem;
class uiTextItem;

/*!
\brief Displays a polar diagram which can be used to set the azimuth and the dip
by gyrating the pointer. Azimuth is set by circular motion while dip is set
by linear motion towards or away from the center.
*/

mExpClass(uiTools) uiPolarDiagram : public uiGraphicsView
{ mODTextTranslationClass(uiPolarDiagram);
public:

				uiPolarDiagram(uiParent*);
				~uiPolarDiagram();

    // angles in user degrees
    void			setValues(float azimuth, float dip);
    void			getValues(float* azimuth, float* dip) const;

    Notifier<uiPolarDiagram>	valueChanged;

protected:

    void			draw();
    void			drawCircles();
    void			drawSegments();
    void			drawPointer();
    void			updatePointer();
    void            	    	mouseEventCB(CallBacker*);
    void            	    	reSizedCB(CallBacker*);

    uiPoint			center_;
    int				radius_;
    float			azimuth_;  // user degrees
    float			dip_;  // degrees

    ObjectSet<uiCircleItem>	circleitms_;
    ObjectSet<uiLineItem>	segmentitms_;
    uiMarkerItem*		pointeritm_;
    ObjectSet<uiTextItem>	azimuthtextitms_;
    ObjectSet<uiTextItem>	diptextitms_;

};
