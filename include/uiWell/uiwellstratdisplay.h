#ifndef uiwellstratdisplay_h
#define uiwellstratdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
 RCS:           $Id: uiwellstratdisplay.h,v 1.13 2010-09-17 12:26:07 cvsbruno Exp $
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

    const AnnotData&		annotData() const { return annots_; }

    void			setTransparency(int t) 
    				{ transparency_ = t; dataChanged(); }

protected:

    uiStratTreeToDispTransl*	uidatagather_;
    ObjectSet<const AnnotData::Unit> orgunits_;
    AnnotData			annots_;
    uiAnnotDrawer               drawer_;
  
    void			dataChangedCB(CallBacker*)
				{ dataChanged(); }
    void			gatherInfo();
    void			draw();
    void			gatherOrgUnits();
    const AnnotData::Unit*	getNextTimeUnit(float) const;
    void 			setUnitBotPos(AnnotData::Unit&);
    void 			setUnitTopPos(AnnotData::Unit&);
    float			getPosFromMarkers(const AnnotData::Unit&) const;
    float 			getPosMarkerLvlMatch(int) const;

    int				transparency_;

};

#endif


