/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Aug 2001
 RCS:		$Id: od_SEGYExaminer.cc,v 1.2 2004-11-18 16:15:23 bert Exp $
________________________________________________________________________

-*/

#include "uimain.h"
#include "conn.h"
#include "ptrman.h"
#include "msgh.h"
#include "seistrc.h"
#include "segytr.h"
/*
#include "seisscanner.h"
#include "segythdef.h"
#include "iostrm.h"
#include "ioman.h"
#include "iopar.h"
#include "strmprov.h"
#include "filepath.h"
#include "ptrman.h"
#include "survinfo.h"
*/

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

uiSeisSEGYExamine( uiParent* p, const char* fname, bool mult )
	: uiDialog(p,
		uiDialog::Setup("Examine SEG-Y","",0)
		.modal(false)
		.oktext("")
		.canceltext("Dismiss"))
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
    if ( txt && *txt ) UsrMsg( txt );
#endif
}

    BufferString	info;
    uiTextEdit*		txtfld;
};


int main( int argc, char ** argv )
{
    bool inpok = argc > 1;

    int argidx = 1;
    bool multi = false;
    if ( inpok && !strcmp(argv[argidx],"--multi") )
    {
	multi = true;
	argidx++;
	inpok = argc > 2;
    }

    if ( !inpok )
    {
	std::cerr << "Usage: " << argv[0] << " [--multi] filename\n"
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

    uiSeisSEGYExamine* sqyex = new uiSeisSEGYExamine( 0, fnm, multi );
    app.setTopLevel( sqyex );
    sqyex->show();
    exitProgram( app.exec() ); return 0;
}

