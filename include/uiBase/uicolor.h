#ifndef uicolor_h
#define uicolor_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          22/05/2000
 RCS:           $Id: uicolor.h,v 1.5 2002-02-21 09:24:55 nanne Exp $
________________________________________________________________________

-*/

#include "color.h"
#include "uibutton.h"

class uiObject;

	/*! \brief pops a selector box to select a new color 
             \return true if new color selected
	*/
bool  	select( Color&, uiParent* parnt=0, const char* nm=0,
		bool withtransp=false); 


//! \brief This button will have the selected color as background color.
class uiColorInput : public uiPushButton
{
public:

			uiColorInput(uiParent*,const Color&,
				     const char* seltxt="Select color",
				     const char* buttxt="Color ...");

    const Color&	color() const	{ return color_; }

    void		setColor( Color& col_ );

protected:

    Color		color_;
    BufferString	seltxt_;

    void		pushed(CallBacker*);
};


#endif
