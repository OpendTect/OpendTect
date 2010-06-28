#ifndef uiwelldisplaycontrol_h
#define uiwelldisplaycontrol_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2009
 RCS:           $Id: uiwelldisplaycontrol.h,v 1.10 2010-06-28 04:23:04 cvsnanne Exp $
________________________________________________________________________

-*/


#include "menuhandler.h"

class MouseEvent;
class MouseEventHandler;
class uiMenuHandler;
class uiWellLogDisplay;

namespace Well { class Data; class Marker; }


mClass uiWellDisplayControl : public CallBacker
{
public:
				uiWellDisplayControl(uiWellLogDisplay&, 
							Well::Data*);
				~uiWellDisplayControl();

    Notifier<uiWellDisplayControl>  infoChanged;
    
    void			addLogDisplay(uiWellLogDisplay&);
    void			removeLogDisplay(uiWellLogDisplay&);

    const uiMenuHandler*	menuHandler() const	{ return menu_; }
    void			addMenu(uiMenuHandler*);
    void 			setEditOn(bool yn) 	{ edit_ = yn; }

protected:

    ObjectSet<uiWellLogDisplay> logdisps_;

    BufferString                info_;
    uiMenuHandler*		menu_;
    MenuItem            	addmrkmnuitem_;
    MenuItem            	remmrkmnuitem_;

    Well::Data*			wd_;
    Well::Marker* 		selmarker_;
    Well::Marker* 		curmarker_;
    Well::Marker* 		lastmarker_;

    float 			mousePos(int); 
    bool                        mousepressed_;
    bool			needsave_;
    bool			edit_;
    
    Well::Marker* 		selectMarker(CallBacker*,bool allowrghtclk);
    MouseEventHandler& 		mouseEventHandler(int);

    void                        changeMarkerPos(Well::Marker*);
    void 			getPosInfo(int,float,BufferString&) const;
    bool                        handleUserClick(const MouseEvent&);
    void			trigMarkersChanged();

    void                        createMenuCB(CallBacker*);
    void			handleMenuCB(CallBacker*);
    void 			mouseMoved(CallBacker*);
    void                        mousePressed(CallBacker*);
    void			mouseRelease(CallBacker*);
    void 			usrClicked(CallBacker*);
    void			setPosInfo(CallBacker*);
    void			resetMarkerCursors(CallBacker*);
};

#endif
