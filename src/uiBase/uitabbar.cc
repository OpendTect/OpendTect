/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          17/01/2002
 RCS:           $Id: uitabbar.cc,v 1.4 2003-10-17 14:19:02 bert Exp $
________________________________________________________________________

-*/

#include "uitabbar.h"
#include "uiobjbody.h"
#include "pixmap.h"
#include "sets.h"

#include "i_qtabbar.h"

#include "qiconset.h"

class uiTabBody : public QTab
{
public:
		uiTabBody( uiTab& handle,  const char* nm )
		    : QTab( nm )
		    , handle_(handle) {}

protected:

    uiTab&	handle_;
};



uiTab::uiTab( uiGroup& grp )
    : UserIDObject( grp.name() )
    , body_( *new uiTabBody(*this,grp.name()) )
    , grp_( grp )
{}

int uiTab::id()
    { return body_.identifier(); }





class uiTabBarBody : public uiObjBodyImpl<uiTabBar,QTabBar>
{

public:

			uiTabBarBody( uiTabBar& handle, uiParent* p,
					const char* nm )
			    : uiObjBodyImpl<uiTabBar,QTabBar>(handle,p,nm)
			    , messenger_( *new i_tabbarMessenger(this, &handle))
			    {
				setHSzPol( uiObject::medvar );
			    }

    virtual		~uiTabBarBody()	{ delete &messenger_; }
/*
    void		addGrpTotabs(uiGroup& grp) { tabs += &grp; }
    virtual void	finalise(bool t)
			{
			    for ( int idx=0; idx< tabs.size(); idx++ )
				tabs[idx]->finalise();

			    uiObjBodyImpl<uiTabBar,QTabBar>::finalise(t);
			}

protected:

    ObjectSet<uiGroup>  tabs;
*/
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
    body_= new uiTabBarBody(*this,parnt,nm);
    return *body_; 
}

int uiTabBar::addTab( uiTab* tab )
{
    if ( !tab ) return -1;
    tabs_ += tab;
    return body_->addTab( &tab->body_ ); 
}

int uiTabBar::insertTab( uiTab* tab, int index )
{
    if ( !tab ) return -1;
    tabs_ += tab;
    return body_->insertTab( &tab->body_, index ); 
}

void uiTabBar::removeTab( uiTab* tab )
    { if ( tab ) body_->removeTab( &tab->body_ ); }

void uiTabBar::setTabEnabled( int idx, bool yn )
    { body_->setTabEnabled( idx, yn ); }

bool uiTabBar::isTabEnabled( int idx ) const
    { return body_->isTabEnabled( idx ); }

void uiTabBar::setCurrentTab(int id)
    { body_->setCurrentTab( id ); }

int uiTabBar::currentTabId() const
    { return body_->currentTab(); }

int uiTabBar::keyboardFocusTabId() const
    { return body_->keyboardFocusTab(); }

int uiTabBar::size() const
    { return body_->count(); }


int uiTabBar::idOf( uiGroup* grp ) const
{
    for ( int idx=0; idx<tabs_.size(); idx++ )
    {
	if ( &tabs_[idx]->group() == grp ) return tabs_[idx]->id();
    }

    return -1;
}


uiGroup* uiTabBar::page( int id ) const
{
    for ( int idx=0; idx<tabs_.size(); idx++ )
    {
	if ( tabs_[idx]->id() == id ) return &tabs_[idx]->group();
    }

    return 0;
}
