/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          August 2010
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: od_remoteservice.cc,v 1.5 2012-05-02 15:11:40 cvskris Exp $";

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
