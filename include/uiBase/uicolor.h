#ifndef uicolor_h
#define uicolor_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          22/05/2000
 RCS:           $Id: uicolor.h,v 1.3 2001-08-23 14:59:17 windev Exp $
________________________________________________________________________

-*/

#include "color.h"
#include "uibutton.h"

class uiObject;

bool  	select( Color&, uiParent* parnt=0, const char* nm=0); 
	/*!< \brief pops a selector box to select a new color 
             \return true if new color selected
	*/


class uiColorInput : public uiPushButton
{
//!< \brief This button will have the selected color as background color.
public:

			uiColorInput(uiParent*,const Color&,
				     const char* seltxt="Select color",
				     const char* buttxt="Color ...");

    const Color&	color() const	{ return color_; }

protected:

    Color		color_;
    BufferString	seltxt_;

    void		pushed(CallBacker*);
};


#endif
