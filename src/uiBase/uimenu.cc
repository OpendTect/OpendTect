/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          26/04/2000
 RCS:           $Id: uimenu.cc,v 1.1 2000-11-27 10:20:35 bert Exp $
________________________________________________________________________

-*/

#include <uimenu.h>
#include <i_qobjwrap.h>
#include <i_qmenu.h>
#include <uidobjset.h> 

#include <qmenudata.h> 
#include <qmenubar.h>
#include <qpopupmenu.h> 

#include <uimainwin.h>


uiMenuData::uiMenuData()
{}


uiMenuData::~uiMenuData()
{ 
    itms.deepErase(); 
}


void uiMenuData::insertItem( uiMenuItem* it, int index )
{
    it->id = qMenuData().insertItem( QString( it->name() ), 
			 &it->_messenger, SLOT( activated() ), 0, -1, index );
    it->parent=this;
    itms += it;
}

/*!
    \brief Insert a menu item by menu-text and CallBack.

    If you want to be able to specify a callback flexibly, construct
    a uiMenuItem by hand and insert this into the menu. Then you can
    add callbacks at any time to the uiMenuItem.

*/
void uiMenuData::insertItem( const char* text, const CallBack& cb, int index )
{ 
    uiMenuItem* it = new uiMenuItem( text, cb );
    insertItem ( it, index ); 
    itms += it;
} 

void uiMenuData::insertItem( uiPopupMenu* it, int index )
{
    it->id = qMenuData().insertItem( QString( it->name() ), it->mQtThing(),
				     -1, index);
//    it->parent=this;
    itms += it;
}

void uiMenuData:: insertSeparator( int index ) 
{ qMenuData().insertSeparator( index ); }

uiMenuItem::uiMenuItem( const char* nm ) 
    : UserIDObject( nm )
    , _messenger( *new i_MenuMessenger( this ) )
    , id( 0 ) 
{}

uiMenuItem::uiMenuItem( const char* nm, const CallBack& cb )
    : UserIDObject( nm )
    , _messenger( *new i_MenuMessenger( this ) )
    , id( 0 ) 
{ 
    notifyCBL += cb; 
}

uiMenuItem::~uiMenuItem() { delete &_messenger; }


bool uiMenuItem::isEnabled () const
{ return parent->qMenuData().isItemEnabled( id ); }


void uiMenuItem::setEnabled ( bool yn )
{ parent->qMenuData().setItemEnabled( id, yn ); }


bool uiMenuItem::isChecked () const
{ return parent->qMenuData().isItemChecked( id ); }


void uiMenuItem::setChecked( bool yn )
{ parent->qMenuData().setItemChecked( id, yn ); }

void uiMenuItem::set( const char* txt )
{
    parent->qMenuData().changeItem ( id, QString(txt) );
}

uiMenuBar::uiMenuBar( uiMainWin* parnt, const char* nm, QMenuBar& qThing )
: uiNoWrapObj<QMenuBar>( &qThing, parnt, nm, false ) 
{}

const QWidget* 	uiMenuBar::qWidget_() const 	{ return mQtThing(); } 

QMenuData& uiMenuBar::qMenuData() { return *mQtThing(); }


uiPopupMenu::uiPopupMenu( uiObject* parnt, const char* nm )
: uiWrapObj<i_QPopupMenu>( new i_QPopupMenu( *(uiObject*)this, 
			    parnt ? &parnt->clientQWidget() : 0, nm ), 
	 parnt, nm ) 
{}

const QWidget* 	uiPopupMenu::qWidget_() const 	{ return mQtThing(); } 

QMenuData& uiPopupMenu::qMenuData() 
    { return *mQtThing(); }

bool uiPopupMenu::isCheckable() 
    { return mQtThing()->isCheckable(); }

void uiPopupMenu::setCheckable(bool yn) 
    { return mQtThing()->setCheckable(yn); }
