#ifndef uiwellstratdisplay_h
#define uiwellstratdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
 RCS:           $Id: uiwellstratdisplay.h,v 1.12 2010-09-07 16:03:06 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uistratdisplay.h"
#include "welldisp.h"

namespace Well 
{ 
    class Marker; 
    class D2TModel; 
    class Well2DDispData;
}

/*!\brief creates a display of stratigraphy IF levels are linked to markers.*/
mClass uiWellStratDisplay : public uiAnnotDisplay
{
public:

				uiWellStratDisplay(uiParent*,bool,
					    const Well::Well2DDispData&);
				~uiWellStratDisplay(){};

    Well::Well2DDispData&	dispData() 		{ return dispdata_; }
    const Well::Well2DDispData&	dispData() const 	{ return dispdata_; }
    void			doDataChange()		{ dataChanged(0); }

    void			setTransparency(int t) 
    				{ transparency_ = t; doDataChange(); }

protected:

    Well::Well2DDispData  	dispdata_;
    uiStratTreeToDispTransl	uidatagather_;
    ObjectSet<const AnnotData::Unit> orgunits_;
  
    virtual void		dataChanged(CallBacker*);
    void			gatherOrgUnits();
    const AnnotData::Unit*	getNextTimeUnit(float) const;
    void 			setUnitBotPos(AnnotData::Unit&);
    void 			setUnitTopPos(AnnotData::Unit&);
    float			getPosFromMarkers(const AnnotData::Unit&) const;
    float 			getPosMarkerLvlMatch(int) const;

    int				transparency_;

};

#endif


