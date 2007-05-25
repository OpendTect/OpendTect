/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		December 2006
 RCS:		$Id: uisplashscreen.cc,v 1.3 2007-05-25 10:31:05 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uisplashscreen.h"
#include "uimainwin.h"
#include "pixmap.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QSplashScreen>


uiSplashScreen::uiSplashScreen( const ioPixmap& pm )
{
    QDesktopWidget* qdw = QApplication::desktop();
    QWidget* parent = qdw->screen( qdw->primaryScreen() );
    qsplashscreen_ = new QSplashScreen( parent,  *pm.qpixmap(),
	    				Qt::WindowStaysOnTopHint );
}


uiSplashScreen::~uiSplashScreen()
{ delete qsplashscreen_; }

void uiSplashScreen::show()
{ qsplashscreen_->show(); }

void uiSplashScreen::finish( uiMainWin* mw )
{ qsplashscreen_->finish( mw->qWidget() ); }

void uiSplashScreen::showMessage( const char* msg )
{ qsplashscreen_->showMessage( msg ); }
