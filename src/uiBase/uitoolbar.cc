/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2001
 RCS:           $Id: uitoolbar.cc,v 1.8 2001-11-22 08:38:07 arend Exp $
________________________________________________________________________

-*/

#include "uitoolbar.h"
#include "uimain.h"
#include "uimainwin.h"
#include "uiobj.h"
#include "pixmap.h"
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qapplication.h>
#include <qmainwindow.h>

#include "qobject.h"

#include "i_qtoolbut.h"

#include "uibody.h"


class uiToolBarBody : public uiBodyImpl<uiToolBar,QToolBar>
{
public:

			uiToolBarBody( uiToolBar& handle, QToolBar& bar )
			    : uiBodyImpl<uiToolBar,QToolBar>( handle, 0, bar )
			    {}

			~uiToolBarBody()	{ deepErase(receivers); }


    void 		addButton( const ioPixmap&, const CallBack& cb, 
				   const char* nm="ToolBarButton" );

    void		display(bool yn=true);
			//!< you must call this after all buttons are added

//    void		addChild( uiObjHandle* child, uiBody*, bool manage=true );

    static QMainWindow::ToolBarDock
			qdock(uiToolBar::ToolBarDock);

protected:
    virtual const QWidget*      managewidg_() const	{ return qwidget(); }

private:
    ObjectSet<i_QToolButReceiver> receivers; // for deleting
};



uiToolBar* uiToolBar::getNew( const char* nm, ToolBarDock d, bool newline,
			      uiMainWin* main )
{
    if ( !main ) main = uiMain::theMain().topLevel();

    QMainWindow* mw = ( main && main->uiObj() && main->uiObj()->body() ) ?
	dynamic_cast<QMainWindow*> ( main->body()->qwidget() )  : 0;

    if ( !mw ) return 0;

    QMainWindow::ToolBarDock d_ = uiToolBarBody::qdock(d);
    QToolBar& bar = *new QToolBar( QString(nm), mw, d_, newline );

    return new uiToolBar( nm, bar );
}


uiToolBar::uiToolBar( const char* nm, QToolBar& qtb )
    : uiParent( nm, 0 )
    { setBody( &mkbody(nm,qtb) ); }

uiToolBarBody& uiToolBar::mkbody( const char* nm, QToolBar& qtb )
{ 
    body_=new uiToolBarBody( *this, qtb );
    return *body_; 
}


void uiToolBar::addButton(const ioPixmap& pm, const CallBack& cb,const char* nm)
    { body_->addButton(pm,cb,nm); }


void uiToolBarBody::addButton(const ioPixmap& pm, const CallBack& cb,const char* nm)
{
#if 0 

    uiToolButton* but = new uiToolButton( this, nm, pm, &cb );

#else

    i_QToolButReceiver* br= new i_QToolButReceiver;
    QToolButton* but= new QToolButton( *pm.Pixmap(), QString(nm), QString::null,
                           br,  SLOT(buttonPressed()),qthing(), nm );

    receivers += br;

    br->pressed.notify( cb );

#endif
}


void uiToolBar::display( bool yn )
{
    if ( !body_->qthing() ) return;
    if ( yn )	body_->qthing()->show();
    else	body_->qthing()->hide();
}


QMainWindow::ToolBarDock uiToolBarBody::qdock( uiToolBar::ToolBarDock d )
{
    switch( d )
    {
	case uiToolBar::Top:		return QMainWindow::Top;
	case uiToolBar::Bottom:		return QMainWindow::Bottom;
	case uiToolBar::Right:		return QMainWindow::Right;
	case uiToolBar::Left:		return QMainWindow::Left;
	case uiToolBar::Minimized:	return QMainWindow::Minimized;
    }
    return (QMainWindow::ToolBarDock) 0;
}

