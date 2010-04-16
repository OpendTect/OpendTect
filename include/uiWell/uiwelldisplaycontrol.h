#ifndef uiwelldisplaycontrol_h
#define uiwelldisplaycontrol_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2009
 RCS:           $Id: uiwelldisplaycontrol.h,v 1.6 2010-04-16 13:06:11 cvsbruno Exp $
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
    
    const uiMenuHandler*	menu() const { return menu_; }
    void			addMenu(uiMenuHandler*);
    void 			addLogDisplay(uiWellLogDisplay&);
    void 			removeLogDisplay(uiWellLogDisplay&);
    void			setEditOn( bool yn ) { edit_ = yn; }
    
protected:

    Well::Data&			wd_ ;

    ObjectSet<uiWellLogDisplay> logdisps_;
    Well::Marker* 		selmarker_;
    Well::Marker* 		curmarker_;
    Well::Marker* 		lastmarker_;
    Well::Marker* 		selectMarker(CallBacker*,bool allowrghtclk);

    uiMenuHandler*		menu_;
    MenuItem            	addmrkmnuitem_;
    MenuItem            	remmrkmnuitem_;
    bool                        mousepressed_;
    bool 			edit_;
   
    void                        changeMarkerPos(Well::Marker*);
    bool                        handleUserClick(const MouseEvent&);
    float			mousePos(); 
    void			trigMarkersChanged();

    void                        createMenuCB(CallBacker*);
    void			handleMenuCB(CallBacker*);
    void                        mouseMoved(CallBacker*);
    void                        mousePressed(CallBacker*);
    void			mouseRelease(CallBacker*);
    void 			usrClickCB(CallBacker*);
    void			resetMarkerCursors(CallBacker*);
};

#endif
