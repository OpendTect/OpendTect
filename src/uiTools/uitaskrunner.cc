/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink/Bert
 Date:          Aug 2000/Oct 2001
________________________________________________________________________

-*/

#include "uitaskrunner.h"

#include "mousecursor.h"

#include "uibutton.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiprogressbar.h"
#include "uistatusbar.h"

#include "fixedstring.h"
#include "thread.h"
#include "threadlock.h"
#include "timer.h"

#include <math.h>

static const char* noprogbardispsymbs[] =
    {	"|||||              ", " |||||             ", "  |||||            ",
	"   |||||           ", "    |||||          ", "     |||||         ",
	"      |||||        ", "       |||||       ", "        |||||      ",
	"         |||||     ", "          |||||    ", "           |||||   ",
	"            |||||  ", "             ||||| ", "              |||||",
	"             ||||| ", "            |||||  ", "           |||||   ",
	"          |||||    ", "         |||||     ", "        |||||      ",
	"       |||||       ", "      |||||        ", "     |||||         ",
	"    |||||          ", "   |||||           ", "  |||||            ",
	" |||||             ", "|||||              " };

#ifndef __debug__
static const int noprogbardispnrsymbs = 29;
#endif



/*!If there is a main window up, we should always use that window as parent.
   Only if main window does not exist, use the provided parent. */

static uiParent* getTRParent( const uiParent* p )
{
    uiParent* res = uiMainWin::activeWindow();
    if ( res )
        return res;

    return const_cast<uiParent*>( p );
}



uiTaskRunnerProvider::uiTaskRunnerProvider( const uiParent* p )
    : parent_(const_cast<uiParent*>(p))
{
}


TaskRunner& uiTaskRunnerProvider::runner() const
{
    if ( !runner_ )
	runner_ = new uiTaskRunner( getTRParent(parent_) );
    return *runner_;
}


void uiTaskRunnerProvider::emitErrorMessage( const uiString& msg,
					     bool wrn ) const
{
    if ( wrn )
	gUiMsg(parent_).warning( msg );
    else
	gUiMsg(parent_).error( msg );
}


uiTaskRunner::uiTaskRunner( const uiParent* prnt, bool dispmsgonerr )
    : uiDialog( getTRParent(prnt),
                uiDialog::Setup(tr("Executing"),mNoDlgTitle,mNoHelpKey)
	.nrstatusflds( -1 )
	.oktext(uiStrings::sPause().appendPlainText("   "))
	.canceltext(uiStrings::sAbort()) )
    , task_(nullptr)
    , thread_(nullptr)
    , tim_(*new Timer("") )
    , execnm_("")
    , statelock_(true)
    , dispinfolock_(false)
    , uitaskrunnerthreadlock_(false)
    , dispmsgonerr_( dispmsgonerr )
    , symbidx_( 0 )
{
    proglbl_ = new uiLabel( this, toUiString(noprogbardispsymbs[0]) );
    proglbl_->attach( hCentered );
#ifdef __debug__
    proglbl_->setPrefWidthInChar( 20 );
    proglbl_->setAlignment( OD::Alignment::Left );
#endif

    progbar_ = new uiProgressBar( this, "ProgressBar", 0, 0 );
    progbar_->setPrefWidthInChar( 50 );

    tim_.tick.notify( mCB( this, uiTaskRunner, timerTick ) );
    postFinalise().notify( mCB( this, uiTaskRunner, onFinalise ) );

    statusBar()->addMsgFld( tr("Current activity"), OD::Alignment::Left, 2 );
    statusBar()->addMsgFld( tr("Counted items"), OD::Alignment::Right, 2 );
    statusBar()->addMsgFld( tr("Number done"), OD::Alignment::Left, 1 );

    setForceFinalise( true );
}


uiTaskRunner::~uiTaskRunner()
{
    if ( thread_ )
	{ thread_->waitForFinish(); deleteAndZeroPtr(thread_); }
    delete &tim_;
}


bool uiTaskRunner::execute( Task& t )
{
    if ( task_ )
	return (execres_ = t.execute());

    MouseCursorChanger mousecursor( MouseCursor::Arrow );

    task_ = &t; state_ = 1;
    prevtotalnr_ = prevnrdone_ = prevpercentage_ = -1;
    prevmessage_ = uiString::empty();
    if ( statusBar() )
	statusBar()->message( prevmessage_, 0 );
    prevnrdonetext_ = prevmessage_;
    execnm_ = task_->name();

    updateFields();
    return (execres_ = go());
}


void uiTaskRunner::onFinalise( CallBacker* )
{
    button(OK)->setIcon( "pause" );
    button(CANCEL)->setIcon( "stop" );

    tim_.start( 100, true );
    Threads::Locker lckr( uitaskrunnerthreadlock_ );
    BufferString nm( "Task: ", task_ ? task_->name() : "<none>" );
    thread_ = new Threads::Thread( mCB(this,uiTaskRunner,doWork), nm );
}


void uiTaskRunner::emitErrorMessage( const uiString& msg, bool wrn ) const
{
    if ( wrn )
	uiMSG().warning( msg );
    else
	uiMSG().error( msg );
}


void uiTaskRunner::doWork( CallBacker* )
{
    task_->enableWorkControl( true );
    bool res = task_->execute();

    Threads::Locker lckr( statelock_ );
    state_ = res ? 0 : -1;
}


void uiTaskRunner::updateFields()
{
    if ( !task_ ) return;

    uiStatusBar& sb = *statusBar();

    Threads::Locker lckr( dispinfolock_ );
    const int totalnr = mCast( int, task_->totalNr() );
    const int nrdone = mCast( int, task_->nrDone() );
    const uiString nrdonetext = task_->nrDoneText();
#ifdef __debug__
    if ( nrdonetext.getString() == "Nr Done" )
	{ pErrMsg("Task executed in UI needs valid nrDoneText"); }
#endif
    const uiString message = task_->message();

    if ( nrdone < 0 )
    {
	setCaption( toUiString(execnm_) );
	return;
    }

    if ( prevmessage_.getString() != message.getString() )
    {
	sb.message( message, 0 );
	prevmessage_ = message;
    }
    if ( prevnrdonetext_.getString() != nrdonetext.getString() )
    {
	sb.message( nrdonetext, 1 );
	prevnrdonetext_ = nrdonetext;
    }

    const bool nrdonechg = prevnrdone_ != nrdone;
    if ( nrdonechg )
    {
	prevnrdone_ = nrdone;
	uiString str = toUiString( nrdone );
	sb.message( str, 2 );
    }

    if ( totalnr > 0 )
    {
	if ( totalnr != prevtotalnr_ )
	{
	    progbar_->setTotalSteps( totalnr );
	    prevtotalnr_ = totalnr;
	}

	if ( nrdonechg )
	    progbar_->setProgress( nrdone );

	const float fpercentage = 100.f * ((float)nrdone) / totalnr;
	int percentage = (int)fpercentage;
	if ( percentage > 100 ) percentage = 100;
	if ( percentage!=prevpercentage_ )
	{
	    uiString capt = tr("[%1%] %2").arg(percentage)
					  .arg(toUiString(execnm_));
	    setCaption( capt );

	    prevpercentage_ = percentage;
	}
    }

    const bool disppb = totalnr > 0;
    if ( !disppb )
    {
	symbidx_++;
#ifdef __debug__
	//TODO: solve in uiLabel:
	// if you leave out the spaces at the end, number will be truncated
	proglbl_->setText( tr("[dbg] step: %1     ").arg(symbidx_) );
#else
	if ( symbidx_ >= noprogbardispnrsymbs ) symbidx_ = 0;
	proglbl_->setText( toUiString(noprogbardispsymbs[symbidx_]) );
#endif
    }
    proglbl_->display( !disppb );
    progbar_->display( disppb );
}


bool uiTaskRunner::acceptOK()
{
    Threads::Locker lckr( uitaskrunnerthreadlock_ );
    if ( !task_ ) return false;
    Task::Control state = task_->getState();
    lckr.unlockNow();

    if ( state==Task::Pause )
    {
	task_->controlWork( Task::Run );
	setOkText(uiStrings::sPause() );
	button(OK)->setIcon( "pause" );
    }
    else if ( state==Task::Run )
    {
	task_->controlWork( Task::Pause );
	setOkText(uiStrings::sResume() );
	button(OK)->setIcon( "resume" );
    }

    return false;
}


void uiTaskRunner::timerTick( CallBacker* )
{
    Threads::Locker stlckr( statelock_ );
    const int state = state_;
    stlckr.unlockNow();

    if ( state<1 )
    {
	uiString message;
	uiRetVal errdetails;

	Threads::Locker trlckr( uitaskrunnerthreadlock_,
				Threads::Locker::DontWaitForLock );
	if ( trlckr.isLocked() )
	{
	    if ( task_ ) errdetails = task_->errorWithDetails();
	    message = finalizeTask();
	    trlckr.unlockNow();
	}

	if ( state<0 )
	{
	    errdetails_ = errdetails;
	    if ( dispmsgonerr_ )
		uiMSG().error( errdetails_ );
	}

	done( state<0 ? uiDialog::Rejected : uiDialog::Accepted );
	return;
    }

    updateFields();
    tim_.start( 250, true );
}


uiString uiTaskRunner::finalizeTask()
{
    uiString message;
    if ( thread_ )
    {
	thread_->waitForFinish();
	delete thread_;
	thread_ = 0;
    }

    if ( task_ ) message = task_->message();
    task_ = 0;

    return message;
}


bool uiTaskRunner::rejectOK()
{
    Threads::Locker lckr( uitaskrunnerthreadlock_ );
    if ( task_ ) task_->controlWork( Task::Stop );
    finalizeTask();
    lckr.unlockNow();

    state_ = (int)Task::Stop;
    execres_ = false;
    return true;
}
