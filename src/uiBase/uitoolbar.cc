/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2001
 RCS:           $Id: uitoolbar.cc,v 1.1 2001-05-30 16:36:26 arend Exp $
________________________________________________________________________

-*/

#include "uitoolbar.h"
#include "uimain.h"
#include "uimainwin.h"
#include "pixmap.h"
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qapplication.h>

#include "qobject.h"

#include "i_qtoolbut.h"


uiToolBar* uiToolBar::getNew( const char* nm, ToolBarDock d, bool newline,
			      uiMainWin* main )
{
    if ( !main ) main = uiMain::theMain().topLevel();

#if 1
    QMainWindow* mw = static_cast<QMainWindow*>( qApp->mainWidget() );
#else
    QMainWindow* mw = main ? dynamic_cast<QMainWindow*>( &main->qWidget() )
			   : dynamic_cast<QMainWindow*>( qApp->mainWidget() );
#endif
    if ( !mw ) return 0;

    return new uiToolBar( nm, mw, d );
}


uiToolBar::uiToolBar(const char* nm,QMainWindow* mw,ToolBarDock d, bool newline)
    : qbar( new QToolBar( QString(nm), mw, (QMainWindow::ToolBarDock) qdock(d),
	     newline ) )
{}


uiToolBar::~uiToolBar()
    { deepErase(receivers); }


void uiToolBar::addButton(const ioPixmap& pm, const CallBack& cb,const char* nm)
{
    i_QToolButReceiver* br= new i_QToolButReceiver;
    QToolButton* but= new QToolButton( *pm.Pixmap(), QString(nm), QString::null,
                           br,  SLOT(buttonPressed()),qbar, nm );

    receivers += br;

    br->pressed.notify( cb );
}


const QWidget* uiToolBar::qWidget_() const     
    { return qbar; }


int uiToolBar::qdock(ToolBarDock d )
{
    switch( d )
    {
	case Top:	return QMainWindow::Top;
	case Bottom:	return QMainWindow::Bottom;
	case Right:	return QMainWindow::Right;
	case Left:	return QMainWindow::Left;
	case Minimized:	return QMainWindow::Minimized;
    }
    return 0;
}

