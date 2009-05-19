/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          26/04/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uimsg.cc,v 1.45 2009-05-19 22:01:50 cvskris Exp $";


#include "uimsg.h"

#include "mousecursor.h"
#include "uimain.h"
#include "uimainwin.h"
#include "uistatusbar.h"
#include "uiobj.h"
#include "uibody.h"
#include "uiparentbody.h"

#undef Ok
#include <QMessageBox>

uiMsg* uiMsg::theinst_ = 0;
uiMsg& uiMSG()
{
    if ( !uiMsg::theinst_ )
	uiMsg::theinst_ = new uiMsg;
    return *uiMsg::theinst_;
}


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
    uiMainWin* mw = uimainwin_; //Always respect user's setting first.
    if ( !mw ) mw = uiMainWin::activeWindow();
    if ( !mw ) mw = uiMain::theMain().topLevel();

    if ( !mw  )		return 0;
    return mw->body()->qwidget();
}


bool uiMsg::toStatusbar( const char* msg, int fldidx, int msec )
{
    if ( !statusBar() ) return false;

    statusBar()->message( msg, fldidx, msec );
    return true;
}


static BufferString& gtCaptn()
{
    static BufferString* captn = 0;
    if ( !captn )
	captn = new BufferString;
    return *captn;
}


void uiMsg::setNextCaption( const char* s )
{
    gtCaptn() = s;
}


#define mPrepCursor() \
    MouseCursorChanger cc( MouseCursor::Arrow )
#define mPrepTxt() \
    mPrepCursor(); \
    BufferString msg( text ); if ( p2 ) msg += p2; if ( p3 ) msg += p3; \
    if ( msg.isEmpty() ) return
#define mCapt(s) QString( getCaptn(s) )
#define mTxt QString( msg.buf() )

static const char* getCaptn( const char* s )
{
    if ( gtCaptn().isEmpty() )
	return s;

    static BufferString oldcaptn;
    oldcaptn = gtCaptn();
    gtCaptn() = "";

    return oldcaptn.buf();
}


int uiMsg::beginCmdRecEvent()
{
    uiMainWin* carrier = uiMain::theMain().topLevel();
    if ( !carrier )
	return -1;

#ifdef __lux32__
    return carrier->beginCmdRecEvent( (od_uint32) this, "QMsgBoxBut" );
#else
    return carrier->beginCmdRecEvent( (od_uint64) this, "QMsgBoxBut" );
#endif

}

void uiMsg::endCmdRecEvent( int refnr, int retval, const char* buttxt0,
			    const char* buttxt1, const char* buttxt2 )
{
    BufferString msg( "QMsgBoxBut " );
    msg += !retval ? buttxt0 : ( retval==1 ? buttxt1 : buttxt2 );

    uiMainWin* carrier = uiMain::theMain().topLevel();
    if ( carrier )
#ifdef __lux32__
	carrier->endCmdRecEvent( (od_uint32) this, refnr, msg );
#else
	carrier->endCmdRecEvent( (od_uint64) this, refnr, msg );
#endif
}


#define mDeclArgs const char* text, const char* p2, const char* p3

void uiMsg::message( mDeclArgs )
{
    mPrepTxt();
    const char* oktxt = "&Ok";

    const int refnr = beginCmdRecEvent();
    QMessageBox::information( popParnt(), mCapt("Information"), mTxt,
	    		      QString(oktxt) );
    endCmdRecEvent( refnr, 0, oktxt );
}


void uiMsg::warning( mDeclArgs )
{
    mPrepTxt();
    const char* oktxt = "&Ok";

    const int refnr = beginCmdRecEvent();
    QMessageBox::warning( popParnt(), mCapt("Warning"), mTxt, QString(oktxt) );
    endCmdRecEvent( refnr, 0, oktxt );
}


void uiMsg::error( mDeclArgs )
{
    mPrepTxt();
    const char* oktxt = "&Ok";

    const int refnr = beginCmdRecEvent();
    QMessageBox::critical( popParnt(), mCapt("Error"), mTxt, QString(oktxt) );
    endCmdRecEvent(  refnr, 0, oktxt );
}


int uiMsg::askSave( const char* text, bool wcancel )
{
    const char* savetxt = "&Save";
    const char* dontsavetxt = "&Don't save";
    const char* canceltxt = "&Cancel";
    return question( text, savetxt, dontsavetxt,
	    	     wcancel ? canceltxt : 0, "Data not saved" );
}


int uiMsg::askRemove( const char* text )
{
    const char* yestxt = "&Remove";
    const char* notxt = "&Cancel";
    return question( text, yestxt, notxt, 0, "Remove data" );
}


int uiMsg::askContinue( const char* text )
{
    const char* yestxt = "&Continue";
    const char* notxt = "&Abort";
    return question( text, yestxt, notxt, 0 );
}


int uiMsg::askOverwrite( const char* text )
{
    const char* yestxt = "&Overwrite";
    const char* notxt = "&Cancel";
    return question( text, yestxt, notxt, 0 );
}


int uiMsg::question( const char* text, const char* yestxt, const char* notxt,
		     const char* cncltxt, const char* title )
{
    mPrepCursor();
    const int refnr = beginCmdRecEvent();

    if ( !yestxt || !*yestxt )
	yestxt = "Yes";
    if ( !notxt || !*notxt )
	notxt = "No";

    const int res = QMessageBox::question( popParnt(),
				mCapt(title), QString(text),
				QString(yestxt),
				QString(notxt),
				cncltxt ? QString(cncltxt) : QString::null,
			       	0, 2 );

    endCmdRecEvent( refnr, res, yestxt, notxt, cncltxt );
    return res == 0 ? 1 : (res == 1 ? 0 : -1);
}


void uiMsg::about( const char* text )
{
    mPrepCursor();
    const int refnr = beginCmdRecEvent();
    QMessageBox::about( popParnt(), mCapt("About"), QString(text) );
    endCmdRecEvent( refnr, 0, "OK" );
}


bool uiMsg::askGoOn( const char* text, bool yn )
{
    const char* oktxt = yn ? "&Yes" : "&Ok";
    const char* canceltxt = yn ? "&No" : "&Cancel";

    return askGoOn( text, oktxt, canceltxt );
}


bool uiMsg::askGoOn( const char* text, const char* textyes, const char* textno )
{
    mPrepCursor();

    const int refnr = beginCmdRecEvent();
    const int res = QMessageBox::warning( popParnt(), mCapt("Please specify"),
	    				  QString(text), QString(textyes),
					  QString(textno), QString::null, 0, 1);
    endCmdRecEvent( refnr, res, textyes, textno );
    return !res;
}


int uiMsg::askGoOnAfter( const char* text, const char* cnclmsg ,
			 const char* textyes, const char* textno )
{
    mPrepCursor();
    if ( !cnclmsg || !*cnclmsg )
	cnclmsg = "&Cancel";
    if ( !textyes || !*textyes )
	textyes = "&Yes";
    if ( !textno || !*textno )
	textno = "&No";

    const int refnr = beginCmdRecEvent();
    const int res = QMessageBox::warning( popParnt(),
				mCapt("Please specify"), QString(text),
				QString(textyes),
				QString(textno),
				QString(cnclmsg), 0, 2 );

    endCmdRecEvent( refnr, res, textyes, textno, cnclmsg );
    return res;
}


bool uiMsg::showMsgNextTime( const char* text, const char* ntmsg )
{
    mPrepCursor();
    const char* oktxt = "&Ok";
    if ( !ntmsg || !*ntmsg )
	ntmsg = "&Don't show this message again";

    const int refnr = beginCmdRecEvent();
    const int res = QMessageBox::warning( popParnt(),
				mCapt("Information"), QString(text),
				QString(oktxt),
				QString(ntmsg),
				QString::null, 0, 1 );

    endCmdRecEvent( refnr, res, oktxt, ntmsg );
    return !res;
}
