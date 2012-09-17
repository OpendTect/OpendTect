#ifndef uidate_h
#define uidate_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2011
 RCS:           $Id: uidate.h,v 1.1 2011/12/22 20:16:47 cvskris Exp $
________________________________________________________________________

-*/

#include "uiobj.h"
#include "uigroup.h"
#include "dateinfo.h"

class uiCalendarBody;
class uiComboBox;
class uiLineEdit;
class uiLabel;
class uiPushButton;

/* Displays a calendar where the uses can select a date. */
mClass uiCalendar : public uiObject
{
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
mClass uiDateSel : public uiGroup
{
public:
			uiDateSel(uiParent*,const char* label,
				  const DateInfo* = 0 );

    void		setDate(const DateInfo&);
    bool		getDate(DateInfo&,bool doui) const;

protected:
    void		showCalendarCB(CallBacker*);

    uiLabel*		label_;
    uiComboBox*		dayfld_;
    uiComboBox*		monthfld_;
    uiLineEdit*		yearfld_;
    uiPushButton*	showcalendarbut_;
};



#endif
