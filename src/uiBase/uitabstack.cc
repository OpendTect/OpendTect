/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitabstack.h"
#include "uitabbar.h"
#include "uiobjbody.h"
#include "objectset.h"

#include <QFrame>

mUseQtnamespace

uiTabStack::uiTabStack( uiParent* parnt, const char* nm, bool mnge )
    : uiGroup( parnt, nm, mnge )
    , tabToBeClosed(this)
    , tabClosed(this)
{
    // Don't change the order of these constuctions!
    tabgrp_ = new uiGroup( this, nm );
    tabbar_ = new uiTabBar( this, nm );

    tabgrp_->setFrameStyle( QFrame::StyledPanel | QFrame::Raised );
    tabgrp_->setBorder(10);
    tabgrp_->attach( stretchedBelow, tabbar_, 0 );

    tabbar_->selected.notify( mCB(this,uiTabStack,tabSel) );
    tabbar_->tabToBeClosed.notify( mCB(this,uiTabStack,tabCloseCB) );
}


uiTabStack::~uiTabStack()
{}


NotifierAccess& uiTabStack::selChange()
{
    return tabbar_->selected;
}


void uiTabStack::tabSel( CallBacker* )
{
    const int id = tabbar_->currentTabId();
    uiGroup* selgrp = page( id );
    ObjectSet<uiTab>& tabs = tabbar_->tabs_;

    for ( int idx=0; idx<tabs.size(); idx++ )
    {
	uiGroup* grp = &tabs[idx]->group();
	const bool disp = grp == selgrp;
	grp->display( disp );
    }
}


void uiTabStack::addTab( uiGroup* grp, const uiString& txt )
{
    if ( !grp ) return;

    const uiString tabcaption = !txt.isEmpty() ? txt : toUiString(grp->name());
    uiTab* tab = new uiTab( *grp, tabcaption );
    tabbar_->addTab( tab );

    if ( !hAlignObj() )
	setHAlignObj( grp );
}


int uiTabStack::insertTab( uiGroup* grp, int index, const uiString& txt )
{
    if ( !grp ) return -1;

    const uiString tabcaption = !txt.isEmpty() ? txt : toUiString(grp->name());
    uiTab* tab = new uiTab( *grp, tabcaption );
    if ( !hAlignObj() )
	setHAlignObj( grp );

    return tabbar_->insertTab( tab, index );
}


void uiTabStack::setTabText( int idx, const char* nm )
{
    tabbar_->setTabText( idx, nm );
}


void uiTabStack::removeTab( uiGroup* grp )
{
    tabbar_->removeTab( grp );
}


void uiTabStack::setTabEnabled( int idx, bool yn )
{
    tabbar_->setTabEnabled( idx, yn );
}


bool uiTabStack::isTabEnabled( int idx ) const
{
    return tabbar_->isTabEnabled( idx );
}

void uiTabStack::setTabEnabled( uiGroup* grp, bool yn )
{
    const int idx = indexOf( grp );
    tabbar_->setTabEnabled( idx, yn );
}


bool uiTabStack::isTabEnabled( uiGroup* grp ) const
{
    const int idx = indexOf( grp );
    return tabbar_->isTabEnabled( idx );
}


void uiTabStack::setTabVisible( uiGroup* grp, bool yn )
{
    const int idx = indexOf( grp );
    tabbar_->setTabVisible( idx, yn );
}


bool uiTabStack::isTabVisible( uiGroup* grp ) const
{
    const int idx = indexOf( grp );
    return tabbar_->isTabVisible( idx );
}


int uiTabStack::indexOf( uiGroup* grp ) const
{ return tabbar_->indexOf( grp ); }

int uiTabStack::size() const
{ return tabbar_->size(); }


void uiTabStack::setCurrentPage( int id )
{
    tabbar_->setCurrentTab( id );
    tabSel( nullptr );
}


void uiTabStack::setCurrentPage( uiGroup* grp )
{
    if( !grp )
	return;

    setCurrentPage( indexOf(grp) );
}


void uiTabStack::setCurrentPage( const char* grpnm )
{
    for ( int idx=0; grpnm && idx<size(); idx++ )
    {
	if ( page(idx)->name().isEqual(grpnm) )
	{
	    setCurrentPage( idx );
	    return;
	}
    }
}


void uiTabStack::setTabIcon( int idx, const char* icnnm )
{
    tabbar_->setTabIcon( idx, icnnm );
}


void uiTabStack::setTabsClosable( bool closable )
{
    tabbar_->setTabsClosable( closable );
}


void uiTabStack::showCloseButton( uiGroup* grp, bool yn, bool shrink )
{
    const int idx = indexOf( grp );
    tabbar_->showCloseButton( idx, yn, shrink );
}


void uiTabStack::tabCloseCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,tabid,cb);
    uiGroup* tabgrp = tabbar_->page( tabid );
    tabToBeClosed.trigger( tabid );
    removeTab( tabgrp );
    tabClosed.trigger();

}


void uiTabStack::setTabIcon( uiGroup* grp, const char* icnnm )
{
    const int idx = indexOf( grp );
    setTabIcon( idx, icnnm );
}


uiGroup* uiTabStack::currentPage() const
{ return page( currentPageId() ); }

uiGroup* uiTabStack::page( int id ) const
{ return tabbar_->page( id ); }

int uiTabStack::currentPageId() const
{ return tabbar_->currentTabId(); }
