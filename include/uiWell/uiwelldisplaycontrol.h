#ifndef uiwelldisplaycontrol_h
#define uiwelldisplaycontrol_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2009
 RCS:           $Id: uiwelldisplaycontrol.h,v 1.12 2010-08-26 14:37:28 cvsbruno Exp $
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
				~uiWellDisplayControl(){};

    void			addLogDisplay(uiWellLogDisplay&);
    void			removeLogDisplay(uiWellLogDisplay&);

    bool			isMouseDown() const 	{ return ismousedown_;} 
    void			setMouseDown(bool yn)   { ismousedown_ = yn; } 
    bool			isCtrlPressed() const	{return isctrlpressed_;}
    void			setCtrlPressed(bool);

    const uiWellLogDisplay*	selLogDisp() const	{ return seldisp_; }
    Well::Marker*		selMarker()		{ return selmarker_; }

    float			time()  	{ return time_; }
    float			depth()		{ return depth_; }

    MouseEventHandler*		mouseEventHandler();
    
    Notifier<uiWellDisplayControl>  posChanged;
    Notifier<uiWellDisplayControl>  mousePressed;
    Notifier<uiWellDisplayControl>  mouseReleased;

protected:

    ObjectSet<uiWellLogDisplay> logdisps_;
    uiWellLogDisplay* 		seldisp_;

    BufferString                info_;
    bool			ismousedown_;
    bool			isctrlpressed_;

    Well::Marker* 		selmarker_;
    Well::Marker* 		lastselmarker_;
    
    void			setSelLogDispCB(CallBacker*);
    void			setSelMarkerCB(CallBacker*);

    MouseEventHandler& 		mouseEventHandler(int);

    void 			getPosInfo(BufferString&) const;
    float 			mousePos() const; 
    
    float			time_;
    float			depth_;

    void 			mouseMovedCB(CallBacker*);
    void                        mousePressedCB(CallBacker*);
    void                        mouseReleasedCB(CallBacker*);
    void			setPosInfo(CallBacker*);
};

#endif
