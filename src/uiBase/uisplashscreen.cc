/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";


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
    qsplashscreen_ = new QSplashScreen( parent, *pm.qpixmap() );
}


uiSplashScreen::~uiSplashScreen()
{ delete qsplashscreen_; }

void uiSplashScreen::show()
{ qsplashscreen_->show(); }

void uiSplashScreen::finish( uiMainWin* mw )
{ qsplashscreen_->finish( mw->qWidget() ); }

void uiSplashScreen::showMessage( const char* msg )
{ qsplashscreen_->showMessage( msg ); }
