/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          31/05/2000
 RCS:           $Id: uimainwin.cc,v 1.1 2000-11-27 10:20:35 bert Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"
#include "qmainwindow.h"
#include "qwidget.h"
#include "i_qobjwrap.h"

#include "uistatusbar.h"
#include "qstatusbar.h"
#include "uimenu.h"
#include "uigroup.h"
#include "uimsg.h"

#include "msgh.h"
#include "errh.h"

#include <iostream.h>

uiMainWin::uiMainWin( uiObject* parnt, const char* nm , bool wantStatusBar, bool wantMenuBar)
	: uiObject( parnt, nm )
	, mLoMngr( 0 )
	, mQtThing(new i_QMainWindow(*this,
			parnt ? &parnt->clientQWidget() : 0, nm))
	, mCentralWidget(*new uiGroup("uiGroup client area",this))
	, mStatusBar( 0 ) , mMenuBar(0) 
{ 
    mLoMngr = mCentralWidget.mLayoutMngr(); 
    mQtThing->setCentralWidget( &mCentralWidget.qWidget() ); 

    if( wantStatusBar )
    {
	QStatusBar* mbar=mQtThing->statusBar();
	if( mbar )
	    mStatusBar = new uiStatusBar( this, "MainWindow StatusBar client", 
					  *mbar);
	else
	    pErrMsg("No statusbar returned from Qt");
    }
    if( wantMenuBar )
    {   
	QMenuBar* myBar =  mQtThing->menuBar();
	if( myBar )
	    mMenuBar = new uiMenuBar( this, "MainWindow MenuBar client", 
				      *myBar);
	else
	    pErrMsg("No menubar returned from Qt");
    }
}


uiMainWin::~uiMainWin( )
{
    delete &mCentralWidget;
}

void uiMainWin::forceRedraw_( bool deep )
{
    uiObject::forceRedraw_( deep );
    mCentralWidget.forceRedraw_( deep );
}

const QWidget* uiMainWin::qWidget_() const
{ return mQtThing; }

void uiMainWin::qThingDel( i_QObjWrp* qth )
{
    mQtThing=0;
}


const uiObject& uiMainWin::clientWidget_() const
{ 
    return mCentralWidget; 
}


uiStatusBar* uiMainWin::statusBar()
{
    if ( !mStatusBar) pErrMsg("No statusbar. See uiMainWin's constructor"); 
    return mStatusBar;
}


uiMenuBar* uiMainWin::menuBar()
{
    if ( !mMenuBar ) pErrMsg("No menuBar. See uiMainWin's constructor"); 
    return mMenuBar;
}


void uiMainWin::postShow( CallBacker* )
{
    MsgClass::theCB = mCB(&uiMSG(),uiMsg,handleMsg);
}
