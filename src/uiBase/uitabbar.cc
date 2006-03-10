/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          17/01/2002
 RCS:           $Id: uitabbar.cc,v 1.12 2006-03-10 13:34:02 cvsbert Exp $
________________________________________________________________________

-*/

#include "uitabbar.h"
#include "uiobjbody.h"
#include "pixmap.h"
#include "sets.h"

#include "i_qtabbar.h"
#include "qiconset.h"

#ifdef USEQT4
# include "qicon.h"
# define muiObjBodyImpl    uiObjBodyImplNoQtNm
#else
# define muiObjBodyImpl    uiObjBodyImpl
class uiTabBody : public QTab
{
public:
		uiTabBody( uiTab& handle,  const char* nm )
		    : QTab( nm )
		    , handle_(handle) {}

protected:

    uiTab&	handle_;
};
#endif

uiTab::uiTab( uiGroup& grp )
    : UserIDObject( grp.name() )
#ifndef USEQT4
    , body_( *new uiTabBody(*this,grp.name()) )
#endif
    , grp_( grp )
{}

#ifndef USEQT4
int uiTab::id()
    { return body_.identifier(); }

int uiTab::setName( const char* nm )
{
    body_.setText( nm );
    return UserIDObject::setName( nm );
}
#endif

class uiTabBarBody : public muiObjBodyImpl<uiTabBar,QTabBar>
{
public:

			uiTabBarBody( uiTabBar& handle, uiParent* p,
					const char* nm )
			    : muiObjBodyImpl<uiTabBar,QTabBar>(handle,p,nm)
			    , messenger_( *new i_tabbarMessenger(this, &handle))
			    {
				setHSzPol( uiObject::MedVar );
			    }

    virtual		~uiTabBarBody()	{ delete &messenger_; }

private:

    i_tabbarMessenger&	messenger_;

};


//------------------------------------------------------------------------------


uiTabBar::uiTabBar( uiParent* parnt, const char* nm, const CallBack* cb )
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , selected( this )
{ if( cb ) selected.notify(*cb); }


uiTabBarBody& uiTabBar::mkbody(uiParent* parnt, const char* nm )
{
    body_ = new uiTabBarBody(*this,parnt,nm);
    return *body_; 
}

int uiTabBar::addTab( uiTab* tab )
{
    if ( !tab ) return -1;
    tabs_ += tab;
    tab->group().display( true );
#ifdef USEQT4
    return body_->insertTab( tabs_.size(), QString(tab->name()) ); 
#else
    return body_->insertTab( &tab->body_ ); 
#endif
}

#ifndef USEQT4
int uiTabBar::insertTab( uiTab* tab, int index )
{
    if ( !tab ) return -1;
    tabs_ += tab;
    tab->group().display( true );
    return body_->insertTab( &tab->body_, index ); 
}
#endif

void uiTabBar::removeTab( mRemoveTabArg )
{
#ifdef USEQT4
    int id = idOf(tab);
    if ( id < 0 ) return;
#else
    uiTab* tab = tabs_[index];
    if ( !tab ) return;
#endif

    tab->group().display( false );
    tabs_ -= tab;

#ifdef USEQT4
    body_->removeTab( id );
#else
    body_->removeTab( &tab->body_ );
#endif
    delete tab;
}

void uiTabBar::removeTab( uiGroup* grp )
{
    for ( int idx=0; idx<tabs_.size(); idx++ )
    {
	if ( &tabs_[idx]->group() == grp )
	{
#ifdef USEQT4
	    removeTab( tabs_[idx] );
#else
	    removeTab( idx );
#endif
	    return;
	}
    }
}


void uiTabBar::setTabEnabled( int idx, bool yn )
    { body_->setTabEnabled( idx, yn ); }

bool uiTabBar::isTabEnabled( int idx ) const
    { return body_->isTabEnabled( idx ); }

void uiTabBar::setCurrentTab(int id)
    { body_->setCurrentTab( id ); }

int uiTabBar::currentTabId() const
#ifdef USEQT4
    { return body_->currentIndex(); }
#else
    { return body_->currentTab(); }
#endif

int uiTabBar::size() const
    { return body_->count(); }


int uiTabBar::idOf( uiGroup* grp ) const
{
    for ( int idx=0; idx<tabs_.size(); idx++ )
    {
	if ( &tabs_[idx]->group() == grp )
#ifdef USEQT4
	return idx;
#else
	return tabs_[idx]->id();
#endif
    }

    return -1;
}

#ifdef USEQT4
int uiTabBar::idOf( uiTab* tab ) const
{
    for ( int idx=0; idx<tabs_.size(); idx++ )
    {
	if ( tabs_[idx] == tab ) return idx;
    }

    return -1;
}
#endif


uiGroup* uiTabBar::page( int id ) const
{
#ifdef USEQT4
    return &tabs_[id]->group();
#else
    for ( int idx=0; idx<tabs_.size(); idx++ )
    {
	if ( tabs_[idx]->id() == id ) return &tabs_[idx]->group();
    }

    return 0;
#endif
}
