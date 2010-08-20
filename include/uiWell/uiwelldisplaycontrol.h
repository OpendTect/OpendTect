#ifndef uiwelldisplaycontrol_h
#define uiwelldisplaycontrol_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2009
 RCS:           $Id: uiwelldisplaycontrol.h,v 1.11 2010-08-20 15:02:27 cvsbruno Exp $
________________________________________________________________________

-*/


#include "callback.h"

class KeyboardEventHandler;
class MouseEventHandler;
class uiWellLogDisplay;

namespace Well { class Marker; }

mClass uiWellDisplayControl : public CallBacker
{
public:
				uiWellDisplayControl(uiWellLogDisplay&);
				~uiWellDisplayControl(){};

    void			addLogDisplay(uiWellLogDisplay&);
    void			removeLogDisplay(uiWellLogDisplay&);

    float 			mousePos() const; 
    bool			isMouseDown() const 	{ return ismousedown_;} 
    void			setMouseDown(bool yn)   { ismousedown_ = yn; } 

    const uiWellLogDisplay*	selLogDisp() const	{ return seldisp_; }
    Well::Marker*		selMarker()		{ return selmarker_; }

    KeyboardEventHandler*	keyboardEventHandler(); 
    MouseEventHandler*		mouseEventHandler();
    
    Notifier<uiWellDisplayControl>  posChanged;
    Notifier<uiWellDisplayControl>  mousePressed;
    Notifier<uiWellDisplayControl>  mouseReleased;

protected:

    ObjectSet<uiWellLogDisplay> logdisps_;
    uiWellLogDisplay* 		seldisp_;

    BufferString                info_;
    bool			ismousedown_;

    Well::Marker* 		selmarker_;
    Well::Marker* 		lastselmarker_;
    
    void			setSelLogDispCB(CallBacker*);
    void			setSelMarkerCB(CallBacker*);

    MouseEventHandler& 		mouseEventHandler(int);
    KeyboardEventHandler& 	keyboardEventHandler(int);

    void 			getPosInfo(BufferString&) const;

    void 			mouseMovedCB(CallBacker*);
    void                        mousePressedCB(CallBacker*);
    void                        mouseReleasedCB(CallBacker*);
    void			keyPressedCB(CallBacker*);
    void			keyReleasedCB(CallBacker*);
    void			setPosInfo(CallBacker*);
};

#endif
