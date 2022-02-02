/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          17/1/2001
________________________________________________________________________

-*/


#include "uidate.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uidialog.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uimsg.h"
#include "uiobjbodyimpl.h"
#include "uispinbox.h"
#include "uistrings.h"

#include <QCalendarWidget>

mUseQtnamespace

class uiCalendarBody : public uiObjBodyImpl<uiCalendar,QCalendarWidget>
{
public:

uiCalendarBody( uiCalendar& hndl, uiParent* p)
    : uiObjBodyImpl<uiCalendar,QCalendarWidget>(hndl,p,0)
{}

};


uiCalendar::uiCalendar( uiParent* p )
    : uiObject(p,0,mkbody(p) )
{}


uiCalendar::~uiCalendar()
{}


uiCalendarBody& uiCalendar::mkbody( uiParent* p )
{
    body_= new uiCalendarBody(*this,p );
    return *body_;
}


void uiCalendar::setDate( const DateInfo& di )
{
    body_->setSelectedDate( QDate(di.year(),di.usrMonth(),di.day()) );
}


DateInfo uiCalendar::getDate() const
{
    const QDate qdate = body_->selectedDate();
    return DateInfo( qdate.year(), qdate.month(), qdate.day() );
}


void uiCalendar::setMinimumDate( const DateInfo& di )
{
    body_->setMinimumDate( QDate(di.year(),di.usrMonth(),di.day()) );
}


void uiCalendar::setMaximumDate( const DateInfo& di )
{
    body_->setMaximumDate( QDate(di.year(),di.usrMonth(),di.day()) );
}



// uiDateSel
uiDateSel::uiDateSel( uiParent* p, const uiString& label, const DateInfo* di )
    : uiGroup( p )
    , changed(this)
    , label_( !label.isEmpty() ? new uiLabel( this, label ) : nullptr )
{
    dayfld_ = new uiSpinBox( this );
    dayfld_->setInterval( 1, 31 );
    dayfld_->setHSzPol( uiObject::SmallVar );
    mAttachCB( dayfld_->valueChanging, uiDateSel::dayChgCB );
    if ( label_ ) label_->attach( leftOf, dayfld_ );

    monthfld_ = new uiComboBox( this, DateInfo::sFullMonths(), 0 );
    monthfld_->setHSzPol( uiObject::SmallVar );
    monthfld_->attach( rightOf, dayfld_ );
    mAttachCB( monthfld_->selectionChanged, uiDateSel::monthChgCB );

    yearfld_ = new uiSpinBox( this );
    yearfld_->setInterval( 1900, 2099 );
    yearfld_->setHSzPol( uiObject::SmallVar );
    yearfld_->attach( rightOf, monthfld_ );
    mAttachCB( yearfld_->valueChanging, uiDateSel::yearChgCB );

    showcalendarbut_ = new uiPushButton( this, uiStrings::sSelect(),
	    mCB(this,uiDateSel,showCalendarCB), false );
    showcalendarbut_->attach( rightOf, yearfld_ );

    setHAlignObj( dayfld_ );

    if ( di )
	setDate( *di );
    else
	setDate( DateInfo() );
}


uiDateSel::~uiDateSel()
{
    detachAllNotifiers();
    delete mindate_;
    delete maxdate_;
}


void uiDateSel::dayChgCB( CallBacker* )
{
    changed.trigger();
}


void uiDateSel::monthChgCB( CallBacker* )
{
    updateNrDays();
    changed.trigger();
}


void uiDateSel::yearChgCB( CallBacker* )
{
    updateNrDays();
    changed.trigger();
}


void uiDateSel::updateNrDays()
{
    QDate qdate( getYear(), getMonth(), 1 );
    const int nrdays = qdate.daysInMonth();
    dayfld_->setMaxValue( nrdays );
}


bool uiDateSel::getDate( DateInfo& di, bool ui ) const
{
    const int year = getYear();
    if ( mIsUdf(year) || year<1900 )
    {
	if ( ui ) uiMSG().error(tr("Year is undefined or less than 1900"));
	return false;
    }

    DateInfo res( year, getMonth(), getDay() );
    if ( res.isUdf() )
    {
	if ( ui ) uiMSG().error(tr("Invalid date"));
	return false;
    }

    di = res;
    return true;
}


void uiDateSel::setDate( const DateInfo& di )
{
    if ( di.isUdf() )
	return;

    yearfld_->setValue( di.year() );
    monthfld_->setValue( di.usrMonth()-1 );
    dayfld_->setValue( di.day() );
    updateNrDays();
}


void uiDateSel::setMinimumDate( const DateInfo& di )
{
    if ( !mindate_ )
	mindate_ = new DateInfo;

    *mindate_ = di;
}


void uiDateSel::setMaximumDate( const DateInfo& di )
{
    if ( !maxdate_ )
	maxdate_ = new DateInfo;

    *maxdate_ = di;
}


int uiDateSel::getDay() const
{
    return dayfld_->getIntValue();
}

int uiDateSel::getMonth() const
{
    return monthfld_->getIntValue() + 1;
}


int uiDateSel::getYear() const
{
    return yearfld_->getIntValue();
}


void uiDateSel::showCalendarCB( CallBacker* )
{
    uiDialog dlg( this, uiDialog::Setup( tr("Calendar"),
					 uiStrings::phrSelect( tr("date") ),
					 mNoHelpKey ) );
    uiCalendar* cal = new uiCalendar( &dlg );
    if ( mindate_ )
	cal->setMinimumDate( *mindate_ );
    if ( maxdate_ )
	cal->setMaximumDate( *maxdate_ );

    DateInfo di;
    if ( getDate(di,false) )
	cal->setDate( di );

    if ( dlg.go() )
	setDate( cal->getDate() );
}
