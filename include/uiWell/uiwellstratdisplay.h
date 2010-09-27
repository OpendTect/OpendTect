#ifndef uiwellstratdisplay_h
#define uiwellstratdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
 RCS:           $Id: uiwellstratdisplay.h,v 1.14 2010-09-27 11:05:19 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uistratdisplay.h"
#include "uiwelldahdisplay.h"

/*!\brief creates a display of stratigraphy IF levels are linked to markers.*/
mClass uiWellStratDisplay : public uiWellDahDisplay
{
public:

				uiWellStratDisplay(uiParent*);
				~uiWellStratDisplay();

    const StratDispData&	stratData() const { return data_; }

    void			setTransparency(int t) 
    				{ transparency_ = t; dataChanged(); }

protected:

    uiStratTreeToDispTransl*	uidatagather_;
    ObjectSet<const StratDispData::Unit> orgunits_;
    StratDispData		data_;
    uiStratDrawer               drawer_;
  
    void			dataChangedCB(CallBacker*)
				{ dataChanged(); }
    void			gatherInfo();
    void			draw();
    void			gatherOrgUnits();
    const StratDispData::Unit*	getNextTimeUnit(float) const;
    void 			setUnitBotPos(StratDispData::Unit&);
    void 			setUnitTopPos(StratDispData::Unit&);
    float			getPosFromMarkers(
	    				const StratDispData::Unit&) const;
    float 			getPosMarkerLvlMatch(int) const;

    int				transparency_;

};

#endif


