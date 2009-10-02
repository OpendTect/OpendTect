#ifndef uipolardiagram_h
#define uipolardiagram_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Karthika
 Date:          Sep 2009
 RCS:           $Id: uipolardiagram.h,v 1.1 2009-10-02 15:49:37 cvskarthika Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h"

class uiCircleItem;
class uiLineItem;
class uiMarkerItem;
class uiTextItem;

mClass uiPolarDiagram : public uiGraphicsView
{
public:

				uiPolarDiagram(uiParent*);
				~uiPolarDiagram();

    // angles in degrees
    void			setValues(float azimuth, float dip);
    void			getValues(float* azimuth, float* dip) const;

protected:

    void			draw();
    void			drawClock();
    void			drawSegments();
    void			drawPointer();
    void			updatePointer();
    void            	    	mouseRelease(CallBacker*);
    void            	    	reSized(CallBacker*);

    uiPoint			center_;
    int				radius_;
    float			azimuth_;
    float			dip_;

    uiCircleItem*		circleitm_;
    ObjectSet<uiLineItem>	segmentitms_;
    uiMarkerItem*		pointeritm_;
    ObjectSet<uiTextItem>	textitms_;

};


#endif
