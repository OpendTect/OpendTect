/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Aug 2001
 RCS:		$Id: od_SEGYExaminer.cc,v 1.4 2005-02-25 17:51:42 cvsbert Exp $
________________________________________________________________________

-*/

#include "uimain.h"
#include "conn.h"
#include "ptrman.h"
#include "msgh.h"
#include "seistrc.h"
#include "segytr.h"
#include "iopar.h"

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

uiSeisSEGYExamine( uiParent* p, const char* fname, bool mult, int n, int f )
	: uiDialog(p,
		uiDialog::Setup("Examine SEG-Y","",0)
		.modal(false)
		.oktext("")
		.canceltext("Dismiss"))
	, ns(n)
	, fmt(f)
{
    txtfld = new uiTextEdit( this, "", true );
    updateInput( fname, mult );
}


void updateInput( const char* fname, bool mult )
{
    updateInp( fname, mult );
    txtfld->setText( info );
    outInfo( "" );
    setName( fname );
}


void updateInp( const char* fn, bool mult )
{
    BufferString fname( fn );
    if ( !mult )
    {
	char* ptr = strchr( fname.buf(), '%' );
	if ( ptr ) *ptr = '1';
	ptr = strchr( fname.buf(), '*' );
	if ( ptr ) *ptr = '1';
    }
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

    tr->dumpToString( true );
    if ( !tr->initRead( conn ) )
    {
	info = "No information:\n";
	info += tr->errMsg();
	outInfo( "" );
	delete conn;
	return;
    }

    outInfo( "Reading first 4 traces" );
    SeisTrc trc;
    int nrtrcs = 0;
    BufferString add( "\n\nFirst 4 traces displayed." );
    while ( !tr->dumpingDone() )
    {
	if ( !tr->read(trc) )
	{
	    if ( !nrtrcs )
		add = "\n\nNo traces found.";
	    else
	    {
		add = "\n\n";
		add += nrtrcs;
		add += " found in file.";
	    }
	    break;
	}
    }

    info = tr->dumpStr();
    info += add;
    outInfo( "Closing input stream" );
}

bool rejectOK()
{
    return true;
}

void outInfo( const char* txt )
{
#ifdef __debug__
    if ( txt && *txt ) std::cerr << txt << std::endl;
#endif
}

    BufferString	info;
    uiTextEdit*		txtfld;
    int			ns;
    int			fmt;
};


int main( int argc, char ** argv )
{
    int argidx = 1;
    bool multi = false; int ns = 0; int fmt = 0;
    while ( argc > argidx
	 && *argv[argidx] == '-' && *(argv[argidx]+1) == '-' )
    {
	if ( !strcmp(argv[argidx],"--multi") )
	    multi = true;
	else if ( !strcmp(argv[argidx],"--ns") )
	    { argidx++; ns = atoi( argv[argidx] ); }
	else if ( !strcmp(argv[argidx],"--fmt") )
	    { argidx++; fmt = atoi( argv[argidx] ); }
	else
	    { std::cerr << "Ignoring option: " << argv[argidx] << std::endl; }
	argidx++;
    }

    if ( argc <= argidx )
    {
	std::cerr << "Usage: " << argv[0]
	    	  << " [--multi] [--ns #samples] [--fmt segy_format_number]"
		     " filename\n"
	     << "Note: filename must be with FULL path." << std::endl;
	exitProgram( 1 );
    }

#ifndef __win__

    switch ( fork() )
    {
    case -1:
	std::cerr << argv[0] << ": cannot fork: " << errno_message()
	    	  << std::endl;
	exitProgram( 1 );
    case 0:	break;
    default:	return 0;
    }

#endif

    char* fnm = argv[argidx];
    replaceCharacter( fnm, (char)128, ' ' );

    uiMain app( argc, argv );

#ifdef __win__
    if ( File_isLink( fnm ) )
	fnm = const_cast<char*>(File_linkTarget(fnm));
#endif

    uiSeisSEGYExamine* sgyex = new uiSeisSEGYExamine( 0, fnm, multi, ns, fmt );
    app.setTopLevel( sgyex );
    sgyex->show();
    exitProgram( app.exec() ); return 0;
}
