/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Aug 2001
 RCS:		$Id: od_SEGYExaminer.cc,v 1.11 2008-07-16 05:14:44 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uimain.h"
#include "conn.h"
#include "ptrman.h"
#include "msgh.h"
#include "seistrc.h"
#include "segytr.h"
#include "iopar.h"
#include "timer.h"

#include "uidialog.h"
#include "uitextedit.h"

#include "prog.h"
#include <unistd.h>
#include <iostream>

#ifdef __win__
#include "filegen.h"
#endif


class uiSeisSEGYExamine : public uiDialog
{
public:

uiSeisSEGYExamine( uiParent* p, const char* fnm, bool mult, int n, int f,
		   int ntr )
	: uiDialog(p,uiDialog::Setup("Examine SEG-Y","",0)
				    .nrstatusflds(2).modal(false))
	, fname(fnm)
	, multi(mult)
	, ns(n)
	, fmt(f)
	, nrtrcs(ntr)
	, timr("startup timer")
{
    setCtrlStyle( LeaveOnly );
    txtfld = new uiTextEdit( this, "", true );
    finaliseDone.notify( mCB(this,uiSeisSEGYExamine,onStartUp) );
}


void onStartUp( CallBacker* )
{
    timr.tick.notify(  mCB(this,uiSeisSEGYExamine,updateInput) );
    timr.start( 100, true );
}


void updateInput( CallBacker* )
{
    display( true );
    updateInp();
    txtfld->setText( info );
    outInfo( "Right-click to select text" );
    setName( fname );
}


void updateInp()
{
    if ( !multi )
    {
	char* ptr = strchr( fname.buf(), '%' );
	if ( ptr ) *ptr = '1';
	ptr = strchr( fname.buf(), '*' );
	if ( ptr ) *ptr = '1';
    }
    toStatusBar( fname, 1 );
    outInfo( "Opening input data stream" );
    Conn* conn = new StreamConn( fname, Conn::Read );
    if ( conn->state() == Conn::Bad )
    {
	delete conn;
	info = "Cannot open input file or device";
	return;
    }

    outInfo( "Reading SEG-Y header" );
    PtrMan<SEGYSeisTrcTranslator> tr = SEGYSeisTrcTranslator::getInstance();

    if ( ns || fmt )
    {
	outInfo( "ns and/or fmt overruled" );
	IOPar iop;
	if ( ns ) iop.set( SEGYSeisTrcTranslator::sExternalNrSamples, ns );
	if ( fmt ) iop.set( SEGYSeisTrcTranslator::sNumberFormat, fmt );
	tr->usePar( iop );
    }

    tr->setTrcDumpMode( SEGYSeisTrcTranslator::String );
    tr->setMaxTrcsDumped( nrtrcs );
    if ( !tr->initRead( conn, Seis::PreScan ) )
    {
	info = "No information:\n";
	info += tr->errMsg();
	outInfo( "" );
	delete conn;
	return;
    }

    BufferString str( "Reading first " );
    str += nrtrcs; str += " traces ...";
    outInfo( str );
    SeisTrc trc;
    bool stoppedatend = false;
    while ( tr->nrTrcsDumped() < nrtrcs )
    {
	if ( !tr->read(trc) )
	    { stoppedatend = true; break; }
    }
    const int nrtrcdumped = tr->nrTrcsDumped();
    tr->closeTraceDump();

    info = tr->dumpStr();
    str = "\n";
    str += nrtrcdumped < 1 ? "No traces found"
	   : (stoppedatend ? "Total traces present in file: "
			   : "Traces displayed: ");
    if ( nrtrcdumped > 0 ) str += nrtrcdumped;
    info += str;
    outInfo( "Closing input stream" );
}

bool rejectOK()
{
    return true;
}

void outInfo( const char* txt )
{
    toStatusBar( txt, 0 );
}

    BufferString	info;
    BufferString	fname;
    Timer		timr;
    int			ns;
    int			fmt;
    int			nrtrcs;
    bool		multi;

    uiTextEdit*		txtfld;
};


int main( int argc, char ** argv )
{
    int argidx = 1;
    bool multi = false; int ns = 0; int fmt = 0; int nrtrcs = 5;
    while ( argc > argidx
	 && *argv[argidx] == '-' && *(argv[argidx]+1) == '-' )
    {
	if ( !strcmp(argv[argidx],"--multi") )
	    multi = true;
	else if ( !strcmp(argv[argidx],"--ns") )
	    { argidx++; ns = atoi( argv[argidx] ); }
	else if ( !strcmp(argv[argidx],"--fmt") )
	    { argidx++; fmt = atoi( argv[argidx] ); }
	else if ( !strcmp(argv[argidx],"--nrtrcs") )
	    { argidx++; nrtrcs = atoi( argv[argidx] ); }
	else
	    { std::cerr << "Ignoring option: " << argv[argidx] << std::endl; }
	argidx++;
    }

    if ( argc <= argidx )
    {
	std::cerr << "Usage: " << argv[0]
	    	  << " [--multi] [--ns #samples] [--nrtrcs #traces]"
		     " [--fmt segy_format_number] filename\n"
	     << "Note: filename must be with FULL path." << std::endl;
	ExitProgram( 1 );
    }

    bool dofork = true;
#if defined( __mac__ ) || defined( __win__ )
    dofork = false;
#endif

    const int forkres = dofork ? fork() : 0;
    switch ( forkres )
    {
    case -1:
	std::cerr << argv[0] << ": cannot fork: " << errno_message()
	    	  << std::endl;
	ExitProgram( 1 );
    case 0:	break;
    default:	return 0;
    }

    char* fnm = argv[argidx];
    replaceCharacter( fnm, (char)128, ' ' );

    uiMain app( argc, argv );

#ifdef __win__
    if ( File_isLink( fnm ) )
	fnm = const_cast<char*>(File_linkTarget(fnm));
#endif

    uiSeisSEGYExamine* sgyex =
	new uiSeisSEGYExamine( 0, fnm, multi, ns, fmt, nrtrcs );
    app.setTopLevel( sgyex );
    sgyex->show();
    ExitProgram( app.exec() ); return 0;
}
