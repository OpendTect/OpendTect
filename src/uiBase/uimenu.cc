/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          26/04/2000
 RCS:           $Id: uimenu.cc,v 1.31 2006-08-21 17:14:45 cvsbert Exp $
________________________________________________________________________

-*/

#include "uimenu.h"
#include "i_qmenu.h"
#include "uiparentbody.h"
#include "uiobjbody.h"
#include "uibody.h"

#include <qmenudata.h>
#include <qmenubar.h>
#include <qcursor.h>
#ifdef USEQT4
# include <qmenu.h>
# include <q3popupmenu.h>
# define mQThing	qmenu_
#else
# include <qpopupmenu.h>
# define mQThing	qthing()
#endif

#ifdef USEQT4


class uiMenuItemContainerBody
{
public:

			uiMenuItemContainerBody()	{}
    virtual		~uiMenuItemContainerBody()	{ deepErase( itms ); }

    int			nrItems() const 		{ return itms.size(); }
    const ObjectSet<uiMenuItem>& items() const		{ return itms; }

    virtual int		insertItem( uiMenuItem* it, int id, int idx ) =0;
    virtual int		insertItem( uiPopupMenu* pmnu, int id, int idx ) =0;
    virtual int		insertItem( const char* text, const CallBack& cb, 
	    			    int id, int idx ) =0;

    virtual QMenuBar*		bar()			{ return 0; }
    virtual mQPopupMenu*	popup()			{ return 0; }

    void			setIcon( const QPixmap& pm )
				{
    			    	    if ( bar() )	bar()->setIcon( pm );
    			    	    if ( popup() )	bar()->setIcon( pm );
				}

    void			setSensitive( bool yn )
				{
				    if ( bar() ) bar()->setEnabled( yn );
				}
protected:

    ObjectSet<uiMenuItem>	itms;

};

     


template <class T>
class uiMenuItemContainerBodyImpl : public uiMenuItemContainerBody
			 , public uiBodyImpl<uiMenuItemContainer,T>
{
public:
			uiMenuItemContainerBodyImpl(uiMenuItemContainer& handle, 
				       uiParent* parnt,
				       T& qThing )
			    : uiBodyImpl<uiMenuItemContainer,T>
				( handle, parnt, qThing )
			    , qmenu_( &qThing ) {}

			~uiMenuItemContainerBodyImpl()		{}


#else

class uiMenuItemContainerBody : public uiBodyImpl<uiMenuItemContainer,QMenuData>
{
public:

			uiMenuItemContainerBody(uiMenuItemContainer& handle, 
						uiParent* parnt,
						QMenuBar& qThing )
			    : uiBodyImpl<uiMenuItemContainer,QMenuData>
				( handle, parnt, qThing )
			    , bar_( &qThing )
			    , popup_( 0 )	{}

			uiMenuItemContainerBody(uiMenuItemContainer& handle, 
						uiParent* parnt,
						QPopupMenu& qThing )
			    : uiBodyImpl<uiMenuItemContainer,QMenuData>
				( handle, parnt, qThing )	
			    , popup_( &qThing )
			    , bar_( 0 )	{}

			~uiMenuItemContainerBody()	{ deepErase( itms ); }

int			nrItems() const 		{ return itms.size(); }
const ObjectSet<uiMenuItem>& items() const		{ return itms; }

#endif
			

    int			insertItem( uiMenuItem* it, int id, int idx )
			{
			    QString nm( it->name() );
			    i_MenuMessenger* msgr__= it->messenger();

			    int newid = mQThing->insertItem( nm, msgr__,
				    			      SLOT(activated()),
							      0, id, idx );

			    it->setId( newid );
			    it->setMenu( this );
			    mQThing->setItemChecked( newid, it->isChecked() );
			    mQThing->setItemEnabled( newid, it->isEnabled() );
			    itms += it;

			    return newid;
			}

    int			insertItem( uiPopupMenu* pmnu, int id, int idx )
			{
			    uiPopupItem* it = &pmnu->item();

			    QString nm( it->name() );
			    mQPopupMenu* pu = pmnu->body_->popup();

			    int newid = mQThing->insertItem( nm, pu, id, idx );

			    it->setId( newid );
			    it->setMenu( this );
			    mQThing->setItemChecked( newid, it->isChecked() );
			    mQThing->setItemEnabled( newid, it->isEnabled() );
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
			    mQThing->setItemChecked( newid, it->isChecked() );
			    mQThing->setItemEnabled( newid, it->isEnabled() );
			    itms += it;

			    return newid;
			} 
#ifdef USEQT4

    QMenuBar*		bar() { return reinterpret_cast<QMenuBar*>(qmenu_); }
    mQPopupMenu*	popup(){ return reinterpret_cast<mQPopupMenu*>(qmenu_);}
    virtual const QWidget* managewidg_() const 	{ return qmenu_; }

private:

    T*				qmenu_;

#else

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

#endif
};


//-----------------------------------------------------------------------

uiMenuItem::uiMenuItem( const char* nm )
    : NamedObject(nm)
    , activated(this)
    , messenger_( *new i_MenuMessenger(this) ) 
    , id_(-1)
    , menu_(0)
    , enabled_(true)
    , checked_(false)
{}


uiMenuItem::uiMenuItem( const char* nm, const CallBack& cb )
    : NamedObject(nm )
    , activated(this)
    , messenger_( *new i_MenuMessenger(this) )
    , id_(-1)
    , menu_(0)
    , enabled_(true)
    , checked_(false)
{ 
    activated.notify( cb ); 
}


uiMenuItem::~uiMenuItem()
{ 
    delete &messenger_; 
}


bool uiMenuItem::isEnabled () const
{
#ifdef USEQT4
    if ( !menu_ )		return enabled_;
    if ( menu_->bar() )		return menu_->bar()->isItemEnabled( id_ );
    if ( menu_->popup() )	return menu_->popup()->isItemEnabled( id_ );

    return enabled_;
#else
    return menu_ && menu_->qthing() ? menu_->qthing()->isItemEnabled( id_ )
				    : enabled_;
#endif
}

void uiMenuItem::setEnabled ( bool yn )
{
    enabled_ = yn;

#ifdef USEQT4
    if ( !menu_ ) return;
    if ( menu_->bar() )		menu_->bar()->setEnabled( yn );
    else if ( menu_->popup() )	menu_->popup()->setEnabled( yn );
#else
    if ( menu_ && menu_->qthing() )
	menu_->qthing()->setItemEnabled( id_, yn ); 
#endif
}


bool uiMenuItem::isChecked () const
{
#ifdef USEQT4
    if ( !menu_ )		return checked_;
    if ( menu_->bar() )		return menu_->bar()->isItemChecked( id_ );
    if ( menu_->popup() )	return menu_->popup()->isItemChecked( id_ );

    return checked_;
#else
    return menu_ && menu_->qthing() ? menu_->qthing()->isItemChecked( id_ )
				    : checked_; 
#endif
}


void uiMenuItem::setChecked( bool yn )
{
    checked_ = yn;

#ifdef USEQT4
    if ( !menu_ ) return;
    if ( menu_->bar() )		menu_->bar()->setItemChecked( id_, yn );
    else if ( menu_->popup() )	menu_->popup()->setItemChecked( id_, yn );
#else
    if ( menu_ && menu_->qthing() )
	menu_->qthing()->setItemChecked( id_, yn ); 
#endif
}


void uiMenuItem::setText( const char* txt )
{
#ifdef USEQT4
    if ( !menu_ ) return;
    if ( menu_->bar() )		menu_->bar()->changeItem( id_, QString(txt) );
    else if ( menu_->popup() )	menu_->popup()->changeItem( id_, QString(txt) );
#else
    if ( menu_ && menu_->qthing() )
	menu_->qthing()->changeItem ( id_, QString(txt) ); 
#endif
}


int uiMenuItem::index() const
{
#ifdef USEQT4
    if ( !menu_ )		return -1;
    if ( menu_->bar() )		return menu_->bar()->indexOf( id_ );
    if ( menu_->popup() )	return menu_->popup()->indexOf( id_ );

    return -1;
#else
    return menu_ && menu_->qthing() ? menu_->qthing()->indexOf( id_ ) : -1;
#endif
}


//-----------------------------------------------------------------------


#ifdef USEQT4
uiMenuItemContainer::uiMenuItemContainer( const char* nm, uiBody* b,
					  uiMenuItemContainerBody* db )
    : uiObjHandle( nm, b )
    , body_( db )				{}
#else
uiMenuItemContainer::uiMenuItemContainer( const char* nm,
					  uiMenuItemContainerBody* b )
    : uiObjHandle( nm, b )
    , body_( b )				{}
#endif


uiMenuItemContainer::~uiMenuItemContainer()	{ delete body_; }


int uiMenuItemContainer::nrItems() const
    { return body_->nrItems(); }


const ObjectSet<uiMenuItem>& uiMenuItemContainer::items() const
    { return body_->items(); }


uiMenuItem* uiMenuItemContainer::find( const MenuItemSeparString& str )
{
    uiMenuItemContainer* parent = this;
    for ( unsigned int idx=0; idx<str.size(); idx++ )
    {
	if ( !parent ) return 0;

	uiMenuItem* itm = parent->find( str[idx] );
	if ( !itm ) return 0;
	if ( idx == str.size()-1 )
	    return itm;

	mDynamicCastGet(uiPopupItem*,popupitm,itm)
	parent = popupitm ? &popupitm->menu() : 0;
    }

    return 0;
}


uiMenuItem* uiMenuItemContainer::find( const char* itmtxt )
{
    for ( int idx=0; idx<body_->nrItems(); idx++ )
    {
	const uiMenuItem* itm = body_->items()[idx];
	if ( !strcmp(itm->name(),itmtxt) )
	    return const_cast<uiMenuItem*>(itm);
    }

    return 0;
}


int uiMenuItemContainer::insertItem( uiMenuItem* it, int id, int idx )
    { return body_->insertItem( it, id, idx ); }

/*!
    \brief Add a menu item by menu-text and CallBack.

    If you want to be able to specify a callback flexibly, construct
    a uiMenuItem by hand and insert this into the menu. Then you can
    add callbacks at any time to the uiMenuItem.

*/
int uiMenuItemContainer::insertItem( const char* text, const CallBack& cb,
				     int id, int idx )
    { return body_->insertItem( text, cb, id, idx ); }

int uiMenuItemContainer::insertItem( uiPopupMenu* it, int id, int idx )
    { return body_->insertItem( it, id, idx ); }

void uiMenuItemContainer::insertSeparator( int idx ) 
{
#ifdef USEQT4
    if ( body_->bar() ) body_->bar()->insertSeparator( idx );
    else if ( body_->popup() ) body_->popup()->insertSeparator( idx );
#else
    body_->qthing()->insertSeparator( idx ); 
#endif
}

#ifndef USEQT4
void uiMenuItemContainer::setMenuBody(uiMenuItemContainerBody* b)  
{ 
    body_=b;
    setBody( b );
}
#endif


int uiMenuItemContainer::idAt( int idx ) const
{
#ifdef USEQT4
    if ( body_->bar() )		return body_->bar()->idAt(idx);
    if ( body_->popup() )	return body_->popup()->idAt(idx);
    return -1;
#else
    return body_->qthing()->idAt(idx);
#endif
}

int uiMenuItemContainer::indexOf( int id ) const
{
#ifdef USEQT4
    if ( body_->bar() )		return body_->bar()->indexOf(id);
    if ( body_->popup() )	return body_->popup()->indexOf(id);
    return -1;
#else
    return body_->qthing()->indexOf(id);
#endif
}

// ------------------------------------------------------------------------

#ifdef USEQT4
# define mContArgs	nm, 0, 0
#else
# define mContArgs	nm, 0
#endif

uiMenuBar::uiMenuBar( uiParent* parnt, const char* nm )
    : uiMenuItemContainer( mContArgs )
{ 
#ifdef USEQT4
    uiMenuItemContainerBodyImpl<QMenuBar>* bd =
		    new uiMenuItemContainerBodyImpl<QMenuBar>( *this, parnt,
				*new QMenuBar(parnt->body()->qwidget(),nm ) );
    body_ = bd;
    setBody( bd );
#else
    setMenuBody( new uiMenuItemContainerBody( *this, parnt,
			    *new QMenuBar(parnt->body()->qwidget(),nm ) ));
#endif
}


uiMenuBar::uiMenuBar( uiParent* parnt, const char* nm, QMenuBar& qThing )
    : uiMenuItemContainer( mContArgs )
{ 
#ifdef USEQT4
    uiMenuItemContainerBodyImpl<QMenuBar>* bd =
	    new uiMenuItemContainerBodyImpl<QMenuBar>( *this, parnt, qThing );
    body_ = bd;
    setBody( bd );
#else
    setMenuBody( new uiMenuItemContainerBody( *this, parnt, qThing ) ); 
#endif
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

uiPopupItem::uiPopupItem( uiPopupMenu& popmen, const char* nm )
    : uiMenuItem( nm )
    , popmenu_( &popmen )
{}


bool uiPopupItem::isCheckable() const
{
    if ( !menu_->popup() ) { pErrMsg("Huh?"); return false; }
    return menu_->popup()->isCheckable(); 
}


void uiPopupItem::setCheckable( bool yn )
{
    if ( !menu_->popup() ) { pErrMsg("Huh?"); return; }
    menu_->popup()->setCheckable(yn); 
}


// -----------------------------------------------------------------------

uiPopupMenu::uiPopupMenu( uiParent* parnt, const char* nm )
    : uiMenuItemContainer( mContArgs )
    , item_( *new uiPopupItem( *this, nm ) )
{
#ifdef USEQT4
    uiMenuItemContainerBodyImpl<mQPopupMenu>* bd =
		    new uiMenuItemContainerBodyImpl<mQPopupMenu>( *this, parnt, 
			  *new mQPopupMenu(parnt->body()->qwidget(),nm ) );
    body_ = bd;
    setBody( bd );
#else
    setMenuBody ( new uiMenuItemContainerBody( *this, parnt, 
			      *new QPopupMenu(parnt->body()->qwidget(),nm ) ));
#endif

    item_.setMenu( body_ );
}


uiPopupMenu::uiPopupMenu( uiParent* parnt, mQPopupMenu* qmnu, const char* nm )
    : uiMenuItemContainer( mContArgs )
    , item_(*new uiPopupItem(*this,nm))
{
#ifdef USEQT4
    uiMenuItemContainerBodyImpl<mQPopupMenu>* bd =
	    new uiMenuItemContainerBodyImpl<mQPopupMenu>(*this,parnt,*qmnu);

    body_ = bd;
    setBody( bd );
#else
    setMenuBody( new uiMenuItemContainerBody(*this,parnt,*qmnu) );
#endif
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

