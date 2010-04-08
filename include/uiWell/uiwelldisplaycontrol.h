#ifndef uiwelldisplaycontrol_h
#define uiwelldisplaycontrol_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2009
 RCS:           $Id: uiwelldisplaycontrol.h,v 1.2 2010-04-08 13:13:11 cvsbruno Exp $
________________________________________________________________________

-*/


#include "menuhandler.h"
#include "uiwelllogdisplay.h"

class MouseEvent;
class uiMenuHandler;

namespace Well { class Data; class Marker; }


mClass uiWellDisplayReshape : public CallBacker
{
public:
				uiWellDisplayReshape(uiWellDisplay&);
				~uiWellDisplayReshape(){};

    uiMenuHandler&		menu() { return menu_; }

protected:

    uiMenuHandler&		menu_;
    MenuItem            	addlogmnuitem_;
    MenuItem            	remlogmnuitem_;
    MenuItem            	addstratmnuitem_;
    MenuItem            	remstratmnuitem_;
    
    void                        createMenuCB(CallBacker*);
    void			handleMenuCB(CallBacker*);
    void			addLogPanel();
    void			remLogPanel();
};



mClass uiWellDisplayMarkerEdit : public CallBacker
{
public:
				uiWellDisplayMarkerEdit(uiWellLogDisplay&,
						     Well::Data&);
				~uiWellDisplayMarkerEdit();
    
    uiMenuHandler&		menu() { return menu_; }
    
protected:

    Well::Data&			wd_ ;

    ObjectSet<uiWellLogDisplay> logdisps_;
    Well::Marker* 		selmarker_;
    Well::Marker* 		curmarker_;
    Well::Marker* 		lastmarker_;
    Well::Marker* 		selectMarker(CallBacker*,bool allowrghtclk);

    uiMenuHandler&		menu_;
    MenuItem            	addmrkmnuitem_;
    MenuItem            	remmrkmnuitem_;
    bool                        mousepressed_;
   
    float			mousePos(); 
    void 			addLogDisplay(uiWellLogDisplay&);
    void                        changeMarkerPos(Well::Marker*);
    bool                        handleUserClick(const MouseEvent&);
    void			trigMarkersChanged();

    void                        createMenuCB(CallBacker*);
    void			handleMenuCB(CallBacker*);
    void                        mouseMoved(CallBacker*);
    void                        mousePressed(CallBacker*);
    void			mouseRelease(CallBacker*);
    void 			usrClickCB(CallBacker*);
};

#endif
