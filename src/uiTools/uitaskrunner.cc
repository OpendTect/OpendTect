/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink/A.H. Bril
 Date:          Aug 2000/Oct 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uitaskrunner.h"

#include "mousecursor.h"
#include "uiprogressbar.h"
#include "uistatusbar.h"
#include "uilabel.h"

#include "timer.h"
#include "uimsg.h"
#include "thread.h"
#include "threadlock.h"
#include "timefun.h"

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
static const int noprogbardispnrsymbs = 29;


uiTaskRunner::uiTaskRunner( uiParent* p, bool dispmsgonerr ) 
    : uiDialog( p, uiDialog::Setup("Executing",mNoDlgTitle,mNoHelpID)
	.nrstatusflds( -1 )
	.oktext("Pause")
	.canceltext("Abort") )
    , task_( 0 )
    , thread_(0)
    , tim_(*new Timer("") )
    , execnm_("")
    , statelock_(true)
    , dispinfolock_(false)
    , uitaskrunnerthreadlock_(false)
    , dispmsgonerr_( dispmsgonerr )
    , symbidx_( 0 )
{
    proglbl_ = new uiLabel( this, noprogbardispsymbs[0] );
    proglbl_->attach( hCentered );
#ifdef __debug__
    proglbl_->setHSzPol( uiObject::WideVar );
#endif

    progbar_ = new uiProgressBar( this, "ProgressBar", 0, 0 );
    progbar_->setPrefWidthInChar( 50 );

    tim_.tick.notify( mCB( this, uiTaskRunner, timerTick ) );
    preFinalise().notify( mCB( this, uiTaskRunner, onFinalise ) );

    statusBar()->addMsgFld( "Current activity", Alignment::Left, 2 );
    statusBar()->addMsgFld( "Counted items", Alignment::Right, 2 );
    statusBar()->addMsgFld( "Number done", Alignment::Left, 1 );
}


uiTaskRunner::~uiTaskRunner()
{
    if ( thread_ )
	{ thread_->waitForFinish(); delete thread_; thread_ = 0; }
    delete &tim_;
}


bool uiTaskRunner::execute( Task& t )
{
    if ( task_ )
	return (execres_ = t.execute());

    MouseCursorChanger mousecursor( MouseCursor::Arrow );

    task_ = &t; state_ = 1;
    prevtotalnr_ = prevnrdone_ = prevpercentage_ = -1;
    prevmessage_ = ""; prevnrdonetext_ = prevmessage_;
    execnm_ = task_->name();

    updateFields();
    return (execres_ = go());
}


void uiTaskRunner::onFinalise( CallBacker* )
{
    tim_.start( 100, true );

    Threads::Locker lckr( uitaskrunnerthreadlock_ );
    thread_ = new Threads::Thread( mCB(this,uiTaskRunner,doWork) );
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
    const BufferString nrdonetext = task_->nrDoneText();
    const BufferString message = task_->message();


    if ( prevmessage_!=message )
    {
	sb.message( message.buf(), 0 );
	prevmessage_ = message;
    }
    if ( prevnrdonetext_!=nrdonetext )
    {
	sb.message( nrdonetext.buf(), 1 );
	prevnrdonetext_ = nrdonetext;
    }

    const bool nrdonechg = prevnrdone_ != nrdone;
    if ( nrdonechg )
    {
	prevnrdone_ = nrdone;
	BufferString str; str += nrdone;
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
	    BufferString capt( "[" );
	    capt += percentage; capt += "%] "; capt += execnm_;
	    setCaption( capt.buf() );

	    prevpercentage_ = percentage;
	}
    }
    const bool disppb = totalnr > 0;
    if ( !disppb )
    {
	symbidx_++;
#ifdef __debug__
	proglbl_->setText( BufferString("[ ",symbidx_, " ]") );
#else
	if ( symbidx_ >= noprogbardispnrsymbs ) symbidx_ = 0;
	proglbl_->setText( noprogbardispsymbs[symbidx_] );
#endif
    }
    proglbl_->display( !disppb );
    progbar_->display( disppb );
}


bool uiTaskRunner::acceptOK( CallBacker* )
{
    Threads::Locker lckr( uitaskrunnerthreadlock_ );
    if ( !task_ ) return false;
    Task::Control state = task_->getState();
    lckr.unlockNow();

    if ( state==Task::Pause )
    {
	task_->controlWork( Task::Run );
	setOkText("Pause" );
    }
    else if ( state==Task::Run )
    {
	task_->controlWork( Task::Pause );
	setOkText("Resume" );
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
	BufferString message;

	Threads::Locker trlckr( uitaskrunnerthreadlock_,
				Threads::Locker::DontWaitForLock );
	if ( trlckr.isLocked() )
	{
	    message = finalizeTask();
	    trlckr.unlockNow();
	}

	if ( state<0 && dispmsgonerr_ )
	    uiMSG().error( message );

	done( state<0 ? 0 : 1 );
	return;
    }

    updateFields();
    tim_.start( 250, true );
}


BufferString uiTaskRunner::finalizeTask()
{
    BufferString message;
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


bool uiTaskRunner::rejectOK( CallBacker* )
{
    Threads::Locker lckr( uitaskrunnerthreadlock_ );
    if ( task_ ) task_->controlWork( Task::Stop );
    finalizeTask();
    lckr.unlockNow();

    state_ = (int)Task::Stop;
    execres_ = false;
    return true;
}
