#ifndef uicolor_h
#define uicolor_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          22/05/2000
 RCS:           $Id: uicolor.h,v 1.11 2004-09-17 07:41:59 nanne Exp $
________________________________________________________________________

-*/

#include "color.h"
#include "uigroup.h"
class uiLabel;


/*! \brief pops a selector box to select a new color 
     \return true if new color selected
*/
bool  	selectColor(Color&,uiParent* parnt=0,const char* seltxt=0,
		    bool withtransp=false); 


/*! \brief small element for color selection. Has no text label.

  The label in this class is for displaying the current color. Another label
  may be created if you specify the lbltxt.
 
 */

class uiColorInput : public uiGroup
{
public:

				uiColorInput(uiParent*,const Color&,
					     const char* buttxt="Color ...",
					     const char* lbltxt=0,
					     const char* seltxt="Select color",
					     bool withalpha=false);
				//!< seltxt is the window caption for the
				//!< Qt color selection dialog.

    const Color&		color() const	{ return color_; }

    void			setColor(const Color&);

    Notifier<uiColorInput> 	colorchanged;

protected:

    Color			color_;
    BufferString		seltxt_;
    bool			withalpha;

    uiLabel*			collbl;

    void			selCol(CallBacker*);
};


#endif
