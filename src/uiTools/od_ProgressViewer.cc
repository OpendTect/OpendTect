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
#include "uistatusbar.h"
#include "uitextedit.h"
#include "uitoolbar.h"

#include "commandlineparser.h"
#include "filepath.h"
#include "progressmeter.h"
#include "varlenarray.h"
#include "oddirs.h"
#include "od_istream.h"
#include "sighndl.h"
#include "timer.h"


static const char* sStopAndQuit = "Stop process and Quit";
static const char* sQuitOnly = "Close this window";


class uiProgressViewer : public uiMainWin
{
public:

		uiProgressViewer(uiParent*,od_istream&,int);
		~uiProgressViewer();

    void	setDelayInMs( int d )		{ delay_ = d; }

protected:

    uiTextEdit*	txtfld;
    uiToolBar*	tb_;
    int		quittbid_;

    int		pid_;
    int		delay_;

    od_istream& strm_;
    BufferString curline_;
    Timer*	timer_;
    od_stream::Pos nrcharsread_;

    inline bool	haveProcess() const		{ return pid_ > 0; }
    inline bool	processEnded() const		{ return pid_ == 0; }

    void	doWork(CallBacker*);
    void	quitFn(CallBacker*);
    void	helpFn(CallBacker*);
    void	saveFn(CallBacker*);

    void	handleProcessStatus();
    void	appendToText();
    void	addChar(char);
};


#define mAddButton(fnm,txt,fn) \
    tb_->addButton( fnm, txt, mCB(this,uiProgressViewer,fn), false );

uiProgressViewer::uiProgressViewer( uiParent* p, od_istream& s, int pid )
	: uiMainWin(p,"Progress",1)
	, timer_(0)
	, strm_(s)
	, pid_(pid)
	, delay_(1)
	, nrcharsread_(0)
{
    topGroup()->setBorder(0);
    topGroup()->setSpacing(0);

    tb_ = new uiToolBar( this, "ToolBar" );
    quittbid_ = mAddButton( "stop", haveProcess() ? sStopAndQuit : sQuitOnly,
			    quitFn );
    mAddButton( "saveflow", "Save text to a file", saveFn );
    mAddButton( "contexthelp", "Help", helpFn );

    txtfld = new uiTextEdit( this, "", true );
    uiFont& fnt = FontList().add( "Non-prop",
	    FontData(FontData::defaultPointSize(),"Courier") );
    txtfld->setFont( fnt );

    //Ensure we have space for 80 chars
    const int nrchars = TextStreamProgressMeter::cNrCharsPerRow()+5;
    mAllocVarLenArr( char, str, nrchars+1 );
    OD::memSet( str, ' ', nrchars );
    str[nrchars] = '\0';

    int deswidth = fnt.width( str );
    const int desktopwidth = uiMain::theMain().desktopSize().hNrPics();
    if ( !mIsUdf(desktopwidth) && deswidth>desktopwidth )
	deswidth = desktopwidth;

    if ( deswidth > txtfld->defaultWidth() )
	txtfld->setPrefWidth( deswidth );

    windowClosed.notify( mCB(this,uiProgressViewer,quitFn) );

    timer_ = new Timer( "Progress" );
    timer_->tick.notify( mCB(this,uiProgressViewer,doWork) );
    timer_->start( 50, true );
}


uiProgressViewer::~uiProgressViewer()
{
    delete timer_;
}


void uiProgressViewer::appendToText()
{
    if ( curline_.isEmpty() )
	return;

    txtfld->append( curline_ );
    uiMain::theMain().flushX();
    curline_.setEmpty();
}



void uiProgressViewer::handleProcessStatus()
{
    if ( haveProcess() && !isProcessAlive(pid_) )
    {
	statusBar()->message( "Processing finished" );
	tb_->setToolTip( quittbid_, sQuitOnly );
	pid_ = 0;
    }
}


void uiProgressViewer::addChar( char c )
{
    if ( !c )
	return;

    if ( c == '\n' )
	appendToText();
    else
    {
	char buf[2]; buf[0] = c; buf[1] = '\0';
	curline_.add( buf );
    }

    nrcharsread_++;
}


void uiProgressViewer::doWork( CallBacker* )
{
    int restartdelay = delay_;

    if ( strm_.isOK() )
    {
	addChar( strm_.peek() );
	strm_.ignore( 1 );
    }
    else
    {
	handleProcessStatus();
	if ( !haveProcess() )
	{
	    appendToText();
	    statusBar()->message( processEnded() ? "Processing ended" : "" );
	    return;
	}

	strm_.reOpen();
	strm_.ignore( nrcharsread_ );
	restartdelay = mMAX(delay_,200);
			// Makes sure we're not re-opening *all* the time
    }

    statusBar()->message( curline_ );
    timer_->start( restartdelay, true );
}


void uiProgressViewer::quitFn( CallBacker* )
{
    if ( haveProcess() )
	SignalHandling::stopProcess( pid_ );
    uiMain::theMain().exit(0);
}


void uiProgressViewer::helpFn( CallBacker* )
{
    const FilePath fp( mGetUserDocDir(), "base", "index.html" );
    uiDesktopServices::openUrl( fp.fullPath() );
}


void uiProgressViewer::saveFn( CallBacker* )
{
    uiFileDialog dlg( this, false, GetProcFileName("log.txt"),
		      "*.txt", "Save log" );
    dlg.setAllowAllExts( true );
    if ( dlg.go() )
    {
	od_ostream strm( dlg.fileName() );
	if ( strm.isOK() )
	   strm << txtfld->text() << od_endl;
    }
}


int main( int argc, char** argv )
{
    SetProgramArgs( argc, argv );

    CommandLineParser cl( argc, argv );
    cl.setKeyHasValue( "pid" );
    cl.setKeyHasValue( "inpfile" );
    cl.setKeyHasValue( "delay" );

    int pid = -1;
    cl.getVal( "pid", pid );
    int delay = 1;
    cl.getVal( "delay", delay );
    BufferString inpfile;
    cl.getVal( "inpfile", inpfile );
    if ( inpfile.isEmpty() )
	inpfile = od_stream::sStdIO();

    uiMain app( argc, argv );

    od_istream istrm( inpfile );
    uiProgressViewer* pv = new uiProgressViewer( 0, istrm, pid );
    pv->setDelayInMs( delay );

    app.setTopLevel( pv );
    pv->show();
    return ExitProgram( app.exec() );
}
