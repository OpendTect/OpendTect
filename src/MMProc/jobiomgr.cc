/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "jobiomgr.h"

#include "commandlineparser.h"
#include "debug.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "hostdata.h"
#include "ioman.h"
#include "iopar.h"
#include "jobinfo.h"
#include "keystrs.h"
#include "networkcommon.h"
#include "oddirs.h"
#include "queue.h"
#include "separstr.h"
#include "string2.h"
#include "oscommand.h"
#include "od_ostream.h"
#include "survinfo.h"
#include "systeminfo.h"
#include "netserver.h"
#include "timefun.h"

#include "mmcommunicdefs.h"
#ifndef __win__
# include <unistd.h>
#endif


#define mDebugOn	(DBG::isOn(DBG_MM))

#define mErrRet(msg) \
{\
    msg_ = msg;\
    DBG::message(DBG_MM,msg);\
    return false;\
}


// JobInfo

JobInfo::JobInfo( int dnr )
    : descnr_(dnr)
{
}


JobInfo::~JobInfo()
{
}


// StatusInfo

StatusInfo::StatusInfo( char tg, int desc, int stat, int pid,
			const char* mg, const char* hostname, int time )
    : tag(tg)
    , descnr(desc)
    , status(stat)
    , msg(mg)
    , hostnm(hostname)
    , timestamp(time)
    , procid(pid)
{
}


StatusInfo::~StatusInfo()
{
}


// CommandString

CommandString::CommandString( const HostData& hostdata, const char* init )
    : hstdata_(hostdata)
    , cmd_(init)
{
}


CommandString::~CommandString()
{
}


CommandString& CommandString::operator=( const char* str )
{
    cmd_ = str;
    return *this;
}


void CommandString::add( const char* txt )
{ cmd_.addSpace().add( txt ); }

void CommandString::addFlag( const char* f, const char* v )
{
    if ( !v || !*v ) return;

    const BufferString flag = CommandLineParser::createKey( f );
    add( flag.str() );
    add( v );
}


void CommandString::addFlag( const char* f, int v )
{
    BufferString flag = CommandLineParser::createKey( f );
    flag.addSpace().add( v );
    add( flag.str() );
}


void CommandString::addFilePath( const FilePath& fp )
{
    const FilePath::Style stl( hstdata_.pathStyle() );
    cmd_.add( stl == FilePath::Unix ? " '" : " \"" )
	.add( fp.fullPath(stl) )
	.add( stl == FilePath::Unix ? "'" : "\"" );
}



/*!\brief Connects job to host.
 *
 */
class JobHostRespInfo
{
public:
			JobHostRespInfo( const HostData& hd, int descnr,
					 char resp = mRSP_WORK )
			: hostdata_(hd)
			, descnr_(descnr)
			, response_( resp ) {}

    HostData		hostdata_;
    int			descnr_;
    char		response_;
};


/*!\brief Sets up a thread that waits for clients to connect.

  The task of the JobIOHandler is to maintain contact with clients.
  Most of the fuctionality could be implemented in the JobIOMgr,
  but we want to be absolutely sure the listening thread
  does not interfear with the normal exection thread's data.

  Therefore, all requests to and from the JobIOHandler are
  made mutex protected.

*/
class JobIOHandler : public CallBacker
{
public:
JobIOHandler( PortNr_Type firstport )
    : firstport_(firstport)
    , server_(false)
{
    mAttachCB( server_.readyRead, JobIOHandler::socketCB );
    listen( firstport_ );
}

virtual	~JobIOHandler()
{
    detachAllNotifiers();
    server_.close();
}

    bool		ready() const	{ return ready_ && port() > 0; }
    Network::Authority	authority() const
			{ return server_.authority(); }
    PortNr_Type		port() const	{ return usedport_; }

    void		listen(PortNr_Type firstport,int maxtries=3000);
    void		reqModeForJob(const JobInfo&,JobIOMgr::Mode);
    void		addJobDesc(const HostData&,int descnr);
    void		removeJobDesc(const char* hostnm,int descnr);
    ObjQueue<StatusInfo>& statusQueue() { return statusqueue_; }

protected:

    JobHostRespInfo*		getJHRFor(int desc,const char* hostnm);
    bool			readTag(char& tag,SeparString& sepstr,
					const OD::String& data);
    void			socketCB(CallBacker*);
    char			getRespFor(int desc,const char* hostnm);

    bool*			exitreq_ = nullptr;
    Network::Server		server_;
    PortNr_Type			firstport_;
    PortNr_Type			usedport_ = 0;
    ObjQueue<StatusInfo>	statusqueue_;
    ObjectSet<JobHostRespInfo>	jobhostresps_;
    bool			ready_ = false;
};


void JobIOHandler::listen( PortNr_Type firstport, int maxtries )
{
    uiRetVal portmsg;
    PortNr_Type currentport = firstport;
    currentport = Network::getUsablePort( portmsg, currentport, maxtries );
    ready_ = currentport >= firstport && portmsg.isOK() &&
	     server_.listen( Network::Any, currentport );
    if ( ready_ )
	usedport_ = currentport;
    else
	server_.close();
}


void JobIOHandler::addJobDesc( const HostData& hd, int descnr )
{
    jobhostresps_ += new JobHostRespInfo( hd, descnr );
}


void JobIOHandler::removeJobDesc( const char* hostnm, int descnr )
{
    JobHostRespInfo* jhri = getJHRFor( descnr, hostnm );
    if ( jhri )
    {
	jobhostresps_ -= jhri;
	delete jhri;
    }
}


JobHostRespInfo* JobIOHandler::getJHRFor( int descnr, const char* hostnm )
{
    JobHostRespInfo* jhri = nullptr;
    const int sz = jobhostresps_.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	JobHostRespInfo* jhri_ = jobhostresps_[idx];
	if ( descnr < 0 || jhri_->descnr_==descnr )
	{
	    if ( !hostnm || !*hostnm )
	    {
		jhri = jhri_;
		break;
	    }

	    BufferString shrthostnm = hostnm;
	    char* ptr = shrthostnm.find( '.' );
	    if ( ptr ) *ptr = '\0';
#ifdef __unix__
	    if ( jhri_->hostdata_.isKnownAs(hostnm) ||
		 jhri_->hostdata_.isKnownAs(shrthostnm) )
#endif
		{ jhri = jhri_; break; }
	}
    }

    return jhri;
}


char JobIOHandler::getRespFor( int descnr, const char* hostnm )
{
    char resp = mRSP_UNDEF;
    JobHostRespInfo* jhri = getJHRFor( descnr, hostnm );
    if ( jhri )
	resp = jhri->response_;

    return resp;
}


void JobIOHandler::reqModeForJob( const JobInfo& ji, JobIOMgr::Mode mode )
{
    char resp = mRSP_UNDEF;

    switch( mode )
    {
    case JobIOMgr::Work		: resp = mRSP_WORK; break;
    case JobIOMgr::Pause	: resp = mRSP_PAUSE; break;
    case JobIOMgr::Stop		: resp = mRSP_STOP; break;
    }

    const int descnr = ji.descnr_;
    BufferString hostnm;
    if ( ji.hostdata_ )
	hostnm = ji.hostdata_->getHostName();

    JobHostRespInfo* jhri = getJHRFor( descnr, hostnm );
    if ( jhri )
	jhri->response_ = resp;
}


void JobIOHandler::socketCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,socketid,cb);
    BufferString data;
    server_.read( socketid, data );
    char tag=mCTRL_STATUS;
    int jobid=-1;
    int status=mSTAT_UNDEF;
    BufferString hostnm;
    BufferString errmsg;
    int procid=-1;

    if ( !data.isEmpty() )
    {
	SeparString statstr;
	const bool ok = readTag( tag, statstr, data );
	if ( ok )
	{
	    jobid = statstr.getIValue( 0 );
	    status = statstr.getIValue( 1 );
	    hostnm = statstr[2];
	    procid = statstr.getIValue( 3 );
	    errmsg = statstr[4];

	    char response = getRespFor( jobid, hostnm );
	    if ( response != mRSP_UNDEF && response != mRSP_STOP )
	    {
		statusqueue_.add( new StatusInfo( tag, jobid, status,
			procid, errmsg, hostnm, Time::getMilliSeconds()) );
	    }

	    // hardly ever needed and quite noisy.
	    static bool debug_resp = GetEnvVarYN("DTECT_MM_DEBUG_RESP");
	    if ( debug_resp && mDebugOn )
	    {
		BufferString msg( "JobIOMgr::JobIOMgr: Response to host " );
		msg.add( hostnm ).add( ", job: " ).add( jobid ).add( ": " );
		if ( response == mRSP_STOP )
		    msg += "STOP";
		else if ( response == mRSP_WORK )
		    msg += "WORK";
		else if ( response == mRSP_PAUSE )
		    msg += "PAUSE";
		else
		    msg += &response;
		DBG::message( msg );
	    }
	    char resp[2];
	    resp[0] = response;
	    resp[1] = '\0';
	    server_.write( socketid, resp );
	}
	else
	{
	    errmsg = server_.errorMsg();
	    ErrMsg( errmsg );
	}
    }
}


bool JobIOHandler::readTag( char& tag, SeparString& sepstr,
			    const OD::String& data )
{
    tag = data[0];
    sepstr.setSepChar( data[1] );
    sepstr = data.buf() + 2;
    return true;
}


// JobIOMgr
JobIOMgr::JobIOMgr( PortNr_Type firstport, int niceval )
    : iohdlr_(*new JobIOHandler(firstport))
    , niceval_(niceval)
    , execpars_(OS::Batch)
{
    for ( int count=0; count<10 && !iohdlr_.ready(); count++ )
	{ sleepSeconds( 0.1 ); }

    if ( mDebugOn )
    {
	BufferString msg( "JobIOMgr::JobIOMgr " );
	if( iohdlr_.ready() )
	{
	    msg += "ready and listening to port ";
	    msg += iohdlr_.port();
	}
	else
	    msg += "NOT ready (yet). Clients might not be able to connect.";

	DBG::message( msg );
    }

#ifdef __unix__
    const StepInterval<int> nicerg(
		    OS::CommandExecPars::cMachineUserPriorityRange( false ) );
    execpars_.prioritylevel_ = -1.f * mCast(float,niceval) / nicerg.width();
#endif
}


JobIOMgr::~JobIOMgr()
{ delete &iohdlr_; }

void JobIOMgr::reqModeForJob( const JobInfo& ji, Mode m )
{ iohdlr_.reqModeForJob(ji,m); }

void JobIOMgr::removeJob( const char* hostnm, int descnr )
{ iohdlr_.removeJobDesc(hostnm,descnr); }

ObjQueue<StatusInfo>& JobIOMgr::statusQueue()
{ return iohdlr_.statusQueue(); }

bool JobIOMgr::isReady() const
{ return iohdlr_.ready(); }


Network::Authority JobIOMgr::authority() const
{
    return iohdlr_.authority();
}


bool JobIOMgr::startProg( const char* progname, IOPar& iop,
			  const FilePath& basefp, const JobInfo& ji,
			  const char* rshcomm )
{
    DBG::message(DBG_MM,"JobIOMgr::startProg");
    if ( !ji.hostdata_ )
	mErrRet("Internal: No hostdata provided")

    const HostData& machine = *ji.hostdata_;
    FilePath ioparfp;
    if ( !mkIOParFile(basefp,machine,iop,ioparfp,msg_) )
	return false;

    OS::MachineCommand mc;
    mkCommand( mc, machine, progname, basefp, ioparfp, ji, rshcomm );

    iohdlr_.addJobDesc( machine, ji.descnr_ );
    if ( mDebugOn )
    {
	const BufferString msg( "Executing: ", mc.toString(&execpars_) );
	DBG::message(msg);
    }

    if ( !mc.execute(execpars_) )
    {
	iohdlr_.removeJobDesc( machine.getHostName(), ji.descnr_ );
	mErrRet( BufferString("Failed to submit command '",
				mc.toString(&execpars_), "'") )
    }

    return true;
}


#ifdef __win__
extern const OD::String& getTempBaseNm();

extern int& MMJob_getTempFileNr();

static FilePath getConvertedFilePath( const HostData& hd, const FilePath& fp )
{
    FilePath newfp = hd.prefixFilePath( HostData::Data );
    if ( !newfp.nrLevels() ) return fp;

    BufferString proc( getTempBaseNm() );
    proc += "_";
    proc += MMJob_getTempFileNr()-1;
    newfp.add( GetSurveyName() ).add( "Proc" )
	 .add( proc ).add( fp.fileName() );
    return newfp;
}


bool JobIOMgr::mkIOParFile( FilePath& iopfp, const FilePath& basefp,
			    const HostData& remotemachine, const IOPar& iop )
{
    iopfp = basefp; iopfp.setExtension( ".par", false );
    IOPar newiop( iop );
    const FilePath::Style machpathstyle(remotemachine.pathStyle());

    FilePath remoteparfp = getConvertedFilePath( remotemachine, basefp );
    BufferString bs( remoteparfp.fullPath() );
    bs.replace( '.',  '_' );
    FilePath logfp( bs );
    remoteparfp.setExtension( ".par", false );
    logfp.setExtension( ".log", false );
    const FilePath remotelogfp( logfp );
    newiop.set( sKey::LogFile(), remotelogfp.fullPath(machpathstyle) );

    const FilePath remdata = remotemachine.prefixFilePath( HostData::Data );
    const BufferString tmpstor = iop.find( sKey::TmpStor() );
    if ( !tmpstor.isEmpty() )
    {
	const FilePath path( tmpstor );
	FilePath remotetmpdir( remdata.nrLevels() ? remdata.fullPath()
						  : path.fullPath() );
	if ( remdata.nrLevels() )
	{
	    remotetmpdir.add( GetSurveyName() ).add( "Seismics" )
			.add( path.fileName() );
	}

	newiop.set( sKey::TmpStor(),remotetmpdir.fullPath(machpathstyle) );
    }

    newiop.set( sKey::DataRoot(), remdata.fullPath(machpathstyle) );
    newiop.set( sKey::Survey(), IOM().surveyName() );

    const BufferString remotelogfnm( logfp.fullPath(machpathstyle) );
    const BufferString remoteiopfnm( iopfp.fullPath() );
    if ( File::exists(remotelogfnm) ) File::remove( remotelogfnm );
    if ( File::exists(remoteiopfnm) ) File::remove( remoteiopfnm );

    od_ostream iopstrm( remoteiopfnm );
    if ( !iopstrm.isOK() )
    {
	const BufferString s( "Cannot open '", remoteiopfnm, "' for write ...");
	mErrRet(s)
    }
    else if ( !newiop.write(iopstrm,sKey::Pars()) )
    {
	const BufferString s("Cannot write parameters into '",remoteiopfnm,"'");
	mErrRet(s)
    }

    iopfp.set( remoteparfp.fullPath(machpathstyle) );
    return true;
}

#else

bool JobIOMgr::mkIOParFile( FilePath& iopfp, const FilePath& basefp,
			    const HostData& machine, const IOPar& iop )
{
    iopfp = basefp; iopfp.setExtension( ".par", false );
    const BufferString iopfnm( iopfp.fullPath() );
    FilePath logfp(basefp); logfp.setExtension( ".log", false );
    const BufferString logfnm( logfp.fullPath() );

    FilePath remotelogfnm( machine.convPath(HostData::Data,logfp) );

    IOPar newiop( iop );
    newiop.set( sKey::LogFile(), remotelogfnm.fullPath(machine.pathStyle()) );

    const BufferString tmpstor = iop.find( sKey::TmpStor() );
    if ( !tmpstor.isEmpty() )
    {
	const FilePath remotetmpdir =
		machine.convPath( HostData::Data, tmpstor );
	newiop.set( sKey::TmpStor(),
		    remotetmpdir.fullPath(machine.pathStyle()) );
    }


    const FilePath remotedr =
		machine.convPath( HostData::Data, GetBaseDataDir() );
    newiop.set( sKey::DataRoot(),
		remotedr.fullPath(machine.pathStyle()) );
    newiop.set( sKey::Survey(), IOM().surveyName() );

    if ( File::exists(iopfnm) ) File::remove( iopfnm );
    if ( File::exists(logfnm) ) File::remove( logfnm );

    od_ostream iopstrm( iopfnm );
    if ( !iopstrm.isOK() )
    {
	const BufferString s( "Cannot open '", iopfnm, "' for write ..." );
	mErrRet(s)
    }
    if ( !newiop.write(iopstrm,sKey::Pars()) )
    {
	const BufferString s( "Cannot write parameters into '", iopfnm, "'" );
	mErrRet(s)
    }

    iopfp = machine.convPath( HostData::Data, iopfp );
    return true;
}

#endif



#undef mErrRet
#define mErrRet() \
{\
    DBG::message(DBG_MM,msg);\
    return false;\
}

#ifdef __win__
bool JobIOMgr::mkIOParFile( const FilePath& basefp,
			    const HostData& remotemachine, const IOPar& iop,
			    FilePath& iopfp, BufferString& msg )
{
    iopfp = basefp; iopfp.setExtension( ".par", false );
    IOPar newiop( iop );
    const FilePath::Style machpathstyle( remotemachine.pathStyle() );

    FilePath remoteparfp = getConvertedFilePath( remotemachine, basefp );
    BufferString bs( remoteparfp.fullPath() );
    bs.replace( '.',  '_' );
    FilePath logfp( bs );
    remoteparfp.setExtension( ".par", false );
    logfp.setExtension( ".log", false );
    const FilePath remotelogfp( logfp );
    newiop.set( sKey::LogFile(), remotelogfp.fullPath(machpathstyle) );

    const FilePath remdata = remotemachine.prefixFilePath( HostData::Data );
    const BufferString tmpstor = iop.find( sKey::TmpStor() );
    if ( !tmpstor.isEmpty() )
    {
	const FilePath path( tmpstor );
	FilePath remotetmpdir( remdata.nrLevels() ? remdata.fullPath()
						  : path.fullPath() );
	if ( remdata.nrLevels() )
	{
	    remotetmpdir.add( GetSurveyName() ).add( "Seismics" )
			.add( path.fileName() );
	}

	newiop.set( sKey::TmpStor(),remotetmpdir.fullPath(machpathstyle) );
    }

    newiop.set( sKey::DataRoot(), remdata.fullPath(machpathstyle) );
    newiop.set( sKey::Survey(), IOM().surveyName() );

    const BufferString remotelogfnm( logfp.fullPath(machpathstyle) );
    const BufferString remoteiopfnm( iopfp.fullPath() );
    if ( File::exists(remotelogfnm) ) File::remove( remotelogfnm );
    if ( File::exists(remoteiopfnm) ) File::remove( remoteiopfnm );

    od_ostream iopstrm( remoteiopfnm );
    if ( !iopstrm.isOK() )
    {
	msg.set( "Cannot open '" ).add( remoteiopfnm ).add( "' for write ..." );
	mErrRet()
    }
    else if ( !newiop.write(iopstrm,sKey::Pars()) )
    {
	msg.set( "Cannot write parameters into '" ).add( remoteiopfnm );
	msg.add( "'" );
	mErrRet()
    }

    iopfp.set( remoteparfp.fullPath(machpathstyle) );
    return true;
}

#else

bool JobIOMgr::mkIOParFile( const FilePath& basefp,
			    const HostData& machine, const IOPar& iop,
			    FilePath& iopfp, BufferString& msg )
{
    iopfp = basefp; iopfp.setExtension( ".par", false );
    const BufferString iopfnm( iopfp.fullPath() );
    FilePath logfp(basefp); logfp.setExtension( ".log", false );
    const BufferString logfnm( logfp.fullPath() );

    FilePath remotelogfnm( machine.convPath(HostData::Data,logfp) );

    IOPar newiop( iop );
    newiop.set( sKey::LogFile(), remotelogfnm.fullPath(machine.pathStyle()) );

    const BufferString tmpstor = iop.find( sKey::TmpStor() );
    if ( !tmpstor.isEmpty() )
    {
	const FilePath remotetmpdir =
		machine.convPath( HostData::Data, tmpstor );
	newiop.set( sKey::TmpStor(),
		    remotetmpdir.fullPath(machine.pathStyle()) );
    }

    const FilePath remotedr =
		machine.convPath( HostData::Data, GetBaseDataDir() );
    newiop.set( sKey::DataRoot(),
		remotedr.fullPath(machine.pathStyle()) );
    newiop.set( sKey::Survey(), IOM().surveyName() );

    if ( File::exists(iopfnm) ) File::remove( iopfnm );
    if ( File::exists(logfnm) ) File::remove( logfnm );

    od_ostream iopstrm( iopfnm );
    if ( !iopstrm.isOK() )
    {
	msg.set( "Cannot open '" ).add( iopfnm ).add( "' for write ..." );
	mErrRet()
    }
    if ( !newiop.write(iopstrm,sKey::Pars()) )
    {
	msg.set( "Cannot write parameters into '" ).add( iopfnm ).add( "'" );
	mErrRet()
    }

    iopfp = machine.convPath( HostData::Data, iopfp );
    return true;
}

#endif


void JobIOMgr::mkCommand( OS::MachineCommand& mc, const HostData& machine,
			  const char* progname, const FilePath& basefp,
			  const FilePath& iopfp, const JobInfo& ji,
			  const char* rshcomm )
{
    const HostData& localhost = machine.localHost();
    const bool remote = &machine != &localhost;
    const bool unixtounix = remote && !localhost.isWindows() &&
			    !machine.isWindows();

    mc.setHostIsWindows( machine.isWindows() );
    if ( remote )
    {
	mc.setRemExec( unixtounix ? rshcomm
				  : OS::MachineCommand::odRemExecCmd() )
	  .setHostName( machine.connAddress() )
	  .setHostIsWindows( machine.isWindows() );
    }

    if ( remote && unixtounix )
	setRexecCmd( progname, machine, localhost, mc );
    else
	mc.setProgram( progname );

    const Network::Authority servauth = authority();
    mc.addKeyedArg( OS::MachineCommand::sKeyPrimaryHost(),
		    servauth.getConnHost(Network::Authority::IPv4) );
    mc.addKeyedArg( OS::MachineCommand::sKeyPrimaryPort(), servauth.getPort() );
    mc.addKeyedArg( OS::MachineCommand::sKeyJobID(), ji.descnr_ );
    mc.addArg( iopfp.fullPath(machine.pathStyle()) );
}


BufferString JobIOMgr::mkRexecCmd( const char* prognm,
				   const HostData& machine,
				   const HostData& localhost )
{
    FilePath execfp( GetScriptDir(), "exec_prog" );
    execfp = machine.convPath( HostData::Appl, execfp, &localhost );

    BufferString res( execfp.fullPath( machine.pathStyle() ).str() );

    return res.addSpace().add( prognm );
}


void JobIOMgr::setRexecCmd( const char* prognm, const HostData& machine,
		    const HostData& localhost, OS::MachineCommand& mc ) const
{
    FilePath execfp( GetShellScript("exec_prog") );
    execfp = machine.convPath( HostData::Appl, execfp, &localhost );
    const BufferString res( execfp.fullPath( machine.pathStyle() ).str() );

    mc.setProgram( res ).addArg( prognm );
}
