/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          17/01/2002
 RCS:           $Id: uitabbar.cc,v 1.19 2008-01-24 18:40:53 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uitabbar.h"
#include "uiobjbody.h"

#include "i_qtabbar.h"
#include <QApplication>
#include <QEvent>


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

    void		activate(int id);
    bool		event(QEvent*);

protected:
    int			activateid_;

private:

    i_tabbarMessenger&	messenger_;

};


static const QEvent::Type sQEventActivate = (QEvent::Type) (QEvent::User+0);

void uiTabBarBody::activate( int id )
{
    activateid_ = id;
    QEvent* actevent = new QEvent( sQEventActivate );
    QApplication::postEvent( this, actevent );
}

bool uiTabBarBody::event( QEvent* ev )
{
    if ( ev->type() != sQEventActivate )
	return QTabBar::event( ev );

    if ( activateid_>=0 && activateid_<handle_.size() )
    {
	handle_.setCurrentTab( activateid_ );
	handle_.selected.trigger();
    }

    handle_.activatedone.trigger();
    return true;
}


//------------------------------------------------------------------------------


uiTabBar::uiTabBar( uiParent* parnt, const char* nm, const CallBack* cb )
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , selected( this )
    , activatedone( this )
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

const char* uiTabBar::textOfTab( int id ) const
{ return id>=0 && id<size() ? tabs_[id]->name().buf() : 0; }

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


void uiTabBar::activate( int id )
{ body_->activate( id ); }
