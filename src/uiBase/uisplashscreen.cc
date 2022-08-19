/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisplashscreen.h"

#include "uimainwin.h"
#include "uipixmap.h"

#include <QSplashScreen>
#if QT_VERSION < QT_VERSION_CHECK(5,11,0)
# include <QApplication>
# include <QDesktopWidget>
#endif

mUseQtnamespace

uiSplashScreen::uiSplashScreen( const uiPixmap& pm )
{
#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
    qsplashscreen_ = new QSplashScreen( *pm.qpixmap() );
#else
    QDesktopWidget* qdw = QApplication::desktop();
    QWidget* parent = qdw->screen( qdw->primaryScreen() );
    qsplashscreen_ = new QSplashScreen( parent, *pm.qpixmap() );
#endif
}


uiSplashScreen::~uiSplashScreen()
{ delete qsplashscreen_; }

void uiSplashScreen::show()
{ qsplashscreen_->show(); }

void uiSplashScreen::finish( uiMainWin* mw )
{ qsplashscreen_->finish( mw->qWidget() ); }

void uiSplashScreen::showMessage( const char* msg )
{ qsplashscreen_->showMessage( msg ); }
