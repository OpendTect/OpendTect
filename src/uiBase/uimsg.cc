/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          26/04/2000
________________________________________________________________________

-*/


#include "uimsg.h"

#include "bufstringset.h"
#include "filepath.h"
#include "mousecursor.h"
#include "oddirs.h"
#include "perthreadrepos.h"
#include "separstr.h"
#include "task.h"
#include "survinfo.h"

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

#include "q_uiimpl.h"

#undef Ok
#include <QCheckBox>
#include <QLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>

#define sError() uiStrings::sError()
#define sWarning() uiStrings::sWarning()

mUseQtnamespace

class ODMessageBox : public QMessageBox
{
public:
ODMessageBox( QWidget* parent )
    : QMessageBox(parent)
{
}

protected:
bool event( QEvent* event )
{
    const bool res = QMessageBox::event( event );
    QTextEdit* textedit = findChild<QTextEdit*>();
    if ( textedit && textedit->isVisible() )
    {
	const QSizePolicy::Policy szpol = QSizePolicy::Expanding;
	const int maxsz = QWIDGETSIZE_MAX;
	textedit->setMaximumSize( maxsz, maxsz );
	textedit->setSizePolicy( szpol, szpol );
	setMaximumSize( maxsz, maxsz );
	setSizePolicy( szpol, szpol );
    }

    return res;
}

};


uiMsg& gUiMsg( const uiParent* p )
{
    static uiMsg* theinst_ = 0;

    if ( !theinst_ )
	theinst_ = new uiMsg;

    theinst_->setMainWin( p ? const_cast<uiParent*>(p)->mainwin() : 0 );

    return *theinst_;
}


uiUserShowWait::uiUserShowWait( const uiParent* p, const uiString& msg,
				int fldidx )
    : mcc_(0)
    , parent_(p)
    , fldidx_(fldidx)
{
    uiMainWin* mw = 0;
    if ( p )
	mw = const_cast<uiParent*>(p)->mainwin();
    if ( !mw || !mw->statusBar() )
	mw = uiMainWin::activeWindow();
    if ( !mw || !mw->statusBar() )
	mw = uiMain::theMain().topLevel();
    sb_ = mw ? mw->statusBar() : 0;
    if ( sb_ )
	sb_->getMessages( prevmessages_ );

    setMessage( msg );
}


uiUserShowWait::~uiUserShowWait()
{
    readyNow();
    if ( sb_ )
	sb_->message( prevmessages_ );
}


void uiUserShowWait::setMessage( const uiString& msg )
{
    if ( !mcc_ )
	mcc_ = new MouseCursorChanger( MouseCursor::Wait );
    if ( sb_ )
	sb_->message( msg.isEmpty() ? msg : m3Dots(msg), fldidx_ );
}


void uiUserShowWait::readyNow()
{
    if ( !mcc_ )
	return;

    setMessage( uiString::empty() );
    mcc_->restore();
    deleteAndZeroPtr( mcc_ );
}


bool uiUSWTaskRunner::execute( Task& t )
{
    return (execres_ = t.execute());
}


void uiUSWTaskRunner::emitErrorMessage( const uiString& msg, bool wrn ) const
{
    if ( wrn )
	gUiMsg(usw_.parent()).warning( msg );
    else
	gUiMsg(usw_.parent()).error( msg );
}


TaskRunner& uiUSWTaskRunnerProvider::runner() const
{
    if ( !runner_ )
	runner_ = new uiUSWTaskRunner( parent_, msg_, sbfld_ );
    return *runner_;
}


void uiUSWTaskRunnerProvider::emitErrorMessage( const uiString& msg,
					        bool wrn ) const
{
    if ( wrn )
	gUiMsg(parent_).warning( msg );
    else
	gUiMsg(parent_).error( msg );
}


uiMsg::uiMsg()
	: uimainwin_(0)
	, lock_(true)
	, msgdisplock_(true)
{
}


uiMainWin* uiMsg::setMainWin( uiMainWin* m )
{
    uiMainWin* old = uimainwin_;
    uimainwin_ = m;
    return old;
}


QWidget* uiMsg::popParnt()
{
    uiMainWin* mw = uimainwin_; //Always respect user's setting first.
    if ( !mw ) mw = uiMainWin::activeWindow();
    if ( !mw ) mw = uiMain::theMain().topLevel();

    if ( !mw  )		return 0;
    return mw->body()->qwidget();
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
    gtCaptn() = uiString::empty();

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
    return carrier->beginCmdRecEvent( (od_int64) this, msg );
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
	carrier->endCmdRecEvent( (od_uint32)this, refnr, msg );
#else
	carrier->endCmdRecEvent( (od_int64)this, refnr, msg );
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
	const uiString& cncltxt, const uiString& title,
        QCheckBox** notagain )
{

    ODMessageBox* mb = new ODMessageBox( parent );
    mb->setIcon( (QMessageBox::Icon)icon );
    mb->setWindowTitle( toQString(title) );
    mb->setText( toQString(txt) );
    addStayOnTopFlag( *mb );

    QIcon qicon; // null icon to avoid icons on the pushbuttons

    if ( !yestxt.isEmpty() )
    {
	QPushButton* yesbut = mb->addButton( QMessageBox::Yes );
	yesbut->setText( toQString(yestxt) );
	uiIcon icn( "ok" );
	qicon = icn.qicon();
	yesbut->setIcon( qicon );
    }

    if ( !notxt.isEmpty() )
    {
	QPushButton* nobut = mb->addButton( QMessageBox::No );
	nobut->setText( toQString(notxt) );
	uiIcon icn( "stop" );
	qicon = icn.qicon();
	nobut->setIcon( qicon );
    }

    if ( !cncltxt.isEmpty() )
    {
	QPushButton* rejbut = mb->addButton( QMessageBox::Abort );
	rejbut->setText( toQString(cncltxt) );
	uiIcon icn( "cancel" );
	qicon = icn.qicon();
	rejbut->setIcon( qicon );
    }

    if ( notagain && mb->layout() )
    {
	*notagain = new QCheckBox();
	(*notagain)->setText( toQString(uiMsg::sDontShowAgain()) );
	mDynamicCastGet(QGridLayout*,grid,mb->layout())
	if ( grid )
	    grid->addWidget( *notagain, grid->rowCount(), 0, 1,
		             grid->columnCount() );
	else
	    mb->layout()->addWidget( *notagain );
    }

    return mb;
}

int uiMsg::showMessageBox( Icon icon, QWidget* parent, const uiString& txt,
			   const uiString& yestxt, const uiString& notxt,
			   const uiString& cncltxt, const uiString& title,
			   bool* notagain )
{
    if ( txt.isEmpty() )
	{ pErrMsg("Empty message to display. Refused."); return -1; }
    mPrepCursor();

    mCapt( title.isEmpty() ? uiStrings::sSpecify() : title );
    const int refnr = beginCmdRecEvent( utfwintitle );

    QCheckBox* checkbox = 0;
    PtrMan<QMessageBox> mb = createMessageBox( icon, parent, txt, yestxt,
					       notxt, cncltxt, wintitle,
					       notagain ? &checkbox : 0 );
    if ( checkbox )
	checkbox->setChecked( *notagain );

    int retval = 0;
    while ( true )
    {
	const int res = mb->exec();

	//Capture if the checkbox was clicked. As signals are blocked
	//this should normally not happen
	if ( checkbox && mb->clickedButton()==checkbox )
	    continue;

	if ( notagain && checkbox )
	    *notagain = checkbox->isChecked();

	retval = res==QMessageBox::Yes ? 1 :
	         res==QMessageBox::No  ? 0 : -1;
	break;
    }

    endCmdRecEvent( refnr, 1-retval, yestxt.getOriginalString(),
		    notxt.getOriginalString(), cncltxt.getOriginalString() );
    return retval;
}


bool uiMsg::message( const uiString& part1, const uiString& part2,
		     const uiString& part3, bool withdontshowagain )
{
    uiString msg = part1;
    if ( !part2.isEmpty() )
	msg.appendPhrase( part2 );
    if ( !part3.isEmpty() )
	msg.appendPhrase( part3 );
    bool notagain = false;
    showMessageBox( Information, popParnt(), msg, uiStrings::sOk(),
		    uiString::empty(), uiString::empty(),
		    uiStrings::sInformation(),
		    withdontshowagain ? &notagain : 0 );
    return notagain;
}


void uiMsg::info( const uiStringSet& strs )
{
    message( strs.cat() );
}

bool uiMsg::warning( const uiString& part1, const uiString& part2,
		     const uiString& part3, bool withdontshowagain )
{
    uiString msg = part1;
    if ( !part2.isEmpty() )
	msg.appendPhrase( part2 );
    if ( !part3.isEmpty() )
	msg.appendPhrase( part3 );
    bool notagain = false;
    showMessageBox( Warning, popParnt(), msg, uiStrings::sOk(),
		    uiString::empty(), uiString::empty(),
		    sWarning(), withdontshowagain ? &notagain : 0 );
    return notagain;
}


void uiMsg::handleWarnings( const uiRetVal& rv )
{
    if ( rv.isError() && !isCancelled(rv) && !isFinished(rv) )
	warning( rv );
}


void uiMsg::warning( const uiRetVal& rv )
{
    if ( rv.isOK() )
	{ pErrMsg("No warning. If expected, consider handleWarnings"); return; }
    else if ( isCancelled(rv) || isFinished(rv) )
	{ pErrMsg("Check on isCancelled and/or isFinished"); return; }

    uiString msg;
    if ( rv.isMultiMessage() )
	msg = rv.messages().cat();
    else if ( rv.isError() )
	msg = rv;
    uiStringSet	uistrset; uistrset += msg;
    showMsg( uimainwin_, WarningMsg, uistrset );
}


void uiMsg::handleErrors( const uiRetVal& rv )
{
    if ( rv.isError() && !isCancelled(rv) && !isFinished(rv) )
	error( rv );
}


void uiMsg::error( const uiRetVal& rv )
{
    if ( rv.isOK() )
	{ pErrMsg("No error. If expected, consider handleErrors"); return; }
    else if ( isCancelled(rv) || isFinished(rv) )
	{ pErrMsg("Check on isCancelled and/or isFinished"); return; }

    if ( rv.isMultiMessage() )
	errorWithDetails( rv.messages() );
    else if ( rv.isError() )
    {
	uiString msg( rv );
	error( msg );
    }
}


bool uiMsg::error( const uiString& part1, const uiString& part2,
		   const uiString& part3, bool withdontshowagain )
{
    uiString msg = part1;
    if ( !part2.isEmpty() )
	msg.appendPhrase( part2 );
    if ( !part3.isEmpty() )
	msg.appendPhrase( part3 );
    bool notagain = false;
    showMessageBox( Critical, popParnt(), msg, uiStrings::sOk(),
		    uiString::empty(), uiString::empty(),
		    sError(), withdontshowagain ? &notagain : 0 );

    return notagain;
}


void uiMsg::errorWithDetails( const uiStringSet& bss,
			      const uiString& before )
{
    uiStringSet strings;
    if ( !before.isEmpty() )
	strings += before;

    strings.append( bss );
    showMsg( uimainwin_, ErrorWithDetails,  strings );
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
	strings.add( ::toUiString(fms[idx]) );

    errorWithDetails( strings );
}


void uiMsg::errorWithDetails( const uiStringSet& strings )
{
    if ( !strings.size() )
	return;

    mPrepCursor();
    showMsg( uimainwin_, ErrorWithDetails,  strings );
}


void uiMsg::errorWithDetailProc( uiStringSet& strings )
{
    mCapt( sError() );
    const uiString oktxt = uiStrings::sOk();
    const int refnr = beginCmdRecEvent( utfwintitle );
    // Use of QMessageBox::Abort enables close and escape actions by the user
    PtrMan<QMessageBox> mb = createMessageBox( Critical, popParnt(), strings[0],
					       uiString::empty(),
					       uiString::empty(),
					       oktxt, wintitle, 0 );
    mb->setDefaultButton( QMessageBox::Abort );
    mb->setSizeGripEnabled( true );

    if ( strings.size()>1 )
    {
	uiString detailed = strings[0];
	for ( int idx=1; idx<strings.size(); idx++ )
	    detailed.appendPhrase( strings[idx] );
	mb->setDetailedText( toQString(detailed) );
    }

    mb->exec();
    endCmdRecEvent( refnr, 0, oktxt.getOriginalString() );
}


int uiMsg::askSave( const uiString& text, bool wcancel )
{
    const uiString dontsavetxt = tr("Don't save");
    return question( text, uiStrings::sSave(), dontsavetxt,
		     wcancel ? uiStrings::sCancel() : uiString::empty(),
		     tr("Data not saved") );
}


int uiMsg::askRemove( const uiString& text, bool wcancel )
{
    const uiString notxt = wcancel ? tr("Don't remove") : uiStrings::sCancel();
    return question( text, uiStrings::sRemove(), notxt,
		     wcancel ? uiStrings::sCancel() : uiString::empty(),
		     tr("Remove data") );
}


bool uiMsg::askContinue( const uiString& text )
{
    return question( text, uiStrings::sContinue(), uiStrings::sAbort(),
		     uiString::empty() ) != 0;
}


bool uiMsg::askOverwrite( const uiString& text )
{
    const uiString yestxt = uiStrings::sOverwrite();
    return question( text, yestxt, uiStrings::sCancel(),
		     uiString::empty() ) != 0;
}


int uiMsg::ask2D3D( const uiString& text, bool wcancel )
{
    if ( !SI().has2D() )
	return 0;
    if ( !SI().has3D() )
	return 1;

    mPrepCursor();

    mCapt( uiStrings::sSpecify() );
    const int refnr = beginCmdRecEvent( utfwintitle );

    uiString yestxt = uiStrings::s2D();
    uiString notxt = uiStrings::s3D();
    uiString cncltxt =
	wcancel ? uiStrings::sCancel() : uiString::empty();
    PtrMan<QMessageBox> mb = createMessageBox( Question, popParnt(), text,
	yestxt, notxt, cncltxt, wintitle, 0 );
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
		     const uiString& cncltxtinp, const uiString& title,
		     bool* notagain )
{
    const uiString yestxt = yestxtinp.isEmpty() ? uiStrings::sYes() : yestxtinp;
    const uiString notxt = notxtinp.isEmpty() ? uiStrings::sNo() : notxtinp;
    return showMessageBox(
	Question, popParnt(), text, yestxt, notxt, cncltxtinp,
	title, notagain );
}


void uiMsg::about( const uiString& text )
{
    mPrepCursor();
    const uiString oktxt = uiStrings::sOk();
    mCapt( uiStrings::sAbout() );
    const int refnr = beginCmdRecEvent( utfwintitle );
    PtrMan<QMessageBox> mb = createMessageBox( NoIcon, popParnt(), text,
					       oktxt, uiString::empty(),
					       uiString::empty(),
					       wintitle, 0 );

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
					       oktxt, uiString::empty(),
					       uiString::empty(),
					       wintitle, 0 );
    File::Path fp( GetSoftwareDir(true), "data", "od.png" );
    uiPixmap pm( fp.fullPath() );
    if ( pm.qpixmap() )
	mb->setIconPixmap( *pm.qpixmap() );

    mb->setBaseSize( 600, 300 );
    mb->exec();
    endCmdRecEvent( refnr, 0, oktxt.getOriginalString() );
}


bool uiMsg::askGoOn( const uiString& text, bool yn, bool* notagain )
{
    const uiString oktxt = yn ? uiStrings::sYes() : uiStrings::sOk();
    const uiString canceltxt = yn ? uiStrings::sNo() : uiStrings::sCancel();
    return askGoOn( text, oktxt, canceltxt, notagain );
}


bool uiMsg::askGoOn( const uiString& text, const uiString& textyes,
		     const uiString& textno, bool* notagain )
{
    return question( text, textyes, textno,
	             uiString::empty(),
		     uiString::empty(), notagain );
}


int uiMsg::askGoOnAfter( const uiString& text, const uiString& cnclmsginp ,
			 const uiString& textyesinp, const uiString& textnoinp,
			 bool* notagain	)
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
    return showMessageBox( Warning, popParnt(), text, yestxt, notxt, cncltxt,
			   uiString::empty(), notagain );
}


uiString uiMsg::sDontShowAgain()
{ return tr("Don't show this message again"); }


void uiMsg::showMsg( uiMainWin* p, msgType msgtyp, const uiStringSet& strset )
{
    Threads::Locker lckr( lock_ );
    CBCapsule<uiStringSet> caps( strset, this );
    if ( p ) uimainwin_ = p;
    if ( msgtyp == ErrorWithDetails || msgtyp == ErrorMsg )
	dispErrMsgCB( &caps );
    else if ( msgtyp == WarningMsg )
	dispWarnMsgCB( &caps );
}


void uiMsg::dispErrMsgCB( CallBacker* cber )
{
    Threads::Locker lckr( msgdisplock_ );
    mDynamicCastGet( CBCapsule<uiStringSet>*, caps, cber )
    mEnsureExecutedInMainThreadWithCapsule( uiMsg::dispErrMsgCB, caps );
    mCBCapsuleUnpack( uiStringSet, uistrset, caps );
    mCapt( sError() );
    errorWithDetailProc(uistrset);
}


void uiMsg::dispWarnMsgCB( CallBacker* cber )
{
    Threads::Locker lckr( msgdisplock_ );
    mDynamicCastGet( CBCapsule<uiStringSet>*, caps, cber )
    mEnsureExecutedInMainThreadWithCapsule( uiMsg::dispWarnMsgCB, caps );
    mCBCapsuleUnpack( uiStringSet, uistrset, caps );
    showMessageBox( Warning, popParnt(), uistrset[0], uiStrings::sOk(),
		    uiString::empty(), uiString::empty(), sWarning() );
}
