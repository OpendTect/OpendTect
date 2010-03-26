#ifndef uiwellstratdisplay_h
#define uiwellstratdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
 RCS:           $Id: uiwellstratdisplay.h,v 1.2 2010-03-26 10:39:27 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uistratdisplay.h"

namespace Well 
{ 
    class Marker; 
    class D2TModel; 
}

namespace Strat{ class UnitRef; }

/*!\brief creates a display of stratigraphy IF levels are linked to markers.*/
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
    Interval<float> 		getUnitPos(const Strat::Level&,
	    				   const Strat::Level&);
};

#endif


