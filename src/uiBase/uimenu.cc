/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          26/04/2000
 RCS:           $Id: uimenu.cc,v 1.17 2003-08-11 11:21:50 nanne Exp $
________________________________________________________________________

-*/

#include "uimenu.h"
#include "i_qmenu.h"
#include "uiparentbody.h"
#include "uidobjset.h"
#include "uiobjbody.h"
#include "uibody.h"
#include "uimainwin.h"

#include <qmenudata.h> 
#include <qmenubar.h>
#include <qpopupmenu.h> 
#include <qcursor.h> 




class uiMenuDataBody : public uiBodyImpl<uiMenuData,QMenuData>
{
public:

			uiMenuDataBody(uiMenuData& handle, 
				       uiParent* parnt,
				       QMenuBar& qThing )
			    : uiBodyImpl<uiMenuData,QMenuData>
				( handle, parnt, qThing )
			    , bar_( &qThing )
			    , popup_( 0 )	{}

			uiMenuDataBody(uiMenuData& handle, 
				       uiParent* parnt,
				       QPopupMenu& qThing )
			    : uiBodyImpl<uiMenuData,QMenuData>
				( handle, parnt, qThing )	
			    , popup_( &qThing )
			    , bar_( 0 )	{}

			~uiMenuDataBody()		{ deepErase( itms ); }

    int			nrItems() const 		{ return itms.size(); }

    int			insertItem( uiMenuItem* it, int id, int idx )
			{
			    QString nm( it->name() );
			    i_MenuMessenger* msgr__= it->messenger();

			    int newid = qthing()->insertItem( nm, msgr__,
				    			      SLOT(activated()),
							      0, id, idx );

			    it->setId( newid );
			    it->setMenu( this );
			    itms += it;

			    return newid;
			}

    int			insertItem( uiPopupMenu* pmnu, int id, int idx )
			{
			    uiPopupItem* it = &pmnu->item();

			    QString nm( it->name() );
			    QPopupMenu* pu = pmnu->body_->popup();

			    int newid = qthing()->insertItem( nm, pu, id, idx );

			    it->setId( newid );
			    it->setMenu( this );
			    itms += it;

			    return newid;
			}

    int			insertItem( const char* text, const CallBack& cb, 
	    			    int id, int idx )
			{ 
			    uiMenuItem* it = new uiMenuItem( text, cb );

			    int newid = insertItem( it, id, idx );
			    it->setId( newid );
			    it->setMenu( this );
			    itms += it;

			    return newid;
			} 


    bool			isCheckable();
    void			setCheckable( bool yn );

    QMenuBar*			bar()			{ return bar_; }
    QPopupMenu*			popup()			{ return popup_; }

    virtual const QWidget*	managewidg_() const 	{ return qwidget(); }

    void			setIcon( const QPixmap& pm )
				{
    			    	    if ( bar_ ) bar_->setIcon( pm );
    			    	    if ( popup_ ) popup_->setIcon( pm );
				}

    void			setSensitive( bool yn )
				{
				    if ( bar_ ) bar_->setEnabled( yn );
				}

private:

    ObjectSet<uiMenuItem>	itms;

    QMenuBar*			bar_;
    QPopupMenu*			popup_;

};


//-----------------------------------------------------------------------

uiMenuItem::uiMenuItem( const char* nm )
    : UserIDObject( nm )
    , activated( this )
    , messenger_( *new i_MenuMessenger( this ) ) 
    , id_( 0 )
    , menu_( 0 )
{}

uiMenuItem::uiMenuItem( const char* nm, const CallBack& cb)
    : UserIDObject( nm )
    , activated( this )
    , messenger_( *new i_MenuMessenger( this ) )
    , id_(-1)
    , menu_( 0 )
{ 
    activated.notify( cb ); 
}


uiMenuItem::~uiMenuItem()
{ 
    delete &messenger_; 
}


bool uiMenuItem::isEnabled () const
    { return menu_->qthing()->isItemEnabled( id_ ); }

void uiMenuItem::setEnabled ( bool yn )
    { menu_->qthing()->setItemEnabled( id_, yn ); }

bool uiMenuItem::isChecked () const
    { return menu_->qthing()->isItemChecked( id_ ); }

void uiMenuItem::setChecked( bool yn )
    { menu_->qthing()->setItemChecked( id_, yn ); }

void uiMenuItem::setText( const char* txt )
    { menu_->qthing()->changeItem ( id_, QString(txt) ); }

int uiMenuItem::index() const
    { return menu_->qthing()->indexOf( id_ ); }


//-----------------------------------------------------------------------



uiMenuData::uiMenuData( const char* nm, uiMenuDataBody* b )
    : uiObjHandle( nm, b )
    , body_( b )				{}


uiMenuData::~uiMenuData()			{ delete body_; }


int uiMenuData::nrItems() const
    { return body_->nrItems(); }


int uiMenuData::insertItem( uiMenuItem* it, int id, int idx )
    { return body_->insertItem( it, id, idx ); }

/*!
    \brief Add a menu item by menu-text and CallBack.

    If you want to be able to specify a callback flexibly, construct
    a uiMenuItem by hand and insert this into the menu. Then you can
    add callbacks at any time to the uiMenuItem.

*/
int uiMenuData::insertItem( const char* text, const CallBack& cb, int id, 
			    int idx )
    { return body_->insertItem( text, cb, id, idx ); }

int uiMenuData::insertItem( uiPopupMenu* it, int id, int idx )
    { return body_->insertItem( it, id, idx ); }

void uiMenuData::insertSeparator( int idx ) 
    { body_->qthing()->insertSeparator( idx ); }


void uiMenuData::setMenuBody(uiMenuDataBody* b)  
{ 
    body_=b;
    setBody( b );
}

int uiMenuData::idAt( int idx ) const
    { return body_->qthing()->idAt(idx); }

int uiMenuData::indexOf( int id ) const
    { return body_->qthing()->indexOf(id); }

// ------------------------------------------------------------------------


uiMenuBar::uiMenuBar( uiParent* parnt, const char* nm )
    : uiMenuData( nm, 0 )
{ 
    setMenuBody( new uiMenuDataBody( *this, parnt,
			*new QMenuBar(parnt->body()->qwidget(),nm ) ));
}

uiMenuBar::uiMenuBar( uiMainWin* parnt, const char* nm, QMenuBar& qThing )
    : uiMenuData( nm, 0 )
{ 
    setMenuBody( new uiMenuDataBody( *this, parnt, qThing ) ); 
}

void uiMenuBar::reDraw( bool deep )
{
    if ( body_->bar() ) 
	body_->bar()->update();
}

void uiMenuBar::setIcon( const QPixmap& pm )
{
    body_->setIcon( pm );
}


void uiMenuBar::setSensitive( bool yn )
{
    body_->setSensitive( yn );
}


// -----------------------------------------------------------------------

uiPopupItem::uiPopupItem( uiPopupMenu& menu, const char* nm )
    : uiMenuItem( nm )
{}


bool uiPopupItem::isCheckable() const
{
    if( !menu_->popup() ) { pErrMsg("Huh?"); return false; }
    return menu_->popup()->isCheckable(); 
}


void uiPopupItem::setCheckable( bool yn )
{
    if( !menu_->popup() ) { pErrMsg("Huh?"); return; }
    menu_->popup()->setCheckable(yn); 
}


// -----------------------------------------------------------------------

uiPopupMenu::uiPopupMenu( uiParent* parnt, const char* nm )
    : uiMenuData( nm, 0 )
    , item_( *new uiPopupItem( *this, nm ) )
{
    setMenuBody ( new uiMenuDataBody( *this, parnt, 
                              *new QPopupMenu(parnt->body()->qwidget(),nm ) ));
    item_.setMenu( body_ );
}


uiPopupMenu::~uiPopupMenu() 
{ 
    delete &item_; 
}


bool uiPopupMenu::isCheckable() const
{ return item().isCheckable(); }

void uiPopupMenu::setCheckable( bool yn ) 
{ item().setCheckable( yn ); }

bool uiPopupMenu::isChecked() const
{ return item().isChecked(); }

void uiPopupMenu::setChecked( bool yn )
{ item().setChecked( yn ); }

bool uiPopupMenu::isEnabled() const
{ return item().isEnabled(); }

void uiPopupMenu::setEnabled( bool yn )
{ item().setEnabled( yn ); }


int uiPopupMenu::exec()
{
    if ( !body_->popup() ) return -1;

    return body_->popup()->exec(QCursor::pos());
}

