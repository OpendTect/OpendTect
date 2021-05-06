#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2011
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "uistrings.h"
class uiLineEdit;
class uiComboBox;
class uiPushButton;
class uiButton;
class uiToolButtonSetup;
namespace Math { class SpecVarSet; }


mExpClass(uiTools) uiMathExpression : public uiGroup
{ mODTextTranslationClass(uiMathExpression);
public:

    mExpClass(uiTools) Setup
    {
    public:
			Setup( const uiString& lbl=uiStrings::sEmptyString() )
			    : label_(lbl)
			    , withsetbut_(false)
			    , withfns_(true)
			    , fnsbelow_(true)
			    , specvars_(0)		{}

	mDefSetupMemb(bool,withfns);
	mDefSetupMemb(bool,fnsbelow);
	mDefSetupMemb(bool,withsetbut);
	mDefSetupMemb(uiString,label);
	mDefSetupMemb(const Math::SpecVarSet*,specvars);
	mDefSetupMemb(CallBack,setcb);
		// if withsetbut and not set, will do returnpress
    };

			uiMathExpression(uiParent*,const Setup&);

    void		setText(const char*);
    void		insertText(const char*);

    const char*		text();
    uiLineEdit*		textField()		{ return txtfld_; }
    uiButton*		addButton(const uiToolButtonSetup&);
    //!< attach this yourself if it's the first and you have no 'Set' button

    Notifier<uiMathExpression>	formSet;
    void		extFormSet()		{ retPressCB(0); }

protected:

    uiLineEdit*		txtfld_;
    uiComboBox*		grpfld_;
    uiComboBox*		fnfld_;
    uiPushButton*	setbut_;
    uiButton*		lastbut_;
    Setup		setup_;

    void		grpSel(CallBacker*);
    void		doIns(CallBacker*);
    void		setButCB(CallBacker*);
    void		retPressCB(CallBacker*);

};


