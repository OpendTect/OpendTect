#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "color.h"
#include "uigroup.h"

class uiLabel;
class uiPushButton;
class uiCheckBox;
class uiComboBox;
class uiSpinBox;


/*! \brief pops a selector box to select a new color 
     \return true if new color selected
*/
mGlobal(uiBase) bool selectColor(OD::Color&,uiParent* parnt=nullptr,
				 uiString=uiString::emptyString(),
				 bool withtransp=false);

// To be used by cmddriver to select a color while closing the QColorDialog
mGlobal(uiBase) void		setExternalColor(const OD::Color&);


/*! \brief small element for color selection. Has no text label.

  The label in this class is for displaying the current color. Another label
  may be created if you specify the lbltxt.
 
 */

mExpClass(uiBase) uiColorInput : public uiGroup
{ mODTextTranslationClass(uiColorInput)
public:
    mExpClass(uiBase) Setup
    {
    public:

	enum TranspHndlng	{ None, InSelector, Separate };

			Setup( const OD::Color& col, TranspHndlng h=None )
				: color_(col)
				, withcheck_(false)
				, dlgtitle_(uiColorInput::sSelColor())
				, transp_(h)
				, withdesc_(h != Separate)
			{}
			~Setup()
			{}

	mDefSetupMemb(OD::Color,color)
	mDefSetupMemb(uiString,lbltxt)
	mDefSetupMemb(bool,withcheck)
	mDefSetupMemb(uiString,dlgtitle)
	mDefSetupMemb(TranspHndlng,transp)
	mDefSetupMemb(bool,withdesc)

    };

				uiColorInput(uiParent*,const Setup&,
					     const char* nm=nullptr);
				~uiColorInput();
				mOD_DisableCopy(uiColorInput)

    const OD::Color&		color() const	{ return color_; }
    void			setColor(const OD::Color&);
    bool			doDraw() const;
    void			setDoDraw(bool);
    void			setLblText(const uiString&);

    Notifier<uiColorInput>	colorChanged;
    Notifier<uiColorInput>	doDrawChanged;

    uiPushButton*		getButton()	{ return colbut_; }
    uiComboBox*			getDescCombo()	{ return descfld_; }

    static uiString		sSelColor();
    static bool			selectColor(OD::Color& col,uiParent*,
						    uiString,bool withtransp);

private:

    uiPushButton*		colbut_;
    uiCheckBox*			dodrawbox_;
    uiSpinBox*			transpfld_;
    uiComboBox*			descfld_;
    uiLabel*			lbl_;

    OD::Color			color_;
    uiString			dlgtxt_;
    bool			selwithtransp_;

    void			selCol(CallBacker*);
    void			dodrawSel(CallBacker*);
    void			descSel(CallBacker*);
    void			transpChg(CallBacker*);
};
