/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          Oct 2004
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "jobiomgr.h"

#include "debugmasks.h"
#include "envvars.h"
#include "errh.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "hostdata.h"
#include "ioman.h"
#include "iopar.h"
#include "jobinfo.h"
#include "keystrs.h"
#include "mmdefs.h"
#include "oddirs.h"
#include "queue.h"
#include "separstr.h"
#include "string2.h"
#include "strmprov.h"
#include "survinfo.h"
#include "systeminfo.h"
#include "tcpserver.h"
#include "thread.h"
#include "timefun.h"

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


class CommandString
{
public:
			CommandString( const char* init=0 ) 
			    : cmd( init ) {}

    void		addWoSpc( const char* txt ) { cmd += txt; }
    void		add( const char* txt ) { cmd += " "; cmd += txt; }
    void		addFlag( const char* f, const char* v )
			{
			    if ( !v || !*v ) return;
			    add(f); add(v);
			}
    void		addFlag( const char* f, int v )
			     { add(f); cmd += " "; cmd += v; }

    void		addFilePath( const FilePath& fp, FilePath::Style stl )
			{
			    cmd += stl == FilePath::Unix ? " '" : " \"";
			    cmd += fp.fullPath(stl);
			    cmd += stl == FilePath::Unix ? "'" : "\"";
			  
			}
    void		addFilePathFlag( const char* flag, const FilePath& fp,
	    			     FilePath::Style stl )
			{
			    cmd += " "; cmd += flag; cmd += " ";
			    addFilePath( fp, stl );
			}

    const BufferString&	string() { return cmd; }

    inline CommandString& operator=( const char* str )
			   { cmd = str; return *this; }
protected:
 
    BufferString	cmd;
};


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
    			JobIOHandler( int firstport )
			    : exitreq_(0) 
			    , firstport_(firstport)
			    , usedport_(firstport)
			    , ready_(false)
			    {
				serversocket_.readyRead.notify(
				    mCB(this,JobIOHandler,socketCB) );
				listen( firstport_ );
			    }

    virtual		~JobIOHandler()
			    {
				serversocket_.close();
				serversocket_.readyRead.remove(
				    mCB(this,JobIOHandler,socketCB) );
			    }
    
    bool		ready()	{ return ready_ && port() > 0; }
    int			port()	{ return usedport_; }
    void		listen(int firstport,int maxtries=3000 ); // as used in 4.0
    void		reqModeForJob(const JobInfo&, JobIOMgr::Mode);
    void		addJobDesc(const HostData&,int descnr);
    void		removeJobDesc(const char* hostnm, int descnr);
    ObjQueue<StatusInfo>& statusQueue() { return statusqueue_; }

protected:

    JobHostRespInfo*		getJHRFor( int desc, const char* hostnm );
    bool			readTag(char& tag,SeparString& sepstr,
					const BufferString& data);
    void			socketCB(CallBacker*);
    char			getRespFor(int,const char* hostnm);

    bool*			exitreq_;
    TcpServer			serversocket_;
    int				firstport_;
    int				usedport_;
    ObjQueue<StatusInfo>	statusqueue_;
    ObjectSet<JobHostRespInfo>	jobhostresps_;
    bool			ready_;
};


void JobIOHandler::listen( int firstport, int maxtries ) // as used in 4.0
{	
    int currentport = firstport;
    for ( int idx=0; idx<maxtries; idx++, currentport++ )
    {
	serversocket_.listen( System::localAddress(),
	    currentport );
	if ( serversocket_.isListening() )
	{
	    usedport_ = currentport;
	    ready_ = true;
	    break;
	}
	else
	    serversocket_.close();
    }
}


void JobIOHandler::addJobDesc( const HostData& hd, int descnr )
{
    jobhostresps_ +=
	new JobHostRespInfo( hd, descnr );
}


void JobIOHandler::removeJobDesc( const char* hostnm, int descnr )
{			
    JobHostRespInfo* jhri =
	getJHRFor( descnr, hostnm );

    if ( jhri )
    { jobhostresps_ -= jhri; delete jhri; }
}


JobHostRespInfo* JobIOHandler::getJHRFor( int descnr, const char* hostnm )
{
    JobHostRespInfo* jhri = 0;

    int sz = jobhostresps_.size();
    for ( int idx=0; idx < sz; idx++ )
    {
	JobHostRespInfo* jhri_ = jobhostresps_[idx];
	if ( descnr < 0 || jhri_->descnr_ == descnr )
	{
	    if ( !hostnm || !*hostnm )  { jhri = jhri_; break; }		

	    BufferString shrthostnm = hostnm;

	    char* ptr = strchr( shrthostnm.buf(), '.' );
	    if ( ptr ) *ptr = '\0';
#ifndef __win__
	    if ( jhri_->hostdata_.isKnownAs(hostnm)  
	      || jhri_->hostdata_.isKnownAs(shrthostnm) )
#endif
		{ jhri = jhri_; break; }	
	}
    }

    return jhri;
}


char JobIOHandler::getRespFor( int descnr , const char* hostnm )
{
    char resp = mRSP_STOP;
    JobHostRespInfo* jhri = getJHRFor( descnr, hostnm );
    if ( jhri ) resp = jhri->response_;
    return resp;
}


void JobIOHandler::reqModeForJob( const JobInfo& ji, JobIOMgr::Mode mode )
{
    char resp = mRSP_STOP;

    switch( mode )
    {
    case JobIOMgr::Work		: resp = mRSP_WORK; break;
    case JobIOMgr::Pause	: resp = mRSP_PAUSE; break;
    case JobIOMgr::Stop		: resp = mRSP_STOP; break;
    }

    int descnr = ji.descnr_;
    BufferString hostnm;
    if ( ji.hostdata_ ) hostnm = ji.hostdata_->name();

    JobHostRespInfo* jhri = getJHRFor( descnr, hostnm );
    if ( jhri ) jhri->response_ = resp;
}


void JobIOHandler::socketCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,socketid,cb);
    BufferString data;
    serversocket_.read( socketid, data );
    char tag=mCTRL_STATUS;
    int jobid=-1;
    int status=mSTAT_UNDEF;
    BufferString hostnm;
    BufferString errmsg;
    int procid=-1;

    if ( !data.isEmpty() )
    {
	SeparString statstr;
	bool ok = readTag( tag, statstr, data );

	if ( ok )
	{
	    getFromString( jobid, statstr[0] );
	    getFromString(status, statstr[1] );
	    hostnm = statstr[2];
	    getFromString( procid, statstr[3] );
	    errmsg = statstr[4];

	    char response = getRespFor( jobid, hostnm );
	    if ( response != mRSP_STOP )
	    {
		statusqueue_.add( new StatusInfo( tag, jobid, status,
			procid, errmsg, hostnm, Time::getMilliSeconds()) );
	    }

	    // hardly ever needed and quite noisy.
	    static bool debug_resp = GetEnvVarYN("DTECT_MM_DEBUG_RESP");
	    if ( debug_resp && mDebugOn )
	    {
		BufferString msg("JobIOMgr::JobIOMgr: Response to host ");
		msg += hostnm; msg += ", job: "; msg += jobid;
		msg += ": ";	    
		if ( response == mRSP_STOP )
		    msg +=  "STOP";
		else if ( response == mRSP_WORK )
		    msg +=  "WORK";
		else if ( response == mRSP_PAUSE )
		    msg +=  "PAUSE";
		else
		    msg += &response;
		DBG::message(msg);
	    }
	    char resp[2];
	    resp[0] = response;
	    resp[1] = '\0';
	    serversocket_.write( socketid, resp );
	}
	else
	{
	    errmsg = serversocket_.errorMsg();
	    ErrMsg( errmsg );
	}
    }
    

}


bool JobIOHandler::readTag( char& tag, SeparString& sepstr,
			    const BufferString& data )
{
    tag = data[0];
    sepstr.setSepChar( data[1] );
    sepstr = data.buf() + 2;
    return true;
}


JobIOMgr::JobIOMgr( int firstport, int niceval )
    : iohdlr_(*new JobIOHandler(firstport))
    , niceval_(niceval)
{
    for ( int count=0; count < 10 && !iohdlr_.ready(); count++ )
	{ Threads::sleep( 0.1 ); }

    if ( mDebugOn )
    {
	BufferString msg("JobIOMgr::JobIOMgr ");
	if( iohdlr_.ready() )
	{ 
	    msg += "ready and listening to port ";
	    msg += iohdlr_.port();
	}
	else
	{
	    msg += "NOT ready (yet). Clients might not be able to connect.";
	}

	DBG::message(msg);
    }
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

bool JobIOMgr::startProg( const char* progname,
	IOPar& iop, const FilePath& basefp, const JobInfo& ji,
	const char* rshcomm )
{
    DBG::message(DBG_MM,"JobIOMgr::startProg");
    if ( !ji.hostdata_ )
	mErrRet("Internal: No hostdata provided")
    const HostData& machine = *ji.hostdata_;

    FilePath ioparfp;
    if ( !mkIOParFile( ioparfp, basefp, machine, iop ) )
	return false;
    
    CommandString cmd;
    mkCommand( cmd, machine, progname, basefp, ioparfp, ji, rshcomm );

    iohdlr_.addJobDesc( machine, ji.descnr_ );

    if ( mDebugOn )
    {
	BufferString msg("Executing: ");
	msg += cmd.string();
	DBG::message(msg);
    }

    StreamProvider strmprov( cmd.string() );
    if ( !strmprov.executeCommand(true) ) // true: in background
    {
	BufferString s( "Failed to submit command '" );
	s += strmprov.command(); s += "'";

	iohdlr_.removeJobDesc( machine.name(), ji.descnr_ );
	mErrRet(s)
    }
    
    return true;
}

#ifdef __win__

extern const BufferString& getTempBaseNm();

extern int& MMJob_getTempFileNr();

static FilePath getConvertedFilePath( const HostData& hd, const FilePath& fp )
{
    FilePath newfp = hd.prefixFilePath( HostData::Data );
    if ( !newfp.nrLevels() ) return fp;
    
    BufferString proc( getTempBaseNm() );
    proc += "_";
    proc += MMJob_getTempFileNr()-1;
     newfp.add(  GetSurveyName() ).add( "Proc" )
    .add( proc ).add( fp.fileName() );
    return newfp;
}


bool JobIOMgr::mkIOParFile( FilePath& iopfp, const FilePath& basefp,
			    const HostData& machine, const IOPar& iop )
{
    FilePath remotefp = getConvertedFilePath( machine, basefp );
    iopfp = basefp; iopfp.setExtension( ".par", false );
    const BufferString iopfnm( iopfp.fullPath() );

    BufferString bs( remotefp.fullPath() );
    replaceCharacter( bs.buf(), '.',  '_' );
    FilePath logfp( bs ); 

    remotefp.setExtension( ".par", false );
    
    logfp.setExtension( ".log", false );
    const BufferString logfnm( logfp.fullPath(machine.pathStyle()) );

    FilePath remotelogfnm( machine.convPath( HostData::Data, logfp ));

    IOPar newiop( iop );
    newiop.set( sKey::LogFile(), remotelogfnm.fullPath(machine.pathStyle()) );

    FilePath remdata = machine.prefixFilePath(HostData::Data);

    const char* tmpstor = iop.find( sKey::TmpStor() );
    if ( tmpstor )
    {
	FilePath path = machine.convPath( HostData::Data, tmpstor );
	FilePath remotetmpdir( remdata.nrLevels() ? remdata.fullPath() 
						  : path.fullPath() );
	if ( remdata.nrLevels() )
	{
	    remotetmpdir.add(  GetSurveyName() ).add( "Seismics" )
			.add( path.fileName() );
	}
    
	newiop.set( sKey::TmpStor(), remotetmpdir.fullPath(machine.pathStyle()) );
    }

    newiop.set( sKey::DataRoot(), remdata.fullPath(machine.pathStyle()) );
    newiop.set( sKey::Survey(), IOM().surveyName() );

    if ( File::exists(iopfnm) ) File::remove( iopfnm );
    if ( File::exists(logfnm) ) File::remove( logfnm );

    StreamData iopsd = StreamProvider(iopfnm).makeOStream();
    if ( !iopsd.usable() )
    {
	BufferString s( "Cannot open '" );
	s += iopfnm; s += "' for write ...";
	mErrRet(s)
    }
    bool res = newiop.write( *iopsd.ostrm, sKey::Pars() );
    iopsd.close();
    if ( !res )
    {
	BufferString s( "Cannot write parameters into '" );
	s += iopfnm; s += "'";
	mErrRet(s)
    }

    iopfp.set( remotefp.fullPath(machine.pathStyle()) );
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

    FilePath remotelogfnm( machine.convPath( HostData::Data, logfp ));

    IOPar newiop( iop );
    newiop.set( sKey::LogFile(), remotelogfnm.fullPath(machine.pathStyle()) );

    const char* tmpstor = iop.find( sKey::TmpStor() );
    if ( tmpstor )
    {
	FilePath remotetmpdir = machine.convPath( HostData::Data, tmpstor );
	newiop.set( sKey::TmpStor(), remotetmpdir.fullPath(machine.pathStyle()) );
    }

    newiop.set( sKey::Survey(), IOM().surveyName() );

    if ( File::exists(iopfnm) ) File::remove( iopfnm );
    if ( File::exists(logfnm) ) File::remove( logfnm );

    StreamData iopsd = StreamProvider(iopfnm).makeOStream();
    if ( !iopsd.usable() )
    {
	BufferString s( "Cannot open '" );
	s += iopfnm; s += "' for write ...";
	mErrRet(s)
    }
    bool res = newiop.write( *iopsd.ostrm, sKey::Pars() );
    iopsd.close();
    if ( !res )
    {
	BufferString s( "Cannot write parameters into '" );
	s += iopfnm; s += "'";
	mErrRet(s)
    }

    return true;
}

#endif


void JobIOMgr::mkCommand( CommandString& cmd, const HostData& machine,
			  const char* progname, const FilePath& basefp,
			  const FilePath& iopfp, const JobInfo& ji,
			  const char* rshcomm )
{
    const bool remote = !machine.isKnownAs( HostData::localHostName() );

    cmd = "@";

#ifdef __msvc__
    BufferString remhostaddress = System::hostAddress( machine.name() );
    if ( remhostaddress != System::localAddress() )//do not use rem exec if host is local
    {
	cmd.add( "od_remexec" );
	cmd.add( machine.name() );
    }
    cmd.add( progname );
    cmd.addFlag( "-masterhost", GetLocalIP() ); 
    cmd.addFlag( "-masterport", iohdlr_.port() );
    cmd.addFlag( "-jobid", ji.descnr_ );
    FilePath parfp( iopfp );
    cmd.addFilePath( parfp, FilePath::Windows );

#else
    cmd.addWoSpc( GetExecScript(remote) );

    if ( remote )
    {
	cmd.add( machine.name() );

	cmd.addFlag( "--rexec", rshcomm ); // rsh/ssh/rcmd

	if ( machine.isWin()  ) cmd.add( "--iswin" );

	cmd.addFilePathFlag( "--with-dtect-appl", 
			     machine.convPath(HostData::Appl,GetSoftwareDir(0)),
			     FilePath::Unix );

	cmd.addFilePathFlag( "--with-dtect-data", 
			     machine.convPath(HostData::Data, GetBaseDataDir()),
			     FilePath::Unix );

	cmd.addFilePathFlag( "--with-local-file-base", basefp, FilePath::Unix);
	cmd.addFilePathFlag( "--with-remote-file-base",
	    machine.convPath(HostData::Data, basefp), FilePath::Unix );

	if ( machine.isWin() && machine.shareData() )
	{
	    const ShareData& sd = *machine.shareData();
	    cmd.addFlag( "--data-host", sd.hostName() );
	    cmd.addFlag( "--data-drive", sd.drive() );
	    cmd.addFlag( "--data-share", sd.share() );
	    cmd.addFlag( "--remotepass", sd.pass() );
	}
    }

    cmd.addFlag( "--nice", niceval_ );
    cmd.addFlag( "--inbg", progname );

    cmd.addFlag( "-masterhost", HostData::localHostName() ); 
    cmd.addFlag( "-masterport", iohdlr_.port() );
    cmd.addFlag( "-jobid", ji.descnr_ );

   
    FilePath riopfp( remote ? machine.convPath( HostData::Data, iopfp )
			     : iopfp );
   
    bool winstyle = machine.isWin() && rshcomm == FixedString("rcmd");
    
    cmd.addFilePath( riopfp, winstyle ? FilePath::Windows : FilePath::Unix );

#endif
}
