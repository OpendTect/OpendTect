#ifndef uimathexpression_h
#define uimathexpression_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2011
 RCS:           $Id: uimathexpression.h,v 1.1 2011-09-01 12:16:24 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class uiLineEdit;
class uiComboBox;


mClass uiMathExpression : public uiGroup
{
public:

			uiMathExpression(uiParent*,bool withfns=true);

    void		setText(const char*);
    void		insertText(const char*);

    const char*		text();
    uiLineEdit*		textField()		{ return txtfld_; }

    Notifier<uiMathExpression>	formSet;

protected:

    uiLineEdit*		txtfld_;
    uiComboBox*		grpfld_;
    uiComboBox*		fnfld_;

    void		grpSel(CallBacker*);
    void		doIns(CallBacker*);
    void		retPressCB(CallBacker*);

};


#endif
