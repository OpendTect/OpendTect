/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          August 2010
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include <QCoreApplication>

#include "prog.h"
#include "remcommhandler.h"

int main( int argc, char** argv )
{
    QCoreApplication app( argc, argv );

    const bool dofork = argc > 1 && !strcmp(argv[1],"--bg");
    if ( dofork )
	forkProcess();

    RemCommHandler* handler = new RemCommHandler( 5050 );
    handler->listen();
    app.exec();
    delete handler;
}
