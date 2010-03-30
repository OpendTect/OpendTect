#ifndef uistratdisplay_h
#define uistratdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
 RCS:           $Id: uistratdisplay.h,v 1.4 2010-03-30 12:04:01 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h"
#include "uiaxishandler.h"
#include "menuhandler.h"
#include "stratdisp.h"

class uiMenuHandler;
class uiParent;
class uiPolygonItem;
class uiTextItem;
class MouseEvent;

mClass uiStratDisplay : public uiGraphicsView
{
public:
				uiStratDisplay(uiParent*);

    void			setZRange( StepInterval<float> rg ) 
    				{ zax_.setBounds(rg); draw(); }
    void			dataChanged();
    int				nrUnits() 	{ return data_.units_.size(); }
    int				nrLevels() 	{ return data_.levels_.size(); }
    StratDisp::Level* 		getLevel(int idx) { return data_.levels_[idx]; }
    StratDisp::Unit* 		getUnit(int idx) { return data_.units_[idx]; }

protected:
  
    StratDisp			data_;

    ObjectSet<uiTextItem>	txtitms_;
    ObjectSet<uiLineItem>	lvlitms_;
    ObjectSet<uiPolygonItem>	unititms_;

    uiAxisHandler 		zax_; 

    uiMenuHandler&      	menu_;
    MenuItem            	addunitmnuitem_;
    MenuItem            	remunitmnuitem_;

    void			draw();
    void			drawUnits();
    void			drawLevels();
    virtual void		gatherInfo();
    bool 			handleUserClick(const MouseEvent&);
    int 			nrSubUnits();
    void			updateAxis(); 
    
    void                	createMenuCB(CallBacker*);
    void                	handleMenuCB(CallBacker*);
    void			init(CallBacker*);
    void			reSized(CallBacker*);
    void                	usrClickCB(CallBacker*);
};

#endif
