/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uibutton.cc,v 1.1 2000-11-27 10:20:34 bert Exp $
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


uiPushButton::uiPushButton( uiObject* parnt, const char* nm )
: uiMltplWrapObj<uiButton,i_PushButton>( new i_PushButton(*this,parnt,nm),
					parnt, nm, add2LM(parnt) ) 
{
   mQtThing()->setText( QString( nm ) );
}

uiPushButton::~uiPushButton() 			{}    
const QWidget* 	uiPushButton::qWidget_() const 	{ return mQtThing(); } 
QButton& 	uiPushButton::qButton() 	{ return *mQtThing(); } 


uiRadioButton::uiRadioButton( uiObject* parnt, const char* nm )
: uiMltplWrapObj<uiButton,i_RadioButton>( new i_RadioButton(*this,parnt,nm),
					parnt, nm, add2LM(parnt) ) 
{
   mQtThing()->setText( QString( nm ) );
}

uiRadioButton::~uiRadioButton() 		{}    
const QWidget* 	uiRadioButton::qWidget_() const 	{ return mQtThing(); } 
QButton& 	uiRadioButton::qButton() 	{ return *mQtThing(); } 

bool uiRadioButton::isChecked() const { return mQtThing()->isChecked (); }

void 		uiRadioButton::setChecked( bool check ) 
					{ return mQtThing()->setChecked( check );}


uiCheckBox::uiCheckBox( uiObject* parnt, const char* nm )
: uiMltplWrapObj<uiButton,i_CheckBox>( new i_CheckBox(*this,parnt,nm),
					parnt, nm, add2LM(parnt) ) 
{
   mQtThing()->setText( QString( nm ) );
}

uiCheckBox::~uiCheckBox() 		{}    
const QWidget* 	uiCheckBox::qWidget_() const 	{ return mQtThing(); } 
QButton& 	uiCheckBox::qButton() 	{ return *mQtThing(); } 

bool 		uiCheckBox::isChecked () const { return mQtThing()->isChecked (); }
void 		uiCheckBox::setChecked ( bool check ) 
					{ return mQtThing()->setChecked( check );}


uiToolButton::uiToolButton( uiObject* parnt, const char* nm )
: uiMltplWrapObj<uiButton,i_ToolButton>( new i_ToolButton(*this,parnt,nm),
					parnt, nm, add2LM(parnt) )
{
   mQtThing()->setText( QString( nm ) );
}

uiToolButton::~uiToolButton() 		{}    
const QWidget* 	uiToolButton::qWidget_() const 	{ return mQtThing(); } 
QButton& 	uiToolButton::qButton()	{ return *mQtThing(); } 

uiButtonGroup::uiButtonGroup( uiObject* parnt, const char* nm
			     , bool vertical, int strips )
: uiWrapObj<i_QButtonGroup>( new i_QButtonGroup( *this, nm , parnt , strips, !vertical ), 
         parnt, nm, true )
{}

uiButtonGroup::~uiButtonGroup() {} // don't delete mQtThing, since Qt deletes it
                                   // when deleting the layout manager

QButtonGroup& 	uiButtonGroup::qButtonGroup() { return *mQtThing(); }
const QWidget* 	uiButtonGroup::qWidget_() const 	{ return mQtThing(); } 

