#ifndef uimathexpression_h
#define uimathexpression_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2011
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uigroup.h"
class uiLineEdit;
class uiComboBox;
class uiPushButton;
class uiToolButton;
class uiToolButtonSetup;


mClass uiMathExpression : public uiGroup
{
public:

    mClass Setup
    {
    public:
			Setup( const char* lbl=0 )
			    : label_(lbl)
			    , withsetbut_(false)
			    , withfns_(true)
			    , fnsbelow_(true)		{}

	mDefSetupMemb(bool,withfns);
	mDefSetupMemb(bool,fnsbelow);
	mDefSetupMemb(bool,withsetbut);
	mDefSetupMemb(BufferString,label);
	mDefSetupMemb(CallBack,setcb);
		// if withsetbut and not set, will do returnpress
    };

			uiMathExpression(uiParent*,const Setup&);

    void		setText(const char*);
    void		insertText(const char*);

    const char*		text();
    uiLineEdit*		textField()		{ return txtfld_; }
    uiToolButton*	addButton(const uiToolButtonSetup&);
    //!< attach this yourself if it's the first and you have no 'Set' button

    Notifier<uiMathExpression>	formSet;
    void		extFormSet()		{ retPressCB(0); }

protected:

    uiLineEdit*		txtfld_;
    uiComboBox*		grpfld_;
    uiComboBox*		fnfld_;
    uiPushButton*	setbut_;
    uiToolButton*	lastbut_;
    Setup		setup_;

    void		grpSel(CallBacker*);
    void		doIns(CallBacker*);
    void		setButCB(CallBacker*);
    void		retPressCB(CallBacker*);

};


#endif
