/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          Oct 2004
 RCS:           $Id: jobiomgr.cc,v 1.2 2004-11-03 16:20:16 arend Exp $
________________________________________________________________________

-*/

#include "jobiomgr.h"
#include "filepath.h"
#include "debugmasks.h"
#include "strmprov.h"
#include "ioparlist.h"
#include "filegen.h"
#include "hostdata.h"
#include "ioman.h"
#include "jobinfo.h"
#include "jobiohdlr.h"


#define sTmpStor	"Temporary storage directory"
#define sLogFil		"Log file"
#define sSurvey		"Survey"

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
			    cmd += " '";
			    cmd += fp.fullPathStyled(stl);
			    cmd += "'";
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


JobIOMgr::JobIOMgr( int firstport, int niceval )
    : hdl_( *new HostDataList )
    , iohdlr_( *new JobIOHandler(firstport) )
    , niceval_( niceval )
{}

JobIOMgr::~JobIOMgr()
{ delete &hdl_; delete &iohdlr_; }

ObjQueue<StatusInfo>& JobIOMgr::statusQueue() { return iohdlr_.statusQueue(); }

bool JobIOMgr::startProg( const char* progname, const HostData& machine,
	IOPar& iop, const FilePath& basefp, const JobInfo& ji )
{
    DBG::message(DBG_MM,"JobIOMgr::startProg");

    FilePath ioparfp;
    if ( !mkIOParFile( ioparfp, basefp, machine, iop ) )
	return false;

    CommandString cmd;
    mkCommand( cmd, machine, progname, basefp, ioparfp, ji );

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
	mErrRet(s);
    }

    return true;
}


bool JobIOMgr::mkIOParFile( FilePath& iopfp, const FilePath& basefp,
			    const HostData& machine, const IOPar& iop )
{
    IOParList iopl;
    iopl.deepErase(); // Protect against a file '.Parameters' in survdir
    IOPar* newiop = new IOPar( iop ); iopl += newiop;

    iopfp = basefp; iopfp.setExtension( ".par", false );
    FilePath logfnm(basefp); logfnm.setExtension( ".log", false );

    FilePath remotelogfnm( machine.convPath( HostData::Data, logfnm ));

    newiop->set( sLogFil, remotelogfnm.fullPathStyled(machine.pathStyle()) );

    if ( newiop->find( sTmpStor ) )
    {
	FilePath remotetmpdir = machine.convPath( HostData::Data,
						  newiop->find( sTmpStor ) );

	newiop->set( sTmpStor,
		remotetmpdir.fullPathStyled( machine.pathStyle() ) );
    }

    newiop->set( sSurvey, IOM().surveyName() );


    if ( File_exists(iopfp.fullPath()) )
	File_remove( iopfp.fullPath(), NO );
    if ( File_exists(logfnm.fullPath()) )
	File_remove( logfnm.fullPath(), NO );

    StreamData ioplsd = StreamProvider(iopfp.fullPath()).makeOStream();
    if ( !ioplsd.usable() )
    {
	BufferString s( "Cannot open '" );
	s += iopfp.fullPath(); s += "' for write ...";
	mErrRet(s);
    }
    bool res = iopl.write( *ioplsd.ostrm );
    ioplsd.close();
    if ( !res )
    {
	BufferString s( "Cannot write parameters into '" );
	s += iopfp.fullPath(); s += "'";
	mErrRet(s);
    }

    return true;
}


void JobIOMgr::mkCommand( CommandString& cmd, const HostData& machine,
			  const char* progname, const FilePath& basefp,
			  const FilePath& iopfp, const JobInfo& ji )
{
    const bool remote = machine.name() && *machine.name()
			&& !hdl_[0]->isKnownAs(machine.name());

    cmd = "@";
    cmd.addWoSpc( GetExecScript(remote) );

    if ( remote )
    {
	cmd.add( machine.name() );

	cmd.addFlag( "--rexec", hdl_.rshComm() ); // rsh/ssh/rcmd

	if ( machine.isWin()  ) cmd.add( "--iswin" );

	cmd.addFilePathFlag( "--with-dtect-appl", 
			     machine.convPath(HostData::Appl, GetSoftwareDir()),
			     FilePath::Unix );

	cmd.addFilePathFlag( "--with-dtect-data", 
			     machine.convPath(HostData::Appl, GetBaseDataDir()),
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
    cmd.addFilePath( riopfp, machine.pathStyle() );
}
