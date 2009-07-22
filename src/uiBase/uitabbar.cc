/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          17/01/2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uitabbar.cc,v 1.23 2009-07-22 16:01:38 cvsbert Exp $";

#include "uitabbar.h"
#include "uiobjbody.h"

#include "i_qtabbar.h"
#include <QApplication>
#include <QEvent>


uiTab::uiTab( uiGroup& grp )
    : NamedObject( grp.name() )
    , grp_( grp )
{}


class uiTabBarBody : public uiObjBodyImpl<uiTabBar,QTabBar>
{
public:
			uiTabBarBody( uiTabBar& handle, uiParent* p,
				      const char* nm )
			    : uiObjBodyImpl<uiTabBar,QTabBar>(handle,p,nm)
			    , messenger_(*new i_tabbarMessenger(this,&handle))
			    {
				setHSzPol( uiObject::MedVar );
			    }

    virtual		~uiTabBarBody()	{ delete &messenger_; }

    void		activate(int idx);
    bool		event(QEvent*);

protected:
    int			activateidx_;

private:

    i_tabbarMessenger&	messenger_;

};


static const QEvent::Type sQEventActivate = (QEvent::Type) (QEvent::User+0);

void uiTabBarBody::activate( int idx )
{
    activateidx_ = idx;
    QEvent* actevent = new QEvent( sQEventActivate );
    QApplication::postEvent( this, actevent );
}

bool uiTabBarBody::event( QEvent* ev )
{
    if ( ev->type() != sQEventActivate )
	return QTabBar::event( ev );

    if ( activateidx_>=0 && activateidx_<handle_.size() )
    {
	handle_.setCurrentTab( activateidx_ );
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
    const int idx = indexOf( tab );
    if ( idx < 0 ) return;

    tab->group().display( false );
    tabs_ -= tab;

    body_->removeTab( idx );
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


void uiTabBar::setCurrentTab( int idx )
{ body_->setCurrentIndex( idx ); }


int uiTabBar::currentTabId() const
{ return body_->currentIndex(); }


const char* uiTabBar::textOfTab( int idx ) const
{ return idx>=0 && idx<size() ? tabs_[idx]->name().buf() : 0; }


int uiTabBar::size() const
{ return body_->count(); }


int uiTabBar::indexOf( const uiGroup* grp ) const
{
    for ( int idx=0; idx<tabs_.size(); idx++ )
    {
	if ( &tabs_[idx]->group() == grp )
	    return idx;
    }

    return -1;
}


int uiTabBar::indexOf( const uiTab* tab ) const
{
    for ( int idx=0; idx<tabs_.size(); idx++ )
    {
	if ( tabs_[idx] == tab ) return idx;
    }

    return -1;
}


uiGroup* uiTabBar::page( int idx ) const
{
    return const_cast<uiGroup*>( &tabs_[idx]->group() );
}


void uiTabBar::activate( int idx )
{ body_->activate( idx ); }
