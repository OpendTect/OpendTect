/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          August 2010
________________________________________________________________________

-*/

#include "remcommhandler.h"
#include "applicationdata.h"

#include "prog.h"


int main( int argc, char** argv )
{
    OD::SetRunContext( OD::BatchProgCtxt );
    SetProgramArgs( argc, argv );
    ApplicationData app;

    auto* handler = new RemCommHandler(mCast(PortNr_Type,5050) );
    handler->listen();
    const bool res = app.exec();

    delete handler;
    return ExitProgram( res );
}

