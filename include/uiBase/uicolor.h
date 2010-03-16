#ifndef uicolor_h
#define uicolor_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          22/05/2000
 RCS:           $Id: uicolor.h,v 1.21 2010-03-16 10:02:46 cvsbert Exp $
________________________________________________________________________

-*/

#include "color.h"
#include "uigroup.h"

class uiLabel;
class uiPushButton;
class uiCheckBox;


/*! \brief pops a selector box to select a new color 
     \return true if new color selected
*/
mGlobal bool  	selectColor(Color&,uiParent* parnt=0,const char* seltxt=0,
		    bool withtransp=false); 

// To be used by cmddriver to select a color while closing the QColorDialog
mGlobal void		setExternalColor( const Color& );
static Color*	externalcolor = 0;


/*! \brief small element for color selection. Has no text label.

  The label in this class is for displaying the current color. Another label
  may be created if you specify the lbltxt.
 
 */

mClass uiColorInput : public uiGroup
{
public:
    mClass Setup
    {
    public:
			Setup(const Color& col)
			    : color_(col)
			    , lbltxt_("")
			    , withcheck_(false)
			    , dlgtitle_("Select color")
			    , withalpha_(false)
			{}

	mDefSetupMemb(Color,color)
	mDefSetupMemb(BufferString,lbltxt)
	mDefSetupMemb(bool,withcheck)
	mDefSetupMemb(BufferString,dlgtitle)
	mDefSetupMemb(bool,withalpha)

    };

    				uiColorInput(uiParent*,const Setup&,
					     const char* nm=0);

    const Color&		color() const	{ return color_; }
    void			setColor(const Color&);
    bool			doDraw() const;
    void			setDoDraw(bool);
    void			setLblText(const char*);

    void			enableAlphaSetting( bool yn )
				{ withalpha_ = yn; }

    Notifier<uiColorInput> 	colorChanged;
    Notifier<uiColorInput> 	doDrawChanged;

    uiPushButton*		getButton()	{ return colbut_; }

protected:

    uiPushButton*		colbut_;
    uiCheckBox*			dodrawbox_;
    uiLabel*			uilbl_;

    Color			color_;
    BufferString		dlgtxt_;
    bool			withalpha_;

    void			selCol(CallBacker*);
    void			dodrawSel(CallBacker*);
};

#endif
