#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odwindow.h"

#include <QObject>
#include <QDockWidget>
#include <QMainWindow>

QT_BEGIN_NAMESPACE

class QMainWindowMessenger : public QObject
{
Q_OBJECT
friend class uiMainWin;
friend class uiMainWinBody;

protected:

QMainWindowMessenger( QMainWindow* qmw, uiMainWin* uimw )
    : qmw_(qmw)
    , uimw_(uimw)
{
   connect( qmw_, &QMainWindow::tabifiedDockWidgetActivated,
	    this, &QMainWindowMessenger::tabifiedDockWidgetActivated );
}


private Q_SLOTS:

void tabifiedDockWidgetActivated( QDockWidget* dockwidget )
{
    if ( !dockwidget )
	return;

    BufferString title( dockwidget->windowTitle() );
    // TODO: notify mainwindow and make scene that belongs to the dockwidget
    // active.
}

private:

    QMainWindow*	qmw_;
    uiMainWin*		uimw_;
};

QT_END_NAMESPACE
