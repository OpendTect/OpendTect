/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2000
 RCS:           $Id: uistatusbar.cc,v 1.1 2000-11-27 10:20:35 bert Exp $
________________________________________________________________________

-*/

#include "uistatusbar.h"
#include "uimainwin.h"
#include "uimain.h" 

#include "i_qobjwrap.h"

#include <qstatusbar.h> 

typedef i_QObjWrapper<QStatusBar> i_QStatusBar;


uiStatusBar::uiStatusBar( uiMainWin* parnt, const char* nm, QStatusBar& qThing )
	: uiNoWrapObj<QStatusBar>(  &qThing, parnt, nm, false )
{
}

const QWidget* 	uiStatusBar::qWidget_() const 	{ return mQtThing(); } 

void uiStatusBar::message( const char* msg) 
{
    mQtThing()->message( msg );
    mQtThing()->repaint();
    uiMain::theMain().flushX();
}
