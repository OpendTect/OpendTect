/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          26/04/2000
 RCS:           $Id: uimsg.cc,v 1.4 2001-08-23 14:59:17 windev Exp $
________________________________________________________________________

-*/


#include "uimsg.h"
#include "uimain.h"
#include "uimainwin.h"
#include "uistatusbar.h"
#include "uiobj.h"
#include "uibody.h"
#include "uiparentbody.h"

#undef Ok
#include <qmessagebox.h>

uiMsg* uiMsg::theinst_ = 0;


uiMsg::uiMsg()
	: mainwin_(uiMain::theMain().topLevel())
{
}


void uiMsg::handleMsg( CallBacker* cb )
{
    mDynamicCastGet(MsgClass*,mc,cb)
    if ( !mc ) return;

    switch ( mc->type() )
    {
    case MsgClass::Warning:
	warning( mc->msg );
    break;
    case MsgClass::Error:
	error( mc->msg );
    break;
    case MsgClass::ProgrammerError:
#ifdef __debug__
	cerr << "(PE) " << mc->msg << endl;
#endif
    break;
    default:
	if ( getenv("dGB_NO_STATUS_BAR_MESSAGES") )
	    cerr << mc->msg << endl;
	else
	    toStatusbar( mc );
    break;
    }
}


void uiMsg::toStatusbar( MsgClass* mc )
{
    if ( !mc ) { mainwin_->statusBar()->message( "" ); return; }

    BufferString msg;
    if ( mc->type() > MsgClass::Message )
    {
	msg = eString( MsgClass::Type, mc->type() );
	msg += ": ";
    }
    msg += mc->msg;
    mainwin_->statusBar()->message( msg );
}


void uiMsg::message( const char* text, const char* caption )
{
    QMessageBox::information( mainwin_->body()->qwidget(),
			      QString(caption), QString(text), QString("Ok") );
}


void uiMsg::warning( const char* text, const char* caption )
{
    QMessageBox::warning( mainwin_->body()->qwidget(), QString(caption),
			  QString(text), QString("Ok") );
}


void uiMsg::error( const char* text, const char* caption )
{
    QMessageBox::critical( mainwin_->body()->qwidget(), QString(caption),
			   QString(text), QString("Ok") );
}


void uiMsg::about( const char* text, const char* caption )
{
    QMessageBox::about( mainwin_->body()->qwidget(), QString(caption),
			QString(text) );
}


bool uiMsg::askGoOn( const char* text, const char* caption )
{
    return !QMessageBox::warning( mainwin_->body()->qwidget(), QString(caption),
                                  QString(text), QString("Yes"), QString("No"));
}
