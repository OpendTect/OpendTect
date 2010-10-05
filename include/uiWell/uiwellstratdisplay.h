#ifndef uiwellstratdisplay_h
#define uiwellstratdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
 RCS:           $Id: uiwellstratdisplay.h,v 1.16 2010-10-05 15:17:52 cvsbruno Exp $
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

    const StratDispData&	stratData() const 
    					{ return data_; }

    int				transparency() const  
    					{ return transparency_; }
    void			setTransparency(int t) 
    					{ transparency_ = t; dataChanged(); }

protected:

    uiStratTreeToDispTransl*	uidatagather_;

    ObjectSet<StratDispData::Unit> leaveddispunits_;
    ObjectSet<const Strat::LeavedUnitRef> leavedunits_;
    TypeSet<float>		posset_;

    ObjectSet<StratDispData::Unit> dispunits_;
    ObjectSet<const Strat::NodeOnlyUnitRef> units_;

    StratDispData		data_;
    uiStratDrawer               drawer_;
  
    int				transparency_;


    void 			assignTimesToLeavedUnits();
    void 			assignTimesToAllUnits();
    const Well::Marker* 	getMarkerFromLvlID(int lvlid) const;
    void 			gatherLeavedUnits();
    bool			areLeavedTied(const Strat::LeavedUnitRef&,
					    const Strat::LeavedUnitRef&) const;

    void			dataChangedCB(CallBacker*)
				{ dataChanged(); }
    void			gatherInfo();
    void			draw();


};

#endif


