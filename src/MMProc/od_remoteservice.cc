/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "remcommhandler.h"
#include "applicationdata.h"

#include "genc.h"
#include "prog.h"
#include "remjobexec.h"


int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::BatchProgCtxt )
    SetProgramArgs( argc, argv );
    ApplicationData app;

    PtrMan<RemCommHandler> handler = new RemCommHandler(
					RemoteJobExec::remoteHandlerPort() );
    return app.exec();
}
