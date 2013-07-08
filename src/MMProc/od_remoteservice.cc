/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          August 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include <QCoreApplication>

#include "remcommhandler.h"
#include "commandlineparser.h"

#include "prog.h"


int main( int argc, char** argv )
{
    SetProgramArgs( argc, argv );
    QCoreApplication app( argc, argv );

    const bool dofork = CommandLineParser().hasKey( "bg" );
    if ( dofork )
	forkProcess();

    RemCommHandler* handler = new RemCommHandler( 5050 );
    handler->listen();
    app.exec();
    delete handler;
}
