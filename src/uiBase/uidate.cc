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


uiCalendarBody& uiCalendar::mkbody( uiParent* p )
{
    body_= new uiCalendarBody(*this,p );
    return *body_;
}

void uiCalendar::setDate( const DateInfo& di )
{
    body_->setSelectedDate(QDate( di.year(), di.usrMonth(), di.day() ) );
}

DateInfo uiCalendar::getDate() const
{
    const QDate qdate = body_->selectedDate();
    return DateInfo( qdate.year(), qdate.month(), qdate.day() );
}


uiDateSel::uiDateSel( uiParent* p, const uiString& label, const DateInfo* di )
    : uiGroup( p )
    , changed(this)
    , label_( !label.isEmpty() ? new uiLabel( this, label ) : nullptr )
{
    dayfld_ = new uiComboBox( this,
		    BufferStringSet(DateInfo::sAllDaysInMonth()), 0 );
    mAttachCB( dayfld_->selectionChanged, uiDateSel::changeCB );
    dayfld_->setHSzPol( uiObject::SmallVar );
    if ( label_ ) label_->attach( leftOf, dayfld_ );

    monthfld_ = new uiComboBox( this, DateInfo::MonthDef(), 0 );
    mAttachCB( monthfld_->selectionChanged, uiDateSel::changeCB );
    monthfld_->setHSzPol( uiObject::SmallVar );
    monthfld_->attach( rightOf, dayfld_ );

    yearfld_ = new uiLineEdit( this, 0 );
    mAttachCB( yearfld_->editingFinished, uiDateSel::changeCB );
    yearfld_->setHSzPol( uiObject::SmallVar );
    yearfld_->setMaxLength( 4 );
    yearfld_->attach( rightOf, monthfld_ );

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
}


void uiDateSel::changeCB( CallBacker* )
{
    changed.trigger();
}


bool uiDateSel::getDate( DateInfo& di, bool ui ) const
{
    int year = yearfld_->getIntValue();
    if ( mIsUdf(year) || year<1900 )
    {
	if ( ui ) uiMSG().error(tr("Year is undefined or less than 1900"));
	return false;
    }

    DateInfo res( year, monthfld_->getIntValue()+1, dayfld_->getIntValue()+1 );
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
    dayfld_->setValue( di.day()-1 );
}

void uiDateSel::showCalendarCB( CallBacker* )
{
    uiDialog dlg( this, uiDialog::Setup( tr("Calendar"),
			 uiStrings::phrSelect( uiStrings::sDate().toLower() ),
					 mNoHelpKey ) );
    uiCalendar* cal = new uiCalendar( &dlg );
    DateInfo di;
    if ( getDate(di,false) )
	cal->setDate( di );

    if ( dlg.go() )
    {
	setDate( cal->getDate() );
    }
}
