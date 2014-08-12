/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          26/04/2000
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uimsg.h"

#include "bufstringset.h"
#include "mousecursor.h"
#include "oddirs.h"
#include "perthreadrepos.h"
#include "pixmap.h"
#include "separstr.h"

#include "uimain.h"
#include "uimainwin.h"
#include "uistatusbar.h"
#include "uiobj.h"
#include "uibody.h"
#include "uiparentbody.h"
#include "uistrings.h"
#include "uistring.h"

#undef Ok
#include <QCheckBox>
#include <QLayout>
#include <QMessageBox>
#include <QPushButton>

#include "odlogo128x128.xpm"
static const char** sODLogo = od_logo_128x128;

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


static uiString& gtCaptn()
{
    mDefineStaticLocalObject( PerThreadObjectRepository<uiString>, captn,  );
    return captn.getObject();
}


void uiMsg::setNextCaption( const uiString& s )
{
    gtCaptn() = s;
}


#define mPrepCursor() \
    MouseCursorChanger cc( MouseCursor::Arrow )

#define mCapt(s) \
BufferString addendum; \
const uiString wintitle = \
	getCaptn( uiMainWin::uniqueWinTitle((s),0,&addendum ) ); \
const BufferString utfwintitle( wintitle.getOriginalString(), addendum )


uiString getCaptn( const uiString& s )
{
    if ( gtCaptn().isEmpty() )
	return s;

    uiString oldcaptn = gtCaptn();
    gtCaptn() = "";

    return oldcaptn;
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
    BufferString msg; if ( abstrbut ) msg = abstrbut->text(); \
    endCmdRecEvent( refnr, 0, msg );

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


static void addStayOnTopFlag( QMessageBox& mb )
{
    Qt::WindowFlags flags = mb.windowFlags();
    flags |= Qt::WindowStaysOnTopHint;
    mb.setWindowFlags( flags );
}


static QMessageBox* createMessageBox( uiMsg::Icon icon, QWidget* parent,
	const uiString& txt, const uiString& yestxt, const uiString& notxt,
	const uiString& cncltxt, const uiString& title )
{

    QMessageBox* mb = new QMessageBox( parent );
    mb->setIcon( (QMessageBox::Icon)icon );
    mb->setWindowTitle( title.getQtString() );
    mb->setText( txt.getQtString() );
    addStayOnTopFlag( *mb );

    QIcon qicon; // null icon to avoid icons on the pushbuttons
    QPushButton* yesbut = mb->addButton( QMessageBox::Yes );
    yesbut->setText(yestxt.getQtString() );
    yesbut->setIcon( qicon );

    if ( !notxt.isEmpty() )
    {
	QPushButton* nobut = mb->addButton( QMessageBox::No );
	nobut->setText( notxt.getQtString() );
	nobut->setIcon( qicon );
    }

    if ( !cncltxt.isEmpty() )
    {
	QPushButton* rejbut = mb->addButton( QMessageBox::Abort );
	rejbut->setText( cncltxt.getQtString() );
	rejbut->setIcon( qicon );
    }

    return mb;
}


int uiMsg::showMessageBox( Icon icon, QWidget* parent, const uiString& txt,
			   const uiString& yestxt, const uiString& notxt,
			   const uiString& cncltxt, const uiString& title )
{
    mPrepCursor();
    if ( txt.isEmpty() )
	return -1;

    mCapt( title.isEmpty() ? tr("Please specify") : title );
    const int refnr = beginCmdRecEvent( utfwintitle );

    PtrMan<QMessageBox> mb = createMessageBox( icon, parent, txt, yestxt, notxt,
					       cncltxt, wintitle );
    const int res = mb->exec();

    endCmdRecEvent( refnr, res, yestxt.getOriginalString(),
		    notxt.getOriginalString(),
		    cncltxt.getOriginalString() );

    if ( res==QMessageBox::Yes )
	return 1;
    if ( res==QMessageBox::No )
	return 0;
    return -1;
}


void uiMsg::message( const uiString& part1, const uiString& part2,
		     const uiString& part3 )
{
    uiString msg = part1;
    if ( !part2.isEmpty() ) msg.append( part2 );
    if ( !part3.isEmpty() ) msg.append( part3 );
    showMessageBox( Information, popParnt(), msg, uiStrings::sOk(), 0, 0,
		    tr("Information") );
}


void uiMsg::warning( const uiString& part1, const uiString& part2,
		     const uiString& part3 )
{
    uiString msg = part1;
    if ( !part2.isEmpty() ) msg.append( part2 );
    if ( !part3.isEmpty() ) msg.append( part3 );
    showMessageBox( Warning, popParnt(), msg, uiStrings::sOk(), 0, 0,
		    tr("Warning") );
}


void uiMsg::error( const uiString& part1, const uiString& part2,
		   const uiString& part3 )
{
    uiString msg = part1;
    if ( !part2.isEmpty() ) msg.append( part2 );
    if ( !part3.isEmpty() ) msg.append( part3 );
    showMessageBox( Critical, popParnt(), msg, uiStrings::sOk(), 0, 0,
		    tr("Error") );
}


void uiMsg::errorWithDetails( const TypeSet<uiString>& bss,
			      const uiString& before )
{
    TypeSet<uiString> strings;
    if ( !before.isEmpty() )
	strings += before;

    strings.append( bss );
    errorWithDetails( strings );
}


void uiMsg::errorWithDetails( const BufferStringSet& bss )
{
    TypeSet<uiString> strings;
    bss.fill( strings );
    errorWithDetails( strings );
}


void uiMsg::errorWithDetails( const FileMultiString& fms )
{
    TypeSet<uiString> strings;
    for ( int idx=0; idx<fms.size(); idx++ )
	strings.add( fms[idx] );

    errorWithDetails( strings );
}


void uiMsg::errorWithDetails( const TypeSet<uiString>& strings )
{
    if ( !strings.size() )
	return;

    MouseCursorChanger cc( MouseCursor::Arrow );

    mCapt( tr("Error") );
    const int refnr = beginCmdRecEvent( utfwintitle.buf() );

    QMessageBox msgbox( QMessageBox::Critical, wintitle.getQtString(),
			strings[0].getQtString(), QMessageBox::Ok, popParnt() );
    if ( strings.size()>1 )
    {
	uiString detailed = strings[0];

	for ( int idx=1; idx<strings.size(); idx++ )
	{
	    uiString old = detailed;
	    detailed = uiString( "%1\n%2" ).arg( old ).arg( strings[idx] );
	}

	msgbox.setDetailedText( detailed.getQtString() );
    }

    addStayOnTopFlag( msgbox );
    msgbox.exec();
    mEndCmdRecEvent( refnr, msgbox );
}


int uiMsg::askSave( const uiString& text, bool wcancel )
{
    const uiString dontsavetxt = tr("Don't save");
    return question( text, uiStrings::sSave(true), dontsavetxt,
		     wcancel ? uiStrings::sCancel() : 0,
		     tr("Data not saved") );
}


int uiMsg::askRemove( const uiString& text, bool wcancel )
{
    const uiString notxt = wcancel ? tr("Don't remove") : uiStrings::sCancel();
    return question( text, uiStrings::sRemove(true), notxt,
		     wcancel ? uiStrings::sCancel() : uiString(0),
		     tr("Remove data") );
}


int uiMsg::askContinue( const uiString& text )
{
    return question( text, uiStrings::sContinue(), uiStrings::sAbort(), 0 );
}


int uiMsg::askOverwrite( const uiString& text )
{
    const uiString yestxt = uiStrings::sOverwrite();
    return question( text, yestxt, uiStrings::sCancel(), 0 );
}


int uiMsg::question( const uiString& text, const uiString& yestxtinp,
		     const uiString& notxtinp,
		     const uiString& cncltxtinp, const uiString& title )
{
    const uiString yestxt = yestxtinp.isEmpty() ? uiStrings::sYes() : yestxtinp;
    const uiString notxt = notxtinp.isEmpty() ? uiStrings::sNo() : notxtinp;
    return showMessageBox(
	Question, popParnt(), text, yestxt, notxt, cncltxtinp, title );
}


void uiMsg::about( const uiString& text )
{
    mPrepCursor();
    mCapt( tr("About") );
    const int refnr = beginCmdRecEvent( utfwintitle );
    QMessageBox::about( popParnt(), wintitle.getQtString(), text.getQtString());
    endCmdRecEvent( refnr, 0, uiStrings::sOk().getOriginalString() );
}


void uiMsg::aboutOpendTect( const uiString& text )
{
    mPrepCursor();
    mCapt( tr("About OpendTect") );
    const int refnr = beginCmdRecEvent( utfwintitle );
    QMessageBox msgbox( popParnt() );
    msgbox.addButton( QMessageBox::Close );
    ioPixmap pm( sODLogo );
    if ( pm.qpixmap() )
	msgbox.setIconPixmap( *pm.qpixmap() );
    msgbox.setWindowTitle( wintitle.getQtString() );
    msgbox.setText( text.getQtString() );
    msgbox.setBaseSize( 600, 300 );
    addStayOnTopFlag( msgbox );
    msgbox.exec();
    endCmdRecEvent( refnr, 0, uiStrings::sOk().getOriginalString() );
}


bool uiMsg::askGoOn( const uiString& text, bool yn )
{
    const uiString oktxt = yn ? uiStrings::sYes() : uiStrings::sOk();
    const uiString canceltxt = yn ? uiStrings::sNo() : uiStrings::sCancel();
    return askGoOn( text, oktxt, canceltxt );
}


bool uiMsg::askGoOn( const uiString& text, const uiString& textyes,
		     const uiString& textno )
{
    return question( text, textyes, textno );
}


int uiMsg::askGoOnAfter( const uiString& text, const uiString& cnclmsginp ,
			 const uiString& textyesinp, const uiString& textnoinp )
{
    const uiString yestxt = textyesinp.isEmpty()
	? uiStrings::sYes()
	: textyesinp;
    const uiString notxt = textnoinp.isEmpty()
	? uiStrings::sNo()
	: textnoinp;
    const uiString cncltxt = cnclmsginp.isEmpty()
	? uiStrings::sCancel()
	: cnclmsginp;
    return showMessageBox( Warning, popParnt(), text, yestxt, notxt, cncltxt );
}


bool uiMsg::showMsgNextTime( const uiString& text, const uiString& ntmsginp )
{
    mPrepCursor();
    const uiString oktxt = uiStrings::sOk();
    mCapt( tr("Information") );
    const int refnr = beginCmdRecEvent( utfwintitle );
    PtrMan<QMessageBox> mb = createMessageBox( Information, popParnt(),
					       text, oktxt, 0, 0,
					       wintitle );
    const uiString notmsg = ntmsginp.isEmpty()
	? tr("Don't show this message again")
	: ntmsginp;

    QCheckBox* cb = new QCheckBox();
    cb->setText( notmsg.getQtString() );
    mDynamicCastGet(QGridLayout*,grid,mb->layout())
    if ( grid )
	grid->addWidget( cb, grid->rowCount(), 0, 1, grid->columnCount() );

    int res = mb->exec();
    const bool checked = cb->isChecked(); // || res==1; TODO Jaap
    res = checked ? 1 : 0;
    endCmdRecEvent( refnr, res, oktxt.getOriginalString(),
		    notmsg.getOriginalString() );
    return !checked;
}
