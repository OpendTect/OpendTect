/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          August 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "remcommhandler.h"
#include "commandlineparser.h"
#include "applicationdata.h"

#include "prog.h"


int main( int argc, char** argv )
{
    SetProgramArgs( argc, argv );
    ApplicationData app;

    const bool dofork = CommandLineParser().hasKey( "bg" );
    if ( dofork )
	ForkProcess();

    RemCommHandler* handler = new RemCommHandler( mCast(PortNr_Type,5050) );
    handler->listen();
    const bool res = app.exec();

    delete handler;
    return ExitProgram( res );
}
