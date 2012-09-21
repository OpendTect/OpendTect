#ifndef uipolardiagram_h
#define uipolardiagram_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Sep 2009
 RCS:           $Id$
________________________________________________________________________

 Displays a polar diagram which can be used to set the azimuth and the dip 
 by gyrating the pointer. Azimuth is set by circular motion while dip is set
 by linear motion towards or away from the center.
-*/

#include "uitoolsmod.h"
#include "uigraphicsview.h"

class uiCircleItem;
class uiLineItem;
class uiMarkerItem;
class uiTextItem;

mClass(uiTools) uiPolarDiagram : public uiGraphicsView
{
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


#endif

