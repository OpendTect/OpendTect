/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          26/04/2000
 RCS:           $Id: uimsg.cc,v 1.7 2001-12-16 15:12:29 bert Exp $
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
#include <iostream>

uiMsg* uiMsg::theinst_ = 0;


uiMsg::uiMsg()
	: mainwin_(uiMain::theMain().topLevel())
	, parent_(0)
{
}


void uiMsg::setParent( QWidget* p )
{
    parent_ = p;
    mainwin_ = 0;
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
    if ( !mc )
    {
	if ( mainwin_ ) mainwin_->statusBar()->message( "" );
	return;
    }

    BufferString msg;
    if ( mc->type() > MsgClass::Message )
    {
	msg = eString( MsgClass::Type, mc->type() );
	msg += ": ";
    }
    msg += mc->msg;

    if ( !mainwin_ )
	cerr << msg << endl;
    else
	mainwin_->statusBar()->message( msg );
}


void uiMsg::message( const char* text, const char* caption )
{
    QMessageBox::information( mainwin_ ? mainwin_->body()->qwidget() : parent_,
			      QString(caption), QString(text), QString("Ok") );
}


void uiMsg::warning( const char* text, const char* caption )
{
    QMessageBox::warning( mainwin_ ? mainwin_->body()->qwidget() : parent_,
			  QString(caption), QString(text), QString("Ok") );
}


void uiMsg::error( const char* text, const char* caption )
{
    QMessageBox::critical( mainwin_ ? mainwin_->body()->qwidget() : parent_,
			   QString(caption), QString(text), QString("Ok") );
}


void uiMsg::about( const char* text, const char* caption )
{
    QMessageBox::about( mainwin_ ? mainwin_->body()->qwidget() : parent_,
			QString(caption), QString(text) );
}


bool uiMsg::askGoOn( const char* text, const char* caption, bool yn )
{
    return !QMessageBox::warning( mainwin_ ? mainwin_->body()->qwidget()
						: parent_,
				  QString(caption), QString(text),
				  QString(yn?"Yes":"OK"),
				  QString(yn?"No":"Cancel"));
}
