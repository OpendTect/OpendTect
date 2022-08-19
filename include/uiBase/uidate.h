#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
class uiSpinBox;

/* Displays a calendar where the uses can select a date. */
mExpClass(uiBase) uiCalendar : public uiObject
{ mODTextTranslationClass(uiCalendar);
public:
			uiCalendar(uiParent*);
			~uiCalendar();

    void		setDate(const DateInfo&);
    DateInfo		getDate() const;
    void		setMinimumDate(const DateInfo&);
    void		setMaximumDate(const DateInfo&);

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
				  const DateInfo* = nullptr );
			~uiDateSel();

    void		setDate(const DateInfo&);
    bool		getDate(DateInfo&,bool doui) const;
    void		setMinimumDate(const DateInfo&);
    void		setMaximumDate(const DateInfo&);

    int			getDay() const;
    int			getMonth() const;
    int			getYear() const;

    Notifier<uiDateSel>	changed;

protected:
    void		showCalendarCB(CallBacker*);

    void		dayChgCB(CallBacker*);
    void		monthChgCB(CallBacker*);
    void		yearChgCB(CallBacker*);
    void		updateNrDays();

    uiLabel*		label_;
    uiSpinBox*		dayfld_;
    uiComboBox*		monthfld_;
    uiSpinBox*		yearfld_;
    uiPushButton*	showcalendarbut_;

    DateInfo*		mindate_	= nullptr;
    DateInfo*		maxdate_	= nullptr;
};
