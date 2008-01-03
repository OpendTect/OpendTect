/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          17/01/2002
 RCS:           $Id: uitabstack.cc,v 1.17 2008-01-03 12:16:03 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uitabstack.h"
#include "uitabbar.h"

#include "uiobjbody.h"
#include "sets.h"

#include <QFrame>


uiTabStack::uiTabStack( uiParent* parnt, const char* nm, bool mnge )
    : uiGroup( parnt, nm, mnge )
    , tabbar_( 0 )
    , tabgrp_( 0 )
{
    // Don't change the order of these constuctions!
    tabgrp_ = new uiGroup( this, nm );
    tabbar_ = new uiTabBar( this, nm );

    tabgrp_->setFrameStyle( QFrame::TabWidgetPanel | QFrame::Raised );
    tabgrp_->setBorder(10);
    tabgrp_->attach( stretchedBelow, tabbar_, 0 );

    tabbar_->selected.notify( mCB(this,uiTabStack,tabSel) );
}


void uiTabStack::tabSel( CallBacker* cb )
{
    int id = tabbar_->currentTabId();
    uiGroup* selgrp = page( id );
    ObjectSet<uiTab>& tabs = tabbar_->tabs_;

    for ( int idx=0; idx<tabs.size(); idx++ )
    {
	const bool disp = tabs[idx]->group() == selgrp;
	tabs[idx]->group().display( disp );
    }
}


void uiTabStack::addTab( uiGroup* grp, const char* txt )
{
    if ( !grp ) return;

    uiTab* tab = new uiTab( *grp );
    BufferString tabname = txt && *txt ? txt : (const char*)grp->name();
    tab->setName( tabname );
    tabbar_->addTab( tab );

    if ( !hAlignObj() )
	setHAlignObj( grp );
}


void uiTabStack::removeTab( uiGroup* grp )
{ tabbar_->removeTab( grp ); }


void uiTabStack::setTabEnabled( uiGroup* grp, bool yn )
{
    int id = idOf( grp );
    tabbar_->setTabEnabled( id, yn );
}


bool uiTabStack::isTabEnabled( uiGroup* grp ) const
{
    int id = idOf( grp );
    return tabbar_->isTabEnabled( id );
}


int uiTabStack::idOf( uiGroup* grp ) const
{ return tabbar_->idOf( grp ); }

int uiTabStack::size() const
{ return tabbar_->size(); }


void uiTabStack::setCurrentPage( int id )
{ 
    tabbar_->setCurrentTab( id );
    tabSel(0);
}


void uiTabStack::setCurrentPage( uiGroup* grp )
{
    if( !grp ) return;
    setCurrentPage( idOf(grp) );
}


uiGroup* uiTabStack::currentPage() const
{ return page( currentPageId() ); }

uiGroup* uiTabStack::page( int id ) const
{ return tabbar_->page( id ); }

int uiTabStack::currentPageId() const
{ return tabbar_->currentTabId(); }
