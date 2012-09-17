#ifndef uiwellstratdisplay_h
#define uiwellstratdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
 RCS:           $Id: uiwellstratdisplay.h,v 1.19 2012/06/27 14:08:52 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uistratdisplay.h"
#include "uiwelldahdisplay.h"

/*!\brief creates a display of stratigraphy IF levels are linked to markers.*/

namespace Well { class Data; }

mClass WellStratUnitGen : public CallBacker
{
public:
				WellStratUnitGen(StratDispData&,
						const ObjectSet<Well::Marker>&,
						const Well::D2TModel*);
				~WellStratUnitGen();

    const StratDispData&	stratData() const { return data_; }
    void			gatherInfo();

protected:

    uiStratTreeToDispTransl*	uidatagather_;

    ObjectSet<StratDispData::Unit> leaveddispunits_;
    ObjectSet<const Strat::LeavedUnitRef> leavedunits_;
    TypeSet<float>		posset_;

    ObjectSet<StratDispData::Unit> dispunits_;
    ObjectSet<const Strat::NodeOnlyUnitRef> units_;

    StratDispData&		data_;
    const ObjectSet<Well::Marker>& markers_;
    const Well::D2TModel*	d2tmodel_;

    void 			assignTimesToLeavedUnits();
    void 			assignTimesToAllUnits();
    const Well::Marker* 	getMarkerFromLvlID(int lvlid) const;
    void 			gatherLeavedUnits();
    bool			areLeavedTied(const Strat::LeavedUnitRef&,
					    const Strat::LeavedUnitRef&) const;

    void			dataChangedCB(CallBacker*) { gatherInfo(); };

public:
				WellStratUnitGen(StratDispData&,
						const Well::Data&);

				//4.4
    const Well::Track*          getTrack() const { return 0; };
    void                        setTrack(const Well::Track) {};
};


mClass uiWellStratDisplay : public uiWellDahDisplay
{
public:

				uiWellStratDisplay(uiParent*);
				~uiWellStratDisplay();

    int				transparency() const  
    					{ return transparency_; }
    void			setTransparency(int t) 
    					{ transparency_ = t; dataChanged(); }

    const StratDispData& 	stratData() const { return data_; }
protected:

    WellStratUnitGen*		stratgen_;

    StratDispData		data_;
    uiStratDrawer               drawer_;
  
    int				transparency_;

    void			dataChangedCB(CallBacker*)
				{ dataChanged(); }
    void			gatherInfo();
    void			draw();


};


#endif

