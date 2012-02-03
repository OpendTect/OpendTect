#ifndef uimathexpression_h
#define uimathexpression_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2011
 RCS:           $Id: uimathexpression.h,v 1.2 2012-02-03 10:47:12 cvsbert Exp $
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
			Setup()
			    : withsetbut_(false)
			    , withfns_(true)
			    , fnsbelow_(true)		{}

	mDefSetupMemb(bool,withfns);
	mDefSetupMemb(bool,fnsbelow);
	mDefSetupMemb(bool,withsetbut);
	mDefSetupMemb(CallBack,setcb);
		// if withsetbut and not set, will do returnpress
    };

			uiMathExpression(uiParent*,const Setup&);
    uiObject*		labelAlignObj();

    void		setText(const char*);
    void		insertText(const char*);

    const char*		text();
    uiLineEdit*		textField()		{ return txtfld_; }
    uiToolButton*	addButton(const uiToolButtonSetup&);

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
