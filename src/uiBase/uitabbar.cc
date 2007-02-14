/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          17/01/2002
 RCS:           $Id: uitabbar.cc,v 1.17 2007-02-14 12:38:00 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uitabbar.h"
#include "uiobjbody.h"
#include "sets.h"

#include "i_qtabbar.h"
#include "qiconset.h"

#ifndef USEQT3
# include <QIcon>
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
    : NamedObject( grp.name() )
#ifdef USEQT3
    , body_( *new uiTabBody(*this,grp.name()) )
#endif
    , grp_( grp )
{}

#ifdef USEQT3
int uiTab::id() const
    { return body_.identifier(); }

void uiTab::setName( const char* nm )
{
    body_.setText( nm );
    NamedObject::setName( nm );
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
    tab->group().display( tabs_.size()==1 );
#ifdef USEQT3
    return body_->insertTab( &tab->body_ ); 
#else
    return body_->insertTab( tabs_.size(), QString(tab->name()) ); 
#endif
}

#ifdef USEQT3
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
#ifdef USEQT3
    uiTab* tab = tabs_[index];
    if ( !tab ) return;
#else
    int id = idOf(tab);
    if ( id < 0 ) return;
#endif

    tab->group().display( false );
    tabs_ -= tab;

#ifdef USEQT3
    body_->removeTab( &tab->body_ );
#else
    body_->removeTab( id );
#endif
    delete tab;
}

void uiTabBar::removeTab( uiGroup* grp )
{
    for ( int idx=0; idx<tabs_.size(); idx++ )
    {
	if ( &tabs_[idx]->group() == grp )
	{
#ifdef USEQT3
	    removeTab( idx );
#else
	    removeTab( tabs_[idx] );
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
#ifdef USEQT3
    { return body_->currentTab(); }
#else
    { return body_->currentIndex(); }
#endif

int uiTabBar::size() const
    { return body_->count(); }


int uiTabBar::idOf( const uiGroup* grp ) const
{
    for ( int idx=0; idx<tabs_.size(); idx++ )
    {
	if ( &tabs_[idx]->group() == grp )
#ifdef USEQT3
	return tabs_[idx]->id();
#else
	return idx;
#endif
    }

    return -1;
}

#ifndef USEQT3
int uiTabBar::idOf( const uiTab* tab ) const
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
#ifdef USEQT3
    for ( int idx=0; idx<tabs_.size(); idx++ )
    {
	if ( tabs_[idx]->id() == id )
	    return const_cast<uiGroup*>( &tabs_[idx]->group() );
    }

    return 0;
#else
    return const_cast<uiGroup*>( &tabs_[id]->group() );
#endif
}
