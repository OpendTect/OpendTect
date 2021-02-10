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


int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::BatchProgCtxt )
    SetProgramArgs( argc, argv );
    ApplicationData app;

    PtrMan<RemCommHandler> handler = new RemCommHandler( mCast(PortNr_Type,5050) );
    handler->listen();
    return app.exec();
}
