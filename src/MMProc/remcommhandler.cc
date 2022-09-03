/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "remcommhandler.h"

#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "mmpkeystr.h"
#include "mmpserver.h"
#include "od_iostream.h"
#include "oddirs.h"
#include "odjson.h"
#include "oscommand.h"
#include "systeminfo.h"
#include "timefun.h"


using namespace MMPStr;

RemCommHandler::RemCommHandler( PortNr_Type port )
    : port_(port)
    , server_(*new MMPServer(port))
    , logstrm_(*new od_ostream)
{
    createLogFile();
    writeLog( BufferString("Starting: ", GetFullExecutablePath()) );
    writeLog( BufferString("Using DataRoot: ", GetBaseDataDir()) );
    mAttachCB( server_.startJob, RemCommHandler::startJobCB );
    mAttachCB( server_.logMsg, RemCommHandler::writeLogCB );
    mAttachCB( server_.dataRootChg, RemCommHandler::dataRootChgCB );
    mAttachCB( server_.getLogFile, RemCommHandler::getLogFileCB );
}


RemCommHandler::~RemCommHandler()
{
    detachAllNotifiers();
    delete &server_;
    delete &logstrm_;
}


void RemCommHandler::dataRootChgCB( CallBacker* )
{
    createLogFile();
    writeLog( BufferString("New DataRoot: ", GetBaseDataDir()) );
}


void RemCommHandler::startJobCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const OD::JSON::Object&,reqobj,cb);

    writeLog( BufferString("Start Job: \n",reqobj.dumpJSon()) );

    const BufferString procnm( reqobj.getStringValue(sProcName()) );
    const FilePath parfp( reqobj.getFilePath(sParFile()) );
    const BufferString parfile( parfp.fullPath() );

    const BufferString hostnm( reqobj.getStringValue(sHostName()) );
    const BufferString portnm( reqobj.getStringValue(sPortName()) );
    const BufferString jobid( reqobj.getStringValue(sJobID()) );

    if ( parfile.isEmpty() || !parfp.exists() )
    {
	writeLog( BufferString("Start Job Error - no Par File: ", parfile) );
	return;
    }

    FilePath procfp( GetExecPlfDir(), procnm );
#ifdef __win__
    procfp.setExtension( "exe" );
#endif
    const BufferString procfile( procfp.fullPath() );
    if ( !procfp.exists() || !File::isExecutable(procfile) )
    {
	writeLog( BufferString("Start Job Error - no Proc: ", procfile) );
	return;
    }

    OS::MachineCommand machcomm( procfile );
    if ( !hostnm.isEmpty() )
	machcomm.addKeyedArg( OS::MachineCommand::sKeyPrimaryHost(),
			      hostnm, OS::OldStyle );
    if ( !portnm.isEmpty() )
	machcomm.addKeyedArg( OS::MachineCommand::sKeyPrimaryPort(),
			      portnm, OS::OldStyle );
    if ( !jobid.isEmpty() )
	machcomm.addKeyedArg( "jobid", jobid, OS::OldStyle );

    machcomm.addKeyedArg( "DTECT_DATA", GetBaseDataDir(), OS::OldStyle );
    machcomm.addArg( parfile );

    OS::CommandLauncher cl( machcomm );
    OS::CommandExecPars pars( OS::Batch );
    pars.createstreams( true );
    uiRetVal uirv;
    const bool res = cl.execute( pars );
    if ( !res )
    {
	const uiString errmsg = cl.errorMsg();
	if ( errmsg.isEmpty() )
	    uirv.add( tr("Cannot launch '%1'").arg( machcomm.toString(&pars) ));
	else
	    uirv.add( errmsg );
    }

    BufferString stderrstr;
    od_istream* stderrstrm = cl.getStdError();
    if ( stderrstrm )
    {
	stderrstrm->getAll( stderrstr );
	if ( !stderrstr.isEmpty() )
	    uirv.add( toUiString(stderrstr) );
    }

    if ( !uirv.isOK() )
	writeLog( uirv.getText() );
    else
	writeLog( BufferString("Launched: ", machcomm.toString(&pars)) );
}


void RemCommHandler::writeLogCB( CallBacker* cb )
{
    mCBCapsuleUnpack(const uiRetVal&,uirv,cb);
    if ( uirv.isOK() )
	return;

    writeLog( uirv.getText() );
}


void RemCommHandler::getLogFileCB( CallBacker* )
{
    OD::JSON::Object paramobj;
    paramobj.set( sLogFile(), logfilename_ );
    uiRetVal uirv = server_.sendResponse( sGetLogFile(), paramobj );
    if ( !uirv.isOK() )
	writeLog( uirv.getText() );
}


void RemCommHandler::createLogFile()
{
    if ( !logfilename_.isEmpty() )
	logstrm_.close();

    FilePath logfp( GetBaseDataDir(), "LogFiles" );
    BufferString lhname = System::localAddress();
    lhname.replace( '.',  '_' );
    logfp.add( lhname );
    logfp.setExtension( ".log" );
    logfilename_ = logfp.fullPath();
    server_.setLogFile( logfilename_ );
    logstrm_.open( logfilename_ );
}


void RemCommHandler::writeLog( const char* msg )
{
    logstrm_ << Time::getDateTimeString() << od_endl;
    logstrm_ << msg <<od_endl;
}
