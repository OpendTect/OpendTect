/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2001
 RCS:           $Id: uitoolbar.cc,v 1.20 2004-09-13 09:40:46 nanne Exp $
________________________________________________________________________

-*/

#include "uitoolbar.h"
#include "uimain.h"
#include "uibody.h"
#include "uiobj.h"
#include "uimainwin.h"
#include "pixmap.h"
#include "uiparentbody.h"

#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qapplication.h>
#include <qmainwindow.h>

#include "qobject.h"
#include "i_qtoolbut.h"



//class uiToolBarBody : public uiBodyImpl<uiToolBar,QToolBar>
class uiToolBarBody : public uiParentBody
{
public:

			uiToolBarBody( uiToolBar& handle, QToolBar& bar )
//			    : uiBodyImpl<uiToolBar,QToolBar>( handle, 0, bar )
			    : uiParentBody("ToolBar")
			    , qbar(&bar)
			    {}

			~uiToolBarBody()	{ deepErase(receivers); }


    int 		addButton( const ioPixmap&, const CallBack& cb, 
				   const char* nm="ToolBarButton",
				   bool toggle=false );
    void		turnOn(int idx, bool yn );
    void		setSensitive(int idx, bool yn );
    void		setSensitive(bool yn);
    void		setToolTip(int,const char*);

    void		display(bool yn=true);
			//!< you must call this after all buttons are added

//    void		addChild( uiObjHandle* child, uiBody*, bool manage=true );

    static QMainWindow::ToolBarDock
			qdock(uiToolBar::ToolBarDock);

protected:
//  virtual const QWidget*      managewidg_() const	{ return qwidget(); }
    virtual const QWidget*      managewidg_() const	{ return qbar; }
    virtual const QWidget*	qwidget_() const	{ return qbar; }
    QToolBar*			qbar;
    virtual void        attachChild ( constraintType tp,
                                      uiObject* child,
                                      uiObject* other, int margin,
                                      bool reciprocal )
			{
			    pErrMsg("Cannot attach uiObjects in a uiToolBar");
			    return;
			}


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
                           br,  SLOT(buttonPressed()),qbar, nm );

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


void uiToolBarBody::setToolTip( int idx, const char* tip )
{
    buttons[idx]->setTextLabel( QString(tip) );
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


uiToolBar::uiToolBar( uiParent* parnt, const char* nm, ToolBarDock d,
		      bool newline )
    : uiParent(nm,0)
    , qtoolbar(0)
{
    QMainWindow::ToolBarDock tbdock = uiToolBarBody::qdock(d);
    QWidget* qwidget = parnt && parnt->pbody() ? parnt->pbody()->qwidget() : 0;
    qtoolbar = new QToolBar( QString(nm), (QMainWindow*)qwidget, 
	    		     tbdock, newline );
    setBody( &mkbody(nm,*qtoolbar) );
}


uiToolBarBody& uiToolBar::mkbody( const char* nm, QToolBar& qtb )
{ 
    body_ = new uiToolBarBody( *this, qtb );
    return *body_; 
}


int uiToolBar::addButton( const ioPixmap& pm, const CallBack& cb,
			  const char* nm, bool toggle)
{ return body_->addButton(pm,cb,nm,toggle); }


void uiToolBar::setLabel( const char* lbl )
{
    qtoolbar->setLabel( QString(lbl) );
    setName( lbl );
}


void uiToolBar::turnOn( int idx, bool yn )
{ body_->turnOn( idx, yn ); }


void uiToolBar::setSensitive( int idx, bool yn )
{ body_->setSensitive( idx, yn ); }


void uiToolBar::setSensitive( bool yn )
{ body_->setSensitive( yn ); }


void uiToolBar::setToolTip( int idx, const char* tip )
{
    body_->setToolTip( idx, tip );
}


void uiToolBar::display( bool yn, bool, bool )
{
    if ( yn )
	qtoolbar->show();
    else
	qtoolbar->hide();
}


void uiToolBar::addSeparator()
{ qtoolbar->addSeparator(); }


void uiToolBar::setStretchableWidget( uiObject* obj )
{
    if ( !obj ) return;
    qtoolbar->setStretchableWidget( obj->body()->qwidget() );
}


void uiToolBar::setMovingEnabled( bool yn )
{ qtoolbar->setMovingEnabled( yn ); }


bool uiToolBar::isMovingEnabled()
{ return qtoolbar->isMovingEnabled(); }


void uiToolBar::dock()
{ qtoolbar->dock(); }

void uiToolBar::undock()
{ qtoolbar->undock(); }


void uiToolBar::setNewLine( bool yn )
{ qtoolbar->setNewLine( yn ); }
