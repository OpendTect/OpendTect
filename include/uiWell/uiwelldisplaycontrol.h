#ifndef uiwelldisplaycontrol_h
#define uiwelldisplaycontrol_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2009
 RCS:           $Id$
________________________________________________________________________

-*/


#include "callback.h"

class MouseEventHandler;
class uiWellDahDisplay;

namespace Well { class Marker; }

mClass uiWellDisplayControl : public CallBacker
{
public:
				uiWellDisplayControl(uiWellDahDisplay&);
				~uiWellDisplayControl();

    void			addDahDisplay(uiWellDahDisplay&);
    void			removeDahDisplay(uiWellDahDisplay&);
    void			clear();

    bool			isMouseDown() const 	{ return ismousedown_;} 
    void			setMouseDown(bool yn)   { ismousedown_ = yn; } 
    bool			isCtrlPressed() const	{return isctrlpressed_;}
    void			setCtrlPressed(bool);

    const uiWellDahDisplay*	selDahDisplay() const	{ return seldisp_; }
    const Well::Marker*		selMarker() const	{ return selmarker_; }
    const Well::Marker*		lastValidMarker() const {return lastselmarker_;}

    void			setSelMarker(const Well::Marker*);

    float			time() const 	{ return time_; }
    float			depth() const	{ return depth_; }
    float			xPos() const	{ return xpos_; }
    float			yPos() const	{ return ypos_; }

    MouseEventHandler*		mouseEventHandler();
    
    Notifier<uiWellDisplayControl>  posChanged;
    Notifier<uiWellDisplayControl>  mousePressed;
    Notifier<uiWellDisplayControl>  mouseReleased;

    Notifier<uiWellDisplayControl> markerSel;

protected:

    ObjectSet<uiWellDahDisplay> logdisps_;
    uiWellDahDisplay* 		seldisp_;

    BufferString                info_;
    bool			ismousedown_;
    bool			isctrlpressed_;

    const Well::Marker* 	selmarker_;
    const Well::Marker* 	lastselmarker_;

    void			highlightMarker(const Well::Marker&,bool);
    MouseEventHandler& 		mouseEventHandler(int);

    void 			getPosInfo(BufferString&) const;
    float 			mousePos() const; 
    
    float			time_;
    float			depth_;
    float			xpos_;
    float			ypos_;

    void 			mouseMovedCB(CallBacker*);
    void                        mousePressedCB(CallBacker*);
    void                        mouseReleasedCB(CallBacker*);
    void			setPosInfo(CallBacker*);
    void			setSelDahDisplay(CallBacker*);
    void			setSelMarkerCB(CallBacker*);
};

#endif
