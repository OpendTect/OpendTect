/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "applicationdata.h"
#include "commandlineparser.h"
#include "genc.h"
#include "ioman.h"
#include "moddepmgr.h"
#include "prog.h"
#include "remcommhandler.h"
#include "remjobexec.h"


int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::BatchProgCtxt )
    SetProgramArgs( argc, argv, false );
    ApplicationData app;

    OD::ModDeps().ensureLoaded( "Network" );

    const PortNr_Type remport = RemoteJobExec::remoteHandlerPort();
    const CommandLineParser clp( argc, argv );

    uiString msg;
    if ( !Network::isPortFree(remport,&msg) )
    {
	od_cerr() << toString(msg) << od_newline;
	od_cerr() << "There is probably already an instance running of ";
	od_cerr() << clp.getExecutableName() << od_endl;
	return 1;
    }

    const uiRetVal uirv = RemCommHandler::parseArgs( clp );
    if ( !uirv.isOK() )
    {
	od_cerr() << uirv.getText() << od_endl;
	return 1;
    }

    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "MMProc" );
    PtrMan<RemCommHandler> handler = new RemCommHandler( remport );
    PIM().loadAuto( true );
    const bool ret = handler ? app.exec() : 1;
    handler = nullptr;
    return ret;
}
