/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          17/01/2002
 RCS:           $Id: uitabbar.cc,v 1.18 2008-01-03 12:16:03 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uitabbar.h"
#include "uiobjbody.h"

#include "i_qtabbar.h"


uiTab::uiTab( uiGroup& grp )
    : NamedObject( grp.name() )
    , grp_( grp )
{}


class uiTabBarBody : public uiObjBodyImplNoQtNm<uiTabBar,QTabBar>
{
public:
			uiTabBarBody( uiTabBar& handle, uiParent* p,
				      const char* nm )
			    : uiObjBodyImplNoQtNm<uiTabBar,QTabBar>(handle,p,nm)
			    , messenger_(*new i_tabbarMessenger(this,&handle))
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


uiTabBarBody& uiTabBar::mkbody( uiParent* parnt, const char* nm )
{
    body_ = new uiTabBarBody( *this, parnt, nm );
    return *body_; 
}

int uiTabBar::addTab( uiTab* tab )
{
    if ( !tab ) return -1;
    tabs_ += tab;
    tab->group().display( tabs_.size()==1 );
    return body_->insertTab( tabs_.size(), QString(tab->name()) ); 
}


void uiTabBar::removeTab( uiTab* tab )
{
    const int id = idOf( tab );
    if ( id < 0 ) return;

    tab->group().display( false );
    tabs_ -= tab;

    body_->removeTab( id );
    delete tab;
}


void uiTabBar::removeTab( uiGroup* grp )
{
    for ( int idx=0; idx<tabs_.size(); idx++ )
    {
	if ( &tabs_[idx]->group() == grp )
	{
	    removeTab( tabs_[idx] );
	    return;
	}
    }
}


void uiTabBar::setTabEnabled( int idx, bool yn )
{ body_->setTabEnabled( idx, yn ); }

bool uiTabBar::isTabEnabled( int idx ) const
{ return body_->isTabEnabled( idx ); }

void uiTabBar::setCurrentTab( int id )
{ body_->setCurrentTab( id ); }

int uiTabBar::currentTabId() const
{ return body_->currentIndex(); }

int uiTabBar::size() const
{ return body_->count(); }


int uiTabBar::idOf( const uiGroup* grp ) const
{
    for ( int idx=0; idx<tabs_.size(); idx++ )
    {
	if ( &tabs_[idx]->group() == grp )
	    return idx;
    }

    return -1;
}


int uiTabBar::idOf( const uiTab* tab ) const
{
    for ( int idx=0; idx<tabs_.size(); idx++ )
    {
	if ( tabs_[idx] == tab ) return idx;
    }

    return -1;
}


uiGroup* uiTabBar::page( int id ) const
{
    return const_cast<uiGroup*>( &tabs_[id]->group() );
}
