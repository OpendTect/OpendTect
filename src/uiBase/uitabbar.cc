/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          16/01/2002
 RCS:           $Id: uitabbar.cc,v 1.1 2002-01-16 11:01:23 arend Exp $
________________________________________________________________________

-*/

#include <uitabbar.h>
#include <uidobjset.h>
#include <uiobjbody.h>
#include <pixmap.h>

#include <i_qtabbar.h>

#include <qiconset.h>


class uiTabBarBody : public uiObjBodyImpl<uiTabBar,QTabBar>
{

public:

			uiTabBarBody( uiTabBar& handle, uiParent* p,
					const char* nm )
			    : uiObjBodyImpl<uiTabBar,QTabBar>(handle,p,nm)
			    , messenger_( *new i_tabbarMessenger(this, &handle))
			    {
				setSzPol( SzPolicySpec().setHSzP( 
						    SzPolicySpec::smallvar) );
			    }

    virtual int		nrTxtLines() const		{ return 1; }

private:

    i_tabbarMessenger&	messenger_;

};


//------------------------------------------------------------------------------


uiTabBar::uiTabBar(  uiParent* parnt, const char* nm, const CallBack* cb =0 )
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , selected( this )
{ if( cb ) selected.notify(*cb); }


uiTabBarBody& uiTabBar::mkbody(uiParent* parnt, const char* nm )
{
    body_= new uiTabBarBody(*this,parnt,nm);
    return *body_; 
}

int uiTabBar::addTab( const char* txt )
    { return body_->addTab( new QTab(QString(txt)) ); }

int uiTabBar::addTab( const ioPixmap& pm )
    { return body_->addTab( new QTab(QIconSet(*pm.Pixmap())) ); }


void uiTabBar::addTabs( const char** textList ) 
{
    const char* pt_cur = *textList;
    while ( pt_cur )
	addTab( pt_cur++ );
}


void uiTabBar::addTabs( const PtrUserIDObjectSet& uids )
{
    int curidx = indexOf(currentTab());
    if ( uids.currentIndex() >= 0 ) curidx = size() + uids.currentIndex() - 1;
    for ( int idx=0; idx<uids.size(); idx++ )
	addTab( uids[idx]->name() );
    setCurrentTab( uids[curidx]->name() );
}


void uiTabBar::addTabs( const ObjectSet<BufferString>& strs )
{
    for ( int idx=0; idx<strs.size(); idx++ )
	addTab( *strs[idx] );
}

void uiTabBar::setTabEnabled( int id, bool yn )
    { body_->setTabEnabled(id,yn);}

bool uiTabBar::isTabEnabled( int id ) const
    { return body_->isTabEnabled(id); }

int uiTabBar::currentTab() const
    { return body_->currentTab(); }

int uiTabBar::keyboardFocusTab() const
    { return body_->keyboardFocusTab(); }

int uiTabBar::indexOf( int id ) const
    { return body_->indexOf(id); }

int uiTabBar::size() const
    { return body_->count(); }

void uiTabBar::removeToolTip( int id )
    { body_->removeToolTip( id ); }

void uiTabBar::setToolTip( int id, const char* tip )
    { body_->QTabBar::setToolTip( id, QString(tip) ); }

const char* uiTabBar::toolTip( int id ) const
{
    if ( id < 0 || id >= body_->count() ) return "";
    rettxt = (const char*)body_->toolTip(id);
    return (const char*)rettxt;
}


void uiTabBar::setCurrentTab(int id )
{ 
    NotifyStopper stopper(selected);

    body_->setCurrentTab( id ); 
}

void uiTabBar::setCurrentTab( const char* txt )
{
    NotifyStopper stopper(selected);

    const int sz = body_->count();
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( body_->tabAt(idx) && body_->tabAt(idx)->text() == txt )
	    { body_->setCurrentTab( body_->tabAt(idx) ); return; }
    }
}


void uiTabBar::setTabText( int id, const char* txt )
{
    if ( body_->tab(id) )
	body_->tab(id)->setText( QString(txt) );
}

