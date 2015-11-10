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
#include "separstr.h"

#include "uibody.h"
#include "uiicon.h"
#include "uimain.h"
#include "uimainwin.h"
#include "uiobj.h"
#include "uiparentbody.h"
#include "uipixmap.h"
#include "uibutton.h"
#include "uistatusbar.h"
#include "uistring.h"
#include "uistrings.h"

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


bool uiMsg::toStatusbar( uiString msg, int fldidx, int msec )
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
    gtCaptn() = uiStrings::sEmptyString();

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


void uiMsg::endCmdRecEvent( int refnr, int buttonnr, const char* buttxt0,
			    const char* buttxt1, const char* buttxt2 )
{
    BufferString msg( "QMsgBoxBut " );

    msg += buttonnr==0 ? buttxt0 : ( buttonnr==1 ? buttxt1 : buttxt2 );

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
    mb->setWindowTitle( title.getQString() );
    mb->setText( txt.getQString() );
    addStayOnTopFlag( *mb );

    const bool withics = uiButton::haveCommonPBIcons();
    QIcon qicon; // null icon to avoid icons on the pushbuttons

    if ( !yestxt.isEmpty() )
    {
	QPushButton* yesbut = mb->addButton( QMessageBox::Yes );
	yesbut->setText(yestxt.getQString() );
	if ( withics )
	{
	    uiIcon icn( "ok" );
	    qicon = icn.qicon();
	}
	yesbut->setIcon( qicon );
    }

    if ( !notxt.isEmpty() )
    {
	QPushButton* nobut = mb->addButton( QMessageBox::No );
	nobut->setText( notxt.getQString() );
	if ( withics )
	{
	    uiIcon icn( "stop" );
	    qicon = icn.qicon();
	}
	nobut->setIcon( qicon );
    }

    if ( !cncltxt.isEmpty() )
    {
	QPushButton* rejbut = mb->addButton( QMessageBox::Abort );
	rejbut->setText( cncltxt.getQString() );
	if ( withics )
	{
	    uiIcon icn( "cancel" );
	    qicon = icn.qicon();
	}
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

    mCapt( title.isEmpty() ? uiStrings::sSpecify() : title );
    const int refnr = beginCmdRecEvent( utfwintitle );

    PtrMan<QMessageBox> mb = createMessageBox( icon, parent, txt, yestxt,
					       notxt, cncltxt, wintitle );
    const int res = mb->exec();

    const int retval = res==QMessageBox::Yes ? 1 :
		       res==QMessageBox::No  ? 0 : -1;

    endCmdRecEvent( refnr, 1-retval, yestxt.getOriginalString(),
		    notxt.getOriginalString(), cncltxt.getOriginalString() );

    return retval;
}


void uiMsg::message( const uiString& part1, const uiString& part2,
		     const uiString& part3 )
{
    uiString msg = part1;
    if ( !part2.isEmpty() ) msg.append( part2 );
    if ( !part3.isEmpty() ) msg.append( part3 );
    showMessageBox( Information, popParnt(), msg, uiStrings::sOk(),
		    uiString::emptyString(), uiString::emptyString(),
		    tr("Information") );
}


void uiMsg::warning( const uiString& part1, const uiString& part2,
		     const uiString& part3 )
{
    uiString msg = part1;
    if ( !part2.isEmpty() ) msg.append( part2 );
    if ( !part3.isEmpty() ) msg.append( part3 );
    showMessageBox( Warning, popParnt(), msg, uiStrings::sOk(),
		    uiString::emptyString(), uiString::emptyString(),
		    tr("Warning") );
}


void uiMsg::error( const uiString& part1, const uiString& part2,
		   const uiString& part3 )
{
    uiString msg = part1;
    if ( !part2.isEmpty() ) msg.append( part2 );
    if ( !part3.isEmpty() ) msg.append( part3 );
    showMessageBox( Critical, popParnt(), msg, uiStrings::sOk(),
		    uiString::emptyString(), uiString::emptyString(),
		    tr("Error") );
}


void uiMsg::errorWithDetails( const uiStringSet& bss,
			      const uiString& before )
{
    uiStringSet strings;
    if ( !before.isEmpty() )
	strings += before;

    strings.append( bss );
    errorWithDetails( strings );
}


void uiMsg::errorWithDetails( const BufferStringSet& bss )
{
    uiStringSet strings;
    bss.fill( strings );
    errorWithDetails( strings );
}


void uiMsg::errorWithDetails( const FileMultiString& fms )
{
    uiStringSet strings;
    for ( int idx=0; idx<fms.size(); idx++ )
	strings.add( mToUiStringTodo(fms[idx]) );

    errorWithDetails( strings );
}


void uiMsg::errorWithDetails( const uiStringSet& strings )
{
    if ( !strings.size() )
	return;

    mPrepCursor();
    const uiString oktxt = uiStrings::sOk();
    mCapt( tr("Error") );
    const int refnr = beginCmdRecEvent( utfwintitle );
    // Use of QMessageBox::Abort enables close and escape actions by the user
    PtrMan<QMessageBox> mb = createMessageBox( Critical, popParnt(), strings[0],
					       uiString::emptyString(),
					       uiString::emptyString(),
					       oktxt, wintitle );
    mb->setDefaultButton( QMessageBox::Abort );

    if ( strings.size()>1 )
    {
	uiString detailed = strings[0];

	for ( int idx=1; idx<strings.size(); idx++ )
	{
	    uiString old = detailed;
	    detailed = tr( "%1\n%2" ).arg( old ).arg( strings[idx] );
	}

	mb->setDetailedText( detailed.getQString() );
    }

    mb->exec();
    endCmdRecEvent( refnr, 0, oktxt.getOriginalString() );
}


int uiMsg::askSave( const uiString& text, bool wcancel )
{
    const uiString dontsavetxt = tr("Don't save");
    return question( text, uiStrings::sSave(), dontsavetxt,
		     wcancel ? uiStrings::sCancel() : uiString::emptyString(),
		     tr("Data not saved") );
}


int uiMsg::askRemove( const uiString& text, bool wcancel )
{
    const uiString notxt = wcancel ? tr("Don't remove") : uiStrings::sCancel();
    return question( text, uiStrings::sRemove(), notxt,
		     wcancel ? uiStrings::sCancel() : uiString::emptyString(),
		     tr("Remove data") );
}


int uiMsg::askContinue( const uiString& text )
{
    return question( text, uiStrings::sContinue(), uiStrings::sAbort(),
		     uiString::emptyString() );
}


int uiMsg::askOverwrite( const uiString& text )
{
    const uiString yestxt = uiStrings::sOverwrite();
    return question( text, yestxt, uiStrings::sCancel(),
		     uiString::emptyString() );
}


int uiMsg::ask2D3D( const uiString& text, bool wcancel )
{
    mPrepCursor();

    mCapt( uiStrings::sSpecify() );
    const int refnr = beginCmdRecEvent( utfwintitle );

    uiString yestxt = uiStrings::s2D();
    uiString notxt = uiStrings::s3D();
    uiString cncltxt =
	wcancel ? uiStrings::sCancel() : uiStrings::sEmptyString();
    PtrMan<QMessageBox> mb = createMessageBox( Question, popParnt(), text,
	yestxt, notxt, cncltxt, wintitle );
    mb->button(QMessageBox::Yes  )->setIcon( QIcon() );
    mb->button(QMessageBox::No	)->setIcon( QIcon() );
    const int res = mb->exec();

    const int retval = res==QMessageBox::Yes ? 1 :
		       res==QMessageBox::No  ? 0 : -1;

    endCmdRecEvent( refnr, 1-retval, yestxt.getOriginalString(),
		    notxt.getOriginalString(), cncltxt.getOriginalString() );

    return retval;
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
    const uiString oktxt = uiStrings::sOk();
    mCapt( tr("About") );
    const int refnr = beginCmdRecEvent( utfwintitle );
    PtrMan<QMessageBox> mb = createMessageBox( NoIcon, popParnt(), text,
					       oktxt, uiString::emptyString(),
					       uiString::emptyString(),
					       wintitle );

    mb->setIconPixmap( mb->windowIcon().pixmap(32) );
    mb->exec();
    endCmdRecEvent( refnr, 0, oktxt.getOriginalString() );
}


void uiMsg::aboutOpendTect( const uiString& text )
{
    mPrepCursor();
    mCapt( tr("About OpendTect") );
    const uiString oktxt = uiStrings::sClose();
    const int refnr = beginCmdRecEvent( utfwintitle );
    PtrMan<QMessageBox> mb = createMessageBox( NoIcon, popParnt(), text,
					       oktxt, uiString::emptyString(),
					       uiString::emptyString(),
					       wintitle );
    uiPixmap pm( sODLogo );
    if ( pm.qpixmap() )
	mb->setIconPixmap( *pm.qpixmap() );

    mb->setBaseSize( 600, 300 );
    mb->exec();
    endCmdRecEvent( refnr, 0, oktxt.getOriginalString() );
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


bool uiMsg::showMsgNextTime( const uiString& text, const uiString& notmsginp )
{
    mPrepCursor();
    const uiString oktxt = uiStrings::sOk();
    const uiString notxt = notmsginp.isEmpty() ?
			   tr("Don't show this message again") : notmsginp;
    const uiString cncltxt = uiStrings::sCancel();
    mCapt( tr("Information") );
    const int refnr = beginCmdRecEvent( utfwintitle );

    PtrMan<QMessageBox> mb = createMessageBox( Information, popParnt(), text,
					       oktxt, notxt, cncltxt, wintitle);
    // Only CmdDriver triggers hidden no-button, since it can't access checkbox.
    mb->button( QMessageBox::No    )->setVisible( false );
    // Make close/escape map onto hidden abort-button to reject checkbox state.
    mb->button( QMessageBox::Abort )->setVisible( false );

    QCheckBox* cb = new QCheckBox();
    cb->setText( notxt.getQString() );
    mDynamicCastGet(QGridLayout*,grid,mb->layout())
    if ( grid )
	grid->addWidget( cb, grid->rowCount(), 0, 1, grid->columnCount() );

    const int res = mb->exec();
    bool checked = cb->isChecked() && res!=QMessageBox::Abort;
    checked = checked || res==QMessageBox::No;

    endCmdRecEvent( refnr, checked ? 1 : 0, oktxt.getOriginalString(),
		    notxt.getOriginalString() );
    return !checked;
}

