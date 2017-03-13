/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          August 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "prog.h"

#include "uidesktopservices.h"
#include "uifiledlg.h"
#include "uifont.h"
#include "uigroup.h"
#include "uimain.h"
#include "uimainwin.h"
#include "uimsg.h"
#include "uistatusbar.h"
#include "uistrings.h"
#include "uitextedit.h"
#include "uitoolbar.h"

#include "batchprog.h"
#include "commandlineparser.h"
#include "filepath.h"
#include "helpview.h"
#include "moddepmgr.h"
#include "oddirs.h"
#include "od_helpids.h"
#include "od_istream.h"
#include "progressmeter.h"
#include "sighndl.h"
#include "timer.h"
#include "texttranslator.h"
#include "varlenarray.h"

#include <iostream>

static const uiString sStopAndQuit()
{ return od_static_tr("sStopAndQuit","Stop process and Quit"); }
static const uiString sQuitOnly()
{return od_static_tr("sStopAndQuit","Close this window"); }


class uiProgressViewer : public uiMainWin
{ mODTextTranslationClass(uiProgressViewer)
public:

		uiProgressViewer(uiParent*,const BufferString& filenm,int);
		~uiProgressViewer();

    void	setDelayInMs( int d )		{ delay_ = d; }

protected:

    enum ProcStatus	{ None, Running, Finished, Terminated, AbnormalEnd };

    uiTextBrowser*	txtfld_;
    uiToolBar*		tb_;
    int			quittbid_;

    int			pid_;
    const BufferString	procnm_;
    ProcStatus		procstatus_;
    int			delay_;
    od_istream		strm_;
    Timer		timer_;

    bool		haveProcess();

    void		doWork(CallBacker*);
    void		quitFn(CallBacker*);
    void		helpFn(CallBacker*);
    void		saveFn(CallBacker*);

    void		handleProcessStatus();
    bool		closeOK();
};


#define mAddButton(fnm,txt,fn) \
    tb_->addButton( fnm, txt, mCB(this,uiProgressViewer,fn), false );

uiProgressViewer::uiProgressViewer( uiParent* p, const BufferString& fnm,
				    int pid )
    : uiMainWin(p,tr("Progress Viewer"),1)
    , timer_("Progress")
    , delay_(500)
    , strm_(fnm.str())
    , pid_(pid)
    , procnm_(getProcessNameForPID(pid))
    , procstatus_(None)
{
    sleepSeconds( 1 );
    topGroup()->setBorder(0);
    topGroup()->setSpacing(0);

    tb_ = new uiToolBar( this, uiStrings::sToolbar() );
    quittbid_ = mAddButton( "stop", haveProcess() ? sStopAndQuit() : sQuitOnly()
			    ,quitFn );
    mAddButton( "save", tr("Save text to a file"), saveFn );
    mAddButton( "contexthelp", uiStrings::sHelp(), helpFn );

    txtfld_ = new uiTextBrowser( this, "Progress", mUdf(int), true, true );
    txtfld_->setSource( fnm.str() );
    uiFont& fnt = FontList().add( "Non-prop",
	    FontData(FontData::defaultPointSize(),"Courier") );
    txtfld_->setFont( fnt );

    //Ensure we have space for 80 chars
    const int nrchars = TextStreamProgressMeter::cNrCharsPerRow()+5;
    mAllocVarLenArr( char, str, nrchars+1 );
    OD::memSet( str, ' ', nrchars );
    str[nrchars] = '\0';

    int deswidth = fnt.width( mToUiStringTodo(str) );
    const int desktopwidth = uiMain::theMain().desktopSize().hNrPics();
    if ( !mIsUdf(desktopwidth) && deswidth>desktopwidth )
	deswidth = desktopwidth;

    if ( deswidth > txtfld_->defaultWidth() )
	txtfld_->setPrefWidth( deswidth );

    mAttachCB( timer_.tick, uiProgressViewer::doWork );
    timer_.start( delay_, true );
}


uiProgressViewer::~uiProgressViewer()
{
    detachAllNotifiers();
}


bool uiProgressViewer::haveProcess()
{
    if ( mIsUdf(pid_) )
	return false;

    if ( isProcessAlive(pid_) )
    {
	procstatus_ = Running;
	return true;
    }

    procstatus_ = None;

    return false;
}


void uiProgressViewer::handleProcessStatus()
{
    if ( haveProcess() )
	return;

    if ( procstatus_ == None )
    {
	sleepSeconds( 1 );
	strm_.reOpen();
	BufferString lines;
	strm_.getAll( lines );
	if ( lines.find(BatchProgram::sKeyFinishMsg()) )
	    procstatus_ = Finished;
	else
	    procstatus_ = AbnormalEnd;

	strm_.close();
    }

    uiString stbmsg = tr("Processing finished %1")
		      .arg( procstatus_ == AbnormalEnd ? tr("abnormally.")
						       : tr("successfully.") );
    if ( procstatus_ == AbnormalEnd )
	stbmsg.append( tr(" It was probably terminated or crashed.") );
    else if ( procstatus_ == Terminated )
	stbmsg = tr("Process %1 was terminated." ).arg(procnm_);

    statusBar()->message( stbmsg );
    timer_.stop();
    tb_->setToolTip( quittbid_, sQuitOnly() );
    pid_ = mUdf(int);
}


void uiProgressViewer::doWork( CallBacker* )
{
    handleProcessStatus();
    if ( procstatus_ != Running )
	return;

    statusBar()->message( tr("Running process %1 with PID %2.")
				    .arg(procnm_).arg(pid_) );
    timer_.start( delay_, true );
}


void uiProgressViewer::quitFn( CallBacker* )
{
    if ( closeOK() )
	uiMain::theMain().exit(0);
}


bool uiProgressViewer::closeOK()
{
    if ( haveProcess() )
    {
	if ( !uiMSG().askGoOn(tr("Do you want to terminate the process?")) )
	    return false;

	SignalHandling::stopProcess( pid_ );
	pid_ = mUdf(int);
	procstatus_ = Terminated;
    }

    return true;
}


void uiProgressViewer::helpFn( CallBacker* )
{
    HelpProvider::provideHelp( mODHelpKey(mProgressViewerHelpID) );
}


void uiProgressViewer::saveFn( CallBacker* )
{
    uiFileDialog dlg( this, false, GetProcFileName("log.txt"),
		      "*.txt", uiStrings::phrSave(uiStrings::sLogs()) );
    dlg.setAllowAllExts( true );
    if ( dlg.go() )
    {
	od_ostream strm( dlg.fileName() );
	if ( strm.isOK() )
	   strm << txtfld_->text() << od_endl;
    }
}


int main( int argc, char** argv )
{
    SetProgramArgs( argc, argv );
    OD::ModDeps().ensureLoaded( "uiBase" );

    TextTranslateMgr::loadTranslations();

    CommandLineParser cl( argc, argv );
    cl.setKeyHasValue( "pid" );
    cl.setKeyHasValue( "inpfile" );
    cl.setKeyHasValue( "delay" );

    int pid = -1;
    cl.getVal( "pid", pid );
    int delay = mUdf(int);
    cl.getVal( "delay", delay );
    BufferString inpfile;
    cl.getVal( "inpfile", inpfile );
    if ( inpfile.isEmpty() )
	inpfile = od_stream::sStdIO();

    uiMain app( argc, argv );

    uiProgressViewer* pv = new uiProgressViewer( 0, inpfile, pid );
    if ( !mIsUdf(delay) )
	pv->setDelayInMs( delay );

    app.setTopLevel( pv );
    pv->show();
    return ExitProgram( app.exec() );
}
