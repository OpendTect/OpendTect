/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          26/04/2000
 RCS:           $Id: uimsg.cc,v 1.22 2004-12-23 12:32:57 arend Exp $
________________________________________________________________________

-*/


#include "uimsg.h"

#include "uicursor.h"
#include "uimain.h"
#include "uimainwin.h"
#include "uistatusbar.h"
#include "uiobj.h"
#include "uibody.h"
#include "uiparentbody.h"

#undef Ok
#include <qmessagebox.h>
#include <iostream>

uiMsg* uiMsg::theinst_ = 0;


uiMsg::uiMsg()
	: uimainwin_(0)
{
}

uiMainWin* uiMsg::setMainWin( uiMainWin* m )
{
    uiMainWin* old = uimainwin_;
    uimainwin_ = m;
    return old;
}


void uiMsg::handleMsg( CallBacker* cb )
{
    mDynamicCastGet(MsgClass*,mc,cb)
    if ( !mc ) return;

    switch ( mc->type() )
    {
    case MsgClass::ProgrammerError:
#ifdef __debug__
	std::cerr << "(PE) " << mc->msg << std::endl;
#endif
    break;
    default:
	std::cerr << mc->msg << std::endl;
    break;
    }
}

uiStatusBar* uiMsg::statusBar()
{
    uiMainWin* mw = uimainwin_;/* ? uimainwin_ 
			       : uiMainWin::gtUiWinIfIsBdy( parent_ );*/

    if ( !mw || !mw->statusBar() )
	mw = uiMainWin::activeWindow();

    if ( !mw || !mw->statusBar() )
	mw = uiMain::theMain().topLevel();

    return mw ? mw->statusBar() : 0;
}


QWidget* uiMsg::popParnt()
{
    uiMainWin* mw = uiMainWin::activeWindow();
    if ( !mw ) mw = uimainwin_;
    if ( !mw ) mw = uiMain::theMain().topLevel();

    if ( !mw  )		return 0;
    return mw->body()->qwidget();
}


void uiMsg::toStatusbar( MsgClass* mc )
{
    if ( !mc )
    {
	if ( statusBar() ) statusBar()->message( "" );
	return;
    }

    BufferString msg;
    if ( mc->type() > MsgClass::Message
      && mc->type() < MsgClass::ProgrammerError )
	msg = mc->type() == MsgClass::Warning ? "[Warning]: " : "Error: ";
    msg += mc->msg;

    if ( !statusBar() )
	std::cerr << msg << std::endl;
    else
	statusBar()->message( msg );
}


void uiMsg::message( const char* text, const char* caption )
{
    uiCursorChanger uicursor( uiCursor::Arrow );
    if ( !caption || !*caption ) caption = "Information";
    QMessageBox::information( popParnt(),
			      QString(caption), QString(text), QString("Ok") );

}


void uiMsg::warning( const char* text, const char* caption )
{
    uiCursorChanger uicursor( uiCursor::Arrow );
    if ( !caption || !*caption ) caption = "Warning";
    QMessageBox::warning( popParnt(),
			  QString(caption), QString(text), QMessageBox::Ok,
		          QMessageBox::NoButton, QMessageBox::NoButton );
}


void uiMsg::error( const char* text, const char* caption )
{
    uiCursorChanger uicursor( uiCursor::Arrow );
    if ( !caption || !*caption ) caption = "Error";
    QMessageBox::critical( popParnt(),
			   QString(caption), QString(text), QString("Ok") );
}


void uiMsg::about( const char* text, const char* caption )
{
    uiCursorChanger uicursor( uiCursor::Arrow );
    if ( !caption || !*caption ) caption = "About";
    QMessageBox::about( popParnt(), QString(caption), QString(text) );
}


bool uiMsg::askGoOn( const char* text, bool yn, const char* caption )
{
    uiCursorChanger uicursor( uiCursor::Arrow );
    if ( !caption || !*caption ) caption = "Please specify";
    return !QMessageBox::warning( popParnt(),
				  QString(caption), QString(text),
				  QString(yn?"Yes":"OK"),
				  QString(yn?"No":"Cancel"),
				  QString::null,0,1);
}



int uiMsg::askGoOnAfter( const char* text, const char* cnclmsg,
			 const char* caption )
{
    uiCursorChanger uicursor( uiCursor::Arrow );
    if ( !cnclmsg || !*cnclmsg ) cnclmsg = "Cancel";
    if ( !caption || !*caption ) caption = "Please specify";
    return QMessageBox::warning( popParnt(), QString(caption), QString(text),
				  QString("Yes"), QString("No"),
				  QString(cnclmsg), 0, 2 );
}
