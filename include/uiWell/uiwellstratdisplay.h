#ifndef uiwellstratdisplay_h
#define uiwellstratdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
 RCS:           $Id: uiwellstratdisplay.h,v 1.10 2010-08-05 11:50:33 cvsbruno Exp $
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

protected:

    Well::Well2DDispData  	dispdata_;
    uiStratTreeToDispTransl	uidatagather_;
  
    virtual void		dataChanged(CallBacker*);
    const AnnotData::Unit*	getNextTimeUnit(float) const;
    void 			setUnitBotPos(AnnotData::Unit&);
    void 			setUnitTopPos(AnnotData::Unit&);
    float			getPosFromMarkers(const AnnotData::Unit&) const;
    float 			getPosMarkerLvlMatch(int) const;

};

#endif


