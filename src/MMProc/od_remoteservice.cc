/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "remcommhandler.h"

#include "applicationdata.h"
#include "genc.h"
#include "moddepmgr.h"
#include "prog.h"
#include "remjobexec.h"


int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::BatchProgCtxt )
    SetProgramArgs( argc, argv, false );
    ApplicationData app;

    OD::ModDeps().ensureLoaded( "Network" );

    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "MMProc" );
    PtrMan<RemCommHandler> handler = new RemCommHandler(
					RemoteJobExec::remoteHandlerPort() );
    PIM().loadAuto( true );
    const bool ret = handler ? app.exec() : 1;
    handler = nullptr;
    return ret;
}
