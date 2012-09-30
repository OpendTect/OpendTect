/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          26/04/2000
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uimsg.h"

#include "mousecursor.h"
#include "separstr.h"
#include "bufstringset.h"
#include "uimain.h"
#include "uimainwin.h"
#include "uistatusbar.h"
#include "uiobj.h"
#include "uibody.h"
#include "uiparentbody.h"

#undef Ok
#include <QMessageBox>
#include <QAbstractButton>

mUseQtnamespace

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

#define mCapt(s) getCaptn( uiMainWin::uniqueWinTitle(s) )
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


int uiMsg::beginCmdRecEvent( const char* wintitle )
{
    uiMainWin* carrier = uiMain::theMain().topLevel();
    if ( !carrier )
	return -1;

    BufferString msg( "QMsgBoxBut " );
    msg += wintitle;

#ifdef __lux32__
    return carrier->beginCmdRecEvent( (od_uint32) this, msg );
#else
    return carrier->beginCmdRecEvent( (od_uint64) this, msg );
#endif

}


#define mEndCmdRecEvent( refnr, qmsgbox ) \
\
    QAbstractButton* abstrbut = qmsgbox.clickedButton(); \
    endCmdRecEvent( refnr, 0, \
		    (abstrbut ? mQStringToConstChar(abstrbut->text()) : "") );
	
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


#define mImplSimpleMsg( odfunc, caption, qtfunc ) \
void uiMsg::odfunc( const char* text, const char* p2, const char* p3 ) \
{ \
    mPrepCursor(); \
    BufferString msg( text ); if ( p2 ) msg += p2; if ( p3 ) msg += p3; \
    if ( msg.isEmpty() ) return; \
    const char* oktxt = "&Ok"; \
 \
    const char* title = mCapt(caption); \
    const int refnr = beginCmdRecEvent( title ); \
    QMessageBox::qtfunc( popParnt(), QString(title), \
	    			   mTxt, QString(oktxt) ); \
    endCmdRecEvent( refnr, 0, oktxt ); \
}

mImplSimpleMsg( message, "Information", information );
mImplSimpleMsg( warning, "Warning", warning );


void uiMsg::error( const char* p1, const char* p2, const char* p3 )
{
    BufferString msg( p1 ); if ( p2 ) msg += p2; if ( p3 ) msg += p3;
    errorWithDetails( FileMultiString(msg.buf()) );
}


void uiMsg::errorWithDetails( const BufferStringSet& bss, const char* before )
{
    FileMultiString fms;
    if ( before && *before )
	fms = before;

    fms += bss;

    errorWithDetails( fms );
}


void uiMsg::errorWithDetails( const FileMultiString& fms )
{
    if ( !fms.size() )
	return;

    MouseCursorChanger cc( MouseCursor::Arrow );

    const char* wintitle = mCapt("Error");
    const int refnr = beginCmdRecEvent( wintitle );

    QMessageBox msgbox( QMessageBox::Critical, QString(wintitle),
			QString(fms[0]), QMessageBox::Ok, popParnt() );
    if ( fms.size()>1 )
    {
	BufferString detailed;
	for ( int idx=0; idx<fms.size(); idx++ )
	{
	    if ( idx>0 )
		detailed += "\n";
	    detailed  += fms[idx];
	}

	msgbox.setDetailedText( QString( detailed.buf() ) );
    }

    msgbox.exec();
    mEndCmdRecEvent( refnr, msgbox );
}



int uiMsg::askSave( const char* text, bool wcancel )
{
    const char* savetxt = "&Save";
    const char* dontsavetxt = "&Don't save";
    const char* canceltxt = "&Cancel";
    return question( text, savetxt, dontsavetxt,
	    	     wcancel ? canceltxt : 0, "Data not saved" );
}


int uiMsg::askRemove( const char* text, bool wcancel )
{
    const char* yestxt = "&Remove";
    const char* notxt = wcancel ? "&Don't remove" : "&Cancel";
    const char* canceltxt = "&Cancel";
    return question( text, yestxt, notxt,
	    	     wcancel ? canceltxt : 0, "Remove data" );
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
    const char* wintitle = mCapt(title && *title ? title : "Please specify");
    const int refnr = beginCmdRecEvent( wintitle );

    if ( !yestxt || !*yestxt )
	yestxt = "Yes";
    if ( !notxt || !*notxt )
	notxt = "No";

    const int res = QMessageBox::question( popParnt(), QString(wintitle),
					   QString(text), QString(yestxt),
					   QString(notxt),
					   cncltxt ? QString(cncltxt)
					           : QString::null,
			       	           0, 2 );

    endCmdRecEvent( refnr, res, yestxt, notxt, cncltxt );
    return res == 0 ? 1 : (res == 1 ? 0 : -1);
}


void uiMsg::about( const char* text )
{
    mPrepCursor();
    const char* wintitle = mCapt("About");
    const int refnr = beginCmdRecEvent( wintitle );
    QMessageBox::about( popParnt(), QString(wintitle),
	    			  QString(text) );
    endCmdRecEvent( refnr, 0, "&OK" );
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

    const char* wintitle = mCapt("Please specify");
    const int refnr = beginCmdRecEvent( wintitle );
    const int res = QMessageBox::warning( popParnt(), QString(wintitle),
	    				  QString(text), QString(textyes),
					  QString(textno),
					  QString::null, 0, 1);
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

    const char* wintitle = mCapt("Please specify");
    const int refnr = beginCmdRecEvent( wintitle );
    const int res = QMessageBox::warning( popParnt(), QString(wintitle),
					  QString(text),
					  QString(textyes), QString(textno),
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

    const char* wintitle = mCapt("Information");
    const int refnr = beginCmdRecEvent( wintitle );

    const int res = QMessageBox::information( popParnt(), QString(wintitle),
					      QString(text), QString(oktxt),
					      QString(ntmsg),
					      QString::null, 0, 1 );

    endCmdRecEvent( refnr, res, oktxt, ntmsg );
    return !res;
}
