/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink/A.H. Bril
 Date:          Aug 2000/Oct 2001
 RCS:           $Id: uitaskrunner.cc,v 1.3 2008-03-27 11:26:25 cvsbert Exp $
________________________________________________________________________

-*/

#include "uitaskrunner.h"
#include "uiprogressbar.h"
#include "uistatusbar.h"

#include "timer.h"
#include "uimsg.h"
#include "thread.h"
#include "timefun.h"

#include <math.h>


uiTaskRunner::uiTaskRunner( uiParent* p ) 
    : uiDialog( p, uiDialog::Setup("","",0)
	.nrstatusflds( -1 )
	.oktext("Pause")
	.canceltext("Abort") )
    , task_( 0 )
    , tim_(*new Timer("") )
    , execnm_("")
    , statemutex_( *new Threads::Mutex )
{
    progbar_ = new uiProgressBar( this, "", 0, 0 );
    progbar_->setPrefWidthInChar( 50 );

    tim_.tick.notify( mCB( this, uiTaskRunner, timerTick ) );
    finaliseStart.notify( mCB( this, uiTaskRunner, onFinalise ) );

    statusBar()->addMsgFld( "Current activity", uiStatusBar::Left, 2 );
    statusBar()->addMsgFld( "Counted items", uiStatusBar::Right, 2 );
    statusBar()->addMsgFld( "Number done", uiStatusBar::Left, 1 );
}


uiTaskRunner::~uiTaskRunner()
{
    delete &statemutex_;
    delete &tim_;
}


bool uiTaskRunner::execute( Task& t )
{
    if ( task_ )
	return t.execute();

    task_ = &t;
    state_ = 1;
    prevtotalnr_ = -1;
    prevnrdone_ = -1;
    prevpercentage_ = -1;
    prevmessage_ = "";
    prevnrdonetext_ = prevmessage_;
    execnm_ = task_->name();

    updateFields();
    return go();
}


void uiTaskRunner::onFinalise( CallBacker* )
{
    const int totalnr = task_->totalNr();

    tim_.start( 100, true );

    thread_ = new Threads::Thread( mCB(this,uiTaskRunner,doWork) );
}


void uiTaskRunner::doWork( CallBacker* )
{
    task_->enableNrDoneCounting( true );
    task_->enableWorkContol( true );
    bool res = task_->execute();

    statemutex_.lock();
    state_ = res ? 0 : -1;
    statemutex_.unLock();

    thread_->threadExit();
}


void uiTaskRunner::updateFields()
{
    uiStatusBar& sb = *statusBar();

    dispinfomutex_.lock();
    const int totalnr = task_->totalNr();
    const int nrdone = task_->nrDone();
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
	if ( prevtotalnr_ <= 0 )
	    progbar_->display( true );

	if ( totalnr != prevtotalnr_ )
	{
	    progbar_->setTotalSteps( totalnr );
	    prevtotalnr_ = totalnr;
	}

	if ( nrdonechg )
	    progbar_->setProgress( nrdone );

	const float fpercentage = 100. * ((float)nrdone) / totalnr;
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
    else if ( prevtotalnr_ > 0 )
	progbar_->display( false );

    dispinfomutex_.unLock();
}


void uiTaskRunner::timerTick( CallBacker* )
{
    statemutex_.lock();
    const int state = state_;
    statemutex_.unLock();

    if ( state<1 )
    {
	thread_->stop();
	thread_ = 0;

	if ( state<0 )
	    uiMSG().error( task_->message() );

	task_ = 0;

	done( state<0 ? 0 : 1 );
	return;
    }

    updateFields();
    tim_.start( 250, true );
}


bool uiTaskRunner::acceptOK( CallBacker* )
{
    Task::Control state = task_->getState();
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


bool uiTaskRunner::rejectOK( CallBacker* )
{
    task_->controlWork( Task::Stop );
    thread_->stop();
    thread_ = 0;

    return true;
}
