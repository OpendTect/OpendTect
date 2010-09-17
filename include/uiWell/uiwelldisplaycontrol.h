#ifndef uiwelldisplaycontrol_h
#define uiwelldisplaycontrol_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2009
 RCS:           $Id: uiwelldisplaycontrol.h,v 1.13 2010-09-17 12:26:07 cvsbruno Exp $
________________________________________________________________________

-*/


#include "callback.h"

class MouseEventHandler;
class uiWellLogDisplay;

namespace Well { class Marker; }

mClass uiWellDisplayControl : public CallBacker
{
public:
				uiWellDisplayControl(uiWellLogDisplay&);
				~uiWellDisplayControl();

    void			addLogDisplay(uiWellLogDisplay&);
    void			removeLogDisplay(uiWellLogDisplay&);
    void			clear();

    bool			isMouseDown() const 	{ return ismousedown_;} 
    void			setMouseDown(bool yn)   { ismousedown_ = yn; } 
    bool			isCtrlPressed() const	{return isctrlpressed_;}
    void			setCtrlPressed(bool);

    const uiWellLogDisplay*	selLogDisp() const	{ return seldisp_; }
    const Well::Marker*		selMarker() const	{ return selmarker_; }
    const Well::Marker*		lastValidMarker() const {return lastselmarker_;}

    void			setSelMarker(const Well::Marker*);

    float			time()  	{ return time_; }
    float			depth()		{ return depth_; }

    MouseEventHandler*		mouseEventHandler();
    
    Notifier<uiWellDisplayControl>  posChanged;
    Notifier<uiWellDisplayControl>  mousePressed;
    Notifier<uiWellDisplayControl>  mouseReleased;

    Notifier<uiWellDisplayControl> markerSel;

protected:

    ObjectSet<uiWellLogDisplay> logdisps_;
    uiWellLogDisplay* 		seldisp_;

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

    void 			mouseMovedCB(CallBacker*);
    void                        mousePressedCB(CallBacker*);
    void                        mouseReleasedCB(CallBacker*);
    void			setPosInfo(CallBacker*);
    void			setSelLogDispCB(CallBacker*);
    void			setSelMarkerCB(CallBacker*);
};

#endif
