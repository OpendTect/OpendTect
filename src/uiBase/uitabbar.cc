/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          17/01/2002
________________________________________________________________________

-*/

#include "uitabbar.h"
#include "i_qtabbar.h"

#include "uiicon.h"
#include "uiobjbodyimpl.h"

#include "q_uiimpl.h"


mUseQtnamespace

uiTab::uiTab( uiGroup& grp, const uiString& caption )
    : grp_( grp )
    , caption_( caption.isEmpty() ? toUiString(grp.name()) : caption )
{}


void uiTab::setCaption( const uiString& caption )
{ caption_ = caption; }


class uiTabBarBody : public uiObjBodyImpl<uiTabBar,QTabBar>
{
public:
			uiTabBarBody( uiTabBar& hndl, uiParent* p,
				      const char* nm )
			    : uiObjBodyImpl<uiTabBar,QTabBar>(hndl,p,
									nm)
			    , messenger_(*new i_tabbarMessenger(this,&hndl))
			    {
				setHSzPol( uiObject::MedVar );
			    }

    virtual		~uiTabBarBody()	{ delete &messenger_; }

protected:
    int			activateidx_;

private:

    i_tabbarMessenger&	messenger_;

};


//------------------------------------------------------------------------------


uiTabBar::uiTabBar( uiParent* parnt, const char* nm, const CallBack* cb )
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , selected( this )
    , tabToBeClosed(this)
{ if( cb ) selected.notify(*cb); }


uiTabBar::~uiTabBar()
{
    deepErase( tabs_ );
}


uiTabBarBody& uiTabBar::mkbody( uiParent* parnt, const char* nm )
{
    body_ = new uiTabBarBody( *this, parnt, nm );
    return *body_;
}


int uiTabBar::addTab( uiTab* tab )
{
    mBlockCmdRec;
    if ( !tab )
	return -1;

    tabs_ += tab;
    tab->group().display( tabs_.size()==1 );
    const int tabidx =
	body_->insertTab( tabs_.size(), toQString(tab->getCaption()) );
    return tabidx;
}


int uiTabBar::insertTab( uiTab* tab, int index )
{
    mBlockCmdRec;
    if ( !tab )
	return -1;

    tabs_.insertAt( tab, index );
    const int tabidx =
	body_->insertTab( index, toQString(tab->getCaption()) );
    return tabidx;
}


void uiTabBar::removeTab( uiTab* tab )
{
    mBlockCmdRec;
    const int idx = indexOf( tab );
    if ( idx < 0 ) return;

    tab->group().display( false );
    tabs_ -= tab;
    body_->removeTab( idx );
    delete tab;
}


void uiTabBar::setTabText( int idx, const QString& text )
{
    body_->setTabText( idx, text );
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


void uiTabBar::setTabVisible( int idx, bool yn )
{
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
    body_->setTabVisible( idx, yn );
#endif
}


bool uiTabBar::isTabVisible( int idx ) const
{
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
    return body_->isTabVisible( idx );
#else
    return false;
#endif
}


void uiTabBar::setCurrentTab( int idx )
{
    mBlockCmdRec;
    body_->setCurrentIndex( idx );
}


void uiTabBar::setTabIcon( int idx, const char* icnnm )
{
    const uiIcon icon( icnnm );
    body_->setTabIcon( idx, icon.qicon() );
}


void uiTabBar::setTabsClosable( bool closable )
{
    body_->setTabsClosable( closable );
}


void uiTabBar::showCloseButton( int idx, bool yn, bool shrink )
{
    QWidget* qtwidget = body_->tabButton( idx, QTabBar::RightSide );
    if ( !qtwidget )
	return;

    if ( yn )
	qtwidget->show();
    else if ( shrink )
	qtwidget->resize( 0, 0 );
    else
	qtwidget->hide();
}


int uiTabBar::currentTabId() const
{ return body_->currentIndex(); }


uiString uiTabBar::textOfTab( int idx ) const
{ return idx>=0 && idx<size() ? tabs_[idx]->getCaption() : uiString(); }


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
