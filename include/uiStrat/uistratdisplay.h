#ifndef uistratdisplay_h
#define uistratdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
 RCS:           $Id: uistratdisplay.h,v 1.8 2010-04-23 08:36:27 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h"
#include "uiaxishandler.h"
#include "menuhandler.h"
#include "uistratdispdata.h"

class uiMenuHandler;
class uiParent;
class uiPolygonItem;
class uiTextItem;
class MouseEvent;

mClass uiStratDisplay : public uiGraphicsView
{
public:
				uiStratDisplay(uiParent*);
				~uiStratDisplay();

    void			setZRange( StepInterval<float> rg ) 
    				{ yax_.setBounds(rg); draw(); }
    virtual void		doDataChange(CallBacker*);
    int				nrUnits() const { return data_.units_.size(); }
    int				nrLevels() const { return data_.levels_.size();}
    uiStratDisp::Level* 	getLevel(int idx) { return data_.levels_[idx]; }
    uiStratDisp::Unit* 		getUnit(int idx) { return data_.units_[idx]; }
    const uiStratDisp::Level* 	getLevel(int i) const {return data_.levels_[i];}
    const uiStratDisp::Unit* 	getUnit(int i) const { return data_.units_[i]; }
    
    uiAxisHandler& 		xAxis() 	{ return yax_; }
    uiAxisHandler& 		yAxis() 	{ return yax_; }

protected:
  
    uiStratDisp			data_;

    ObjectSet<uiTextItem>	txtitms_;
    ObjectSet<uiLineItem>	lvlitms_;
    ObjectSet<uiPolygonItem>	unititms_;

    uiAxisHandler 		yax_; 
    uiAxisHandler 		xax_; 

    uiMenuHandler&      	menu_;
    MenuItem            	addunitmnuitem_;
    MenuItem            	remunitmnuitem_;

    void			dataChanged();
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
