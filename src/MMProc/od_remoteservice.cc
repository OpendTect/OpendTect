/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          August 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: od_remoteservice.cc,v 1.2 2010-09-10 11:54:46 cvsranojay Exp $";

#include <QCoreApplication>
#include "remcommhandler.h"

int main( int argc, char** argv )
{
    QCoreApplication app( argc, argv );
    RemCommHandler* handler = new RemCommHandler( 5050 );
    handler->listen();
    app.exec();
    delete handler;
}
