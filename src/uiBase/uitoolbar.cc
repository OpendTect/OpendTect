/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2001
 RCS:           $Id: uitoolbar.cc,v 1.17 2003-04-22 09:49:49 arend Exp $
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


    int 		addButton( const ioPixmap&, const CallBack& cb, 
				   const char* nm="ToolBarButton",
				   bool toggle=false );
    void		turnOn(int idx, bool yn );
    void		setSensitive(int idx, bool yn );
    void		setSensitive(bool yn);

    void		display(bool yn=true);
			//!< you must call this after all buttons are added

//    void		addChild( uiObjHandle* child, uiBody*, bool manage=true );

    static QMainWindow::ToolBarDock
			qdock(uiToolBar::ToolBarDock);

protected:
    virtual const QWidget*      managewidg_() const	{ return qwidget(); }

private:
    ObjectSet<i_QToolButReceiver> receivers; // for deleting
    ObjectSet<QToolButton>	  buttons;
};


int uiToolBarBody::addButton(const ioPixmap& pm, const CallBack& cb,
			      const char* nm, bool toggle )
{
#if 0 

    uiToolButton* but = new uiToolButton( this, nm, pm, &cb );

#else

    i_QToolButReceiver* br= new i_QToolButReceiver;
    QToolButton* but= new QToolButton( *pm.Pixmap(), QString(nm), QString::null,
                           br,  SLOT(buttonPressed()),qthing(), nm );

    if ( toggle ) but->setToggleButton( true );

    receivers += br;
    buttons += but;

    br->pressed.notify( cb );
    return buttons.size()-1;
#endif
}


void uiToolBarBody::turnOn( int idx, bool yn )
{
    buttons[idx]->setOn( yn );
}


void uiToolBarBody::setSensitive( int idx, bool yn )
{
    buttons[idx]->setEnabled( yn );
}


void uiToolBarBody::setSensitive( bool yn )
{
    if ( qwidget() ) qwidget()->setEnabled( yn );
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

uiToolBar::uiToolBar( const char* nm, QToolBar& qtb )
    : uiParent( nm, 0 )
    { setBody( &mkbody(nm,qtb) ); }


uiToolBarBody& uiToolBar::mkbody( const char* nm, QToolBar& qtb )
{ 
    body_=new uiToolBarBody( *this, qtb );
    return *body_; 
}


uiToolBar* uiToolBar::getNew( QMainWindow& main, const char* nm, ToolBarDock d, 
			      bool newline )
{
    QMainWindow::ToolBarDock d_ = uiToolBarBody::qdock(d);
    QToolBar& bar = *new QToolBar( QString(nm), &main, d_, newline );

    return new uiToolBar( nm, bar );
}


int uiToolBar::addButton(const ioPixmap& pm, const CallBack& cb,
			 const char* nm, bool toggle)
{ return body_->addButton(pm,cb,nm,toggle); }


void uiToolBar::setLabel( const char* lbl )
{
    if ( body_->qthing() ) 
	body_->qthing()->setLabel( QString(lbl) );
    setName( lbl );
}


void uiToolBar::turnOn( int idx, bool yn )
{ body_->turnOn( idx, yn ); }


void uiToolBar::setSensitive( int idx, bool yn )
{ body_->setSensitive( idx, yn ); }


void uiToolBar::setSensitive( bool yn )
{ body_->setSensitive( yn ); }


void uiToolBar::display( bool yn, bool,bool)
{
    if ( !body_->qthing() ) return;
    if ( yn )	body_->qthing()->show();
    else	body_->qthing()->hide();
}


void uiToolBar::addSeparator()
{
    if ( !body_->qthing() ) return;
    body_->qthing()->addSeparator();
}


void uiToolBar::setStretchableWidget( uiObject* obj )
{
    if ( !body_->qthing() || !obj ) return;
    body_->qthing()->setStretchableWidget( obj->body()->qwidget() );
}


void uiToolBar::setMovingEnabled( bool yn )
{
    if ( !body_->qthing() ) return;
    body_->qthing()->setMovingEnabled( yn );
}


bool uiToolBar::isMovingEnabled()
{
    if ( !body_->qthing() ) return false;
    return body_->qthing()->isMovingEnabled();
}
