#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2011
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"
#include "uigroup.h"
#include "dateinfo.h"

class uiCalendarBody;
class uiComboBox;
class uiLineEdit;
class uiLabel;
class uiPushButton;

/* Displays a calendar where the uses can select a date. */
mExpClass(uiBase) uiCalendar : public uiObject
{ mODTextTranslationClass(uiCalendar);
public:
                        uiCalendar(uiParent*);

    void		setDate(const DateInfo&);
    DateInfo		getDate() const;

private:

    uiCalendarBody*	body_;
    uiCalendarBody&	mkbody(uiParent*);

};


/*! A field where the user either can enter a date, or select a date
    in a popup-calendar. */
mExpClass(uiBase) uiDateSel : public uiGroup
{ mODTextTranslationClass(uiDateSel);
public:
			uiDateSel(uiParent*,const uiString& label,
				  const DateInfo* = 0 );
			~uiDateSel();

    void		setDate(const DateInfo&);
    bool		getDate(DateInfo&,bool doui) const;

    Notifier<uiDateSel>	changed;

protected:
    void		showCalendarCB(CallBacker*);
    void		changeCB(CallBacker*);

    uiLabel*		label_;
    uiComboBox*		dayfld_;
    uiComboBox*		monthfld_;
    uiLineEdit*		yearfld_;
    uiPushButton*	showcalendarbut_;
};
