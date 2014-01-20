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

#include <string.h> // declares strlen
#include <stdio.h> // defines EOF
#include <iostream> // std::cin

#define mBufLen 81


class uiProgressViewer : public uiMainWin
{
public:

		uiProgressViewer(uiParent*,std::istream&,int);
		~uiProgressViewer();

protected:

    void	quitFn(CallBacker*);
    void	helpFn(CallBacker*);
    void	saveFn(CallBacker*);

    uiToolBar*	tb_;

    std::istream& strm_;
    uiTextEdit*	txtfld;
    int		quitid_;
    Timer*	timer_;
    int		pid_;
    int		delay_;
    bool	newlineseen_;
    char	fullline_[mBufLen];

    void	doWork(CallBacker*);
    bool	getChunk(char*, int);
    void	appendToText();
};


#define mAddButton(fnm,txt,fn) \
    tb_->addButton( fnm, txt, mCB(this,uiProgressViewer,fn), false );

uiProgressViewer::uiProgressViewer( uiParent* p, std::istream& s, int i )
	: uiMainWin(p,"Progress",1)
	, timer_(0)
	, strm_(s)
	, pid_(i)
	, delay_(0)
	, newlineseen_(false)
{
    fullline_[0] = '\0';
    topGroup()->setBorder(0);
    topGroup()->setSpacing(0);

    tb_ = new uiToolBar( this, "ToolBar" );
    quitid_ = mAddButton( "stop", "Stop process and Quit", quitFn );
    mAddButton( "saveflow", "Save log", saveFn );
    mAddButton( "contexthelp", "Help", helpFn );

    txtfld = new uiTextEdit( this, "", true );
    uiFont& fnt = FontList().add( "Non-prop",
	    FontData(FontData::defaultPointSize(),"Courier") );
    txtfld->setFont( fnt );

    //Ensure we have space for 80 chars
    const int nrchars = TextStreamProgressMeter::cNrCharsPerRow()+5;
    mAllocVarLenArr( char, str, nrchars+1 );
    OD::memSet( str, ' ', nrchars );
    str[nrchars] = 0;

    int deswidth = fnt.width( str );

    const int desktopwidth = uiMain::theMain().desktopSize().hNrPics();
    if ( !mIsUdf(desktopwidth) && deswidth>desktopwidth )
	deswidth = desktopwidth;

    if ( deswidth>txtfld->defaultWidth() )
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
    txtfld->append( fullline_ );
    uiMain::theMain().flushX();
    fullline_[0] = '\0';
}


void uiProgressViewer::doWork( CallBacker* )
{
    if ( strm_.eof() || strm_.fail() )
    {
	appendToText();
	statusBar()->message( fullline_ );
	tb_->setToolTip( quitid_, "Close" );
	pid_ = 0;
	return;
    }

    int orglen = strlen( fullline_ );
    static char buf[mBufLen];
    if ( getChunk( buf, mBufLen - orglen ) )
    {
	int len = buf[1] ? strlen( buf ) : 1;

	bool needappend = buf[len-1] == '\n';
	if ( !needappend )
	    needappend = len + orglen >= mBufLen - 1;
	else
	{
	    newlineseen_ = true;
	    buf[len--] = '\0';
	}

	// cat buf to fullline_
	for ( int idx=0; idx<len; idx++ )
	    fullline_[orglen+idx] = buf[idx];
	fullline_[orglen+len] = '\0';

	if ( needappend )
	    appendToText();

	statusBar()->message( fullline_ );

    }

    timer_->start( delay_, true );
}


bool uiProgressViewer::getChunk( char* buf, int maxnr )
{
    int sz = 0;
    while ( 1 )
    {
	int c = strm_.peek();
	if ( c == EOF )
	{
	    if ( !sz ) return false;
	    break;
	}

	strm_.ignore( 1 );
	buf[sz] = (char)c;
	sz++;

	if ( !delay_ || (delay_ && isspace(buf[sz-1]))
	  || sz == maxnr || buf[sz-1] == '\n' )
	    break;

    }

    buf[sz] = '\0';
    if ( !newlineseen_ && fullline_[0] == 'd' && fullline_[1] == 'G' )
	delay_ = 1;

    return true;
}


void uiProgressViewer::quitFn( CallBacker* )
{
    if ( pid_ )
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
    int pid = 0;
    cl.getVal( "pid", pid );
    if ( pid == 0 )
	pid = argc > 1 ? toInt(argv[1]) : 0;
    BufferString logfile;
    cl.getVal( "logfile", logfile );
    od_istream istrm( logfile.isEmpty() ? std::cin : od_istream(logfile)  );
    uiMain app( argc, argv );
    uiProgressViewer* pv = new uiProgressViewer( 0, istrm.stdStream(), pid );
    app.setTopLevel( pv );
    pv->show();
   
    return ExitProgram( app.exec() );
}
