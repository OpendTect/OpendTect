#ifndef uiwellstratdisplay_h
#define uiwellstratdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
 RCS:           $Id: uiwellstratdisplay.h,v 1.3 2010-03-30 12:09:17 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uistratdisplay.h"

namespace Well 
{ 
    class Marker; 
    class D2TModel; 
}

/*!\brief creates a display of stratigraphy WHEN levels are linked to markers.*/
mClass uiWellStratDisplay : public uiStratDisplay
{
public:

				uiWellStratDisplay(uiParent*, bool nobg,
					const  ObjectSet<Well::Marker>&);
				~uiWellStratDisplay(){};

    void			setZIsTime(bool yn) { istime_ = yn; } 
    void			setD2TModel(const Well::D2TModel* dtm)
				{ d2tm_ = dtm; }
 
protected:
  
    bool 			istime_;
    const Well::D2TModel*	d2tm_;
    const ObjectSet<Well::Marker>& markers_;

    void			gatherInfo();
    void 			setUnitPos(StratDisp::Unit&);
};

#endif


