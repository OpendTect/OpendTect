/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          17/1/2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uidate.cc,v 1.1 2011-12-22 20:16:47 cvskris Exp $";


#include "uidate.h"

#include "uiobjbody.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uicombobox.h"
#include "uilineedit.h"
#include "uibutton.h"
#include "uidialog.h"

#include <QCalendarWidget>


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


uiDateSel::uiDateSel( uiParent* p,const char* label, const DateInfo* di )
    : uiGroup( p )
    , label_( label_ ? new uiLabel( this, label) : 0 )
{
    dayfld_ = new uiComboBox( this, DateInfo::sAllDaysInMonth(), 0 );
    if ( label_ ) dayfld_->attach( rightOf, label_ );

    monthfld_ = new uiComboBox( this, DateInfo::sFullMonths(), 0 );
    monthfld_->attach( rightOf, dayfld_ );

    yearfld_ = new uiLineEdit( this, 0 );
    yearfld_->setMaxLength( 4 );
    yearfld_->attach( rightOf, monthfld_ );

    showcalendarbut_ = new uiPushButton( this, "Select",
	    mCB(this,uiDateSel,showCalendarCB), true );
    showcalendarbut_->attach( rightOf, yearfld_ );

    setHAlignObj( monthfld_ );

    if ( di )
	setDate( *di );
}


bool uiDateSel::getDate( DateInfo& di, bool ui ) const
{
    int year = yearfld_->getIntValue();
    if ( mIsUdf(year) || year<1900 )
    {
	if ( ui ) uiMSG().error("Year is undefined or less than 1900");
	return false;
    }

    DateInfo res( year, monthfld_->getIntValue()+1, dayfld_->getIntValue()+1 );
    if ( res.isUdf() )
    {
	if ( ui ) uiMSG().error("Invalid date");
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
    uiDialog dlg( this, uiDialog::Setup( 0, 0, mNoHelpID ) );
    uiCalendar* cal = new uiCalendar( &dlg );
    DateInfo di;
    if ( getDate(di,false) )
	cal->setDate( di );

    if ( dlg.go() )
    {
	setDate( cal->getDate() );
    }
}


