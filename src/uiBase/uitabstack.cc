/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          17/01/2002
 RCS:           $Id: uitabstack.cc,v 1.3 2003-04-23 10:43:21 arend Exp $
________________________________________________________________________

-*/

#include <uitabstack.h>
#include <uitabbar.h>

#include <uidobjset.h>
#include <uiobjbody.h>
#include <pixmap.h>
#include <sets.h>
#include <qframe.h>


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
//    tabgrp_->setStretch( 2, 2 );

    tabbar_->selected.notify( mCB( this, uiTabStack, tabSel ) );
}

void uiTabStack::tabSel( CallBacker* cb )
{
    int id = tabbar_->currentTabId();
    uiGroup* selgrp = page( id );
    ObjectSet<uiTab>& tabs = tabbar_->tabs_;

    for ( int idx=0; idx<tabs.size(); idx++ )
    {
	bool disp = tabs[idx]->id() == id;
	tabs[idx]->group().display( disp );
    }
}

void uiTabStack::addTab( uiGroup* grp, const char* txt )
{
    if ( !grp ) return;
    if ( !txt || !*txt ) txt = grp->name();

    uiTab* tab = new uiTab( *grp );
    tabbar_->addTab( tab );

//    grp->setStretch(2,2);

    if ( !hAlignObj() )
{
#ifdef __debug__
cout << "Setting align group: " << grp->name() << endl;
#endif

	setHAlignObj( grp );
}
}

void uiTabStack::insertTab( uiGroup* grp, const char* txt, int index )
{
    if ( !grp ) return;
    if ( !txt || !*txt ) txt = grp->name();
    uiTab* tab = new uiTab( *grp );
    tabbar_->insertTab( tab, index );

    if ( !hAlignObj() )
	setHAlignObj( grp );
}


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
{
    return tabbar_->idOf( grp );
}


int uiTabStack::size() const
{
    return tabbar_->size();
}

/*
void uiTabStack::removeTabToolTip( uiGroup* grp )
{
    if ( grp )
	body_->removeTabToolTip( grp->mainObject()->body()->qwidget() );
}


void uiTabStack::setTabToolTip( uiGroup* grp , const char* tip )
{ 
    if( grp ) 
	body_->setTabToolTip( grp->mainObject()->body()->qwidget(), QString(tip) ); 
}
*/

void uiTabStack::setCurrentPage( int id )
{ 
//    NotifyStopper stopper(currentChanged);
    tabbar_->setCurrentTab( id );
    tabSel();
}


void uiTabStack::setCurrentPage( uiGroup* grp )
{
    if( !grp ) return;
    setCurrentPage( idOf(grp) );
}


uiGroup* uiTabStack::currentPage() const
{
    return page( currentPageId() );
}


uiGroup* uiTabStack::page( int id ) const
{ 
    return tabbar_->page( id );
}


int uiTabStack::currentPageId() const
{ 
    return tabbar_->currentTabId();
}

/*
const char* uiTabStack::tabText( int idx ) const
{
    rettxt = (const char*)body_->label(idx);
    return (const char*)rettxt;
}


const char* uiTabStack::tabText( uiGroup* grp ) const
{
    rettxt = (const char*)body_->tabLabel(grp->mainObject()->body()->qwidget());
    return (const char*)rettxt;
}


void uiTabStack::setTabText( uiGroup* grp , const char* txt )
{
    if( !grp ) return;

    body_->setTabLabel( grp->mainObject()->body()->qwidget() , QString(txt) );
}
*/
