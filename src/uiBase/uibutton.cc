/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uibutton.cc,v 1.4 2001-05-04 10:08:58 windev Exp $
________________________________________________________________________

-*/

#include <uibutton.h>
#include <uiobj.h>
#include <i_qobjwrap.h>
#include <i_qbutton.h>

#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qtoolbutton.h>

#include <qbuttongroup.h> 
#include <qvbuttongroup.h> 
#include <qhbuttongroup.h> 


uiButton::uiButton( uiObject* parnt, const char* nm )
: uiObject( parnt, nm )
{
}


bool uiButton::add2LM( uiObject* parnt ) const
{
    return dynamic_cast<uiButtonGroup*>(parnt) ? false : true;
}


void uiButton::setText( const char* txt )
{
    qButton().setText( QString( txt ) );
}

const char* uiButton::text()
{
    return qButton().text();
}

#define mButtonCommon( class, i_class )\
class::class( uiObject* parnt, const char* nm )\
: uiMltplWrapObj<uiButton,i_class>( new i_class(*this,parnt,nm),\
					parnt, nm, add2LM(parnt) ) \
    { mQtThing()->setText( QString( nm ) ); }\
class::~class() 			{}\
const QWidget* 	class::qWidget_() const	{ return mQtThing(); } \
QButton& 	class::qButton() 	{ return *mQtThing(); } 


mButtonCommon( uiPushButton,	i_PushButton )
mButtonCommon( uiRadioButton,	i_RadioButton )
mButtonCommon( uiCheckBox,	i_CheckBox )
mButtonCommon( uiToolButton,	i_ToolButton )

void uiPushButton::setDefault( bool yn)
    { mQtThing()->setDefault( yn ); }

bool uiRadioButton::isChecked() const 
    { return mQtThing()->isChecked (); }

void uiRadioButton::setChecked( bool check ) 
    { mQtThing()->setChecked( check );}


bool uiCheckBox::isChecked () const 
    { return mQtThing()->isChecked (); }

void uiCheckBox::setChecked ( bool check ) 
    { mQtThing()->setChecked( check ); }



uiButtonGroup::uiButtonGroup( uiObject* parnt, const char* nm
			     , bool vertical, int strips )
: uiWrapObj<i_QButtonGroup>( new i_QButtonGroup( *this, nm , parnt , strips, !vertical ), 
         parnt, nm, true )
{}

uiButtonGroup::~uiButtonGroup() {} // don't delete mQtThing, since Qt deletes it
                                   // when deleting the layout manager

QButtonGroup& 	uiButtonGroup::qButtonGroup()	{ return *mQtThing(); }
const QWidget* 	uiButtonGroup::qWidget_() const	{ return mQtThing(); } 

