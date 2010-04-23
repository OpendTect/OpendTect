#ifndef uiwellstratdisplay_h
#define uiwellstratdisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
 RCS:           $Id: uiwellstratdisplay.h,v 1.5 2010-04-23 10:03:50 cvsbruno Exp $
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
mClass uiWellStratDisplay : public uiStratDisplay
{
public:

				uiWellStratDisplay(uiParent*,bool,
					    const Well::Well2DDispData&);
				~uiWellStratDisplay(){};

    Well::Well2DDispData& 	data()		{ return dispdata_; }
    const Well::Well2DDispData& data() const 	{ return dispdata_; }

protected:

    Well::Well2DDispData  	dispdata_;
  
    void			gatherInfo();
    void 			setUnitPos(uiStratDisp::Unit&);
};

#endif


