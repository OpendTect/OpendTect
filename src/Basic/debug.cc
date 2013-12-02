/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          June 2003
 RCS:           $Id$
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "debug.h"
#include "genc.h"
#include "bufstring.h"
#include "timefun.h"
#include "filepath.h"
#include "file.h"
#include "envvars.h"
#include "oddirs.h"
#include "sighndl.h"
#include "undefval.h"
#include "math2.h"
#include "odplatform.h"
#include "oscommand.h"
#include "odcomplex.h"
#include "moddepmgr.h"
#include "od_ostream.h"
#include "msgh.h"
#include "fixedstring.h"

#include <iostream>
#include <signal.h>

#if defined ( __msvc__ )  && defined ( HAS_BREAKPAD )
# define mUseCrashDumper yes
#include "client\windows\handler\exception_handler.h"
#include <QString>
#endif


namespace OD { Export_Basic od_ostream& logMsgStrm(); }
Export_Basic int gLogFilesRedirectCode = -1; // 0 = stderr, 1 = log file
static bool crashonprogerror = false;
static PtrMan<od_ostream> dbglogstrm = 0;
namespace google_breakpad { class ExceptionHandler; }



void od_test_prog_crash_handler(int)
{
    std::cout << "Program crashed.\n";
    exit( 1 );
}


void od_init_test_program(int argc, char** argv )
{
    SetProgramArgs( argc, argv );
    signal(SIGSEGV, od_test_prog_crash_handler );
    crashonprogerror = true;

    OD::ModDeps().ensureLoaded("Basic");
}



bool isUdfImpl( float val )
{
#ifdef __debug__
    if ( !Math::IsNormalNumber(val) )
    {
	if ( DBG::crashOnNaN() )
	{
	    pFreeFnErrMsg("Bad fp value found","dbgIsUdf(f)");
	    DBG::forceCrash(false); return true;
	}
    }
#endif

    return Values::isUdf( val );
}


bool isUdfImpl( double val )
{
#ifdef __debug__
    if ( !Math::IsNormalNumber(val) )
    {
	if ( DBG::crashOnNaN() )
	{
	    pFreeFnErrMsg("Bad fp value found","dbgIsUdf(d)");
	    DBG::forceCrash(false); return true;
	}
    }
#endif

    return Values::isUdf( val );
}


bool isUdfImpl( float_complex val )
{
#ifdef __debug__
    if ( !Math::IsNormalNumber(val.real()) || !Math::IsNormalNumber(val.imag()))
    {
	if ( DBG::crashOnNaN() )
	{
	    pFreeFnErrMsg("Bad fp value found","dbgIsUdf(fc)");
	    DBG::forceCrash(false);
	    return true;
	}
    }
#endif

    return Values::isUdf( val );
}


namespace DBG
{


bool crashOnNaN()
{
#ifdef __debug__
    mDefineStaticLocalObject( bool, dohide,
                     (GetEnvVarYN("OD_DONT_CRASH_ON_NOT_NORMAL_NUMBER", 0)));
#else
    mDefineStaticLocalObject( bool, dohide,
                     (GetEnvVarYN("OD_DONT_CRASH_ON_NOT_NORMAL_NUMBER", 1)));
#endif

    return !dohide;
}


static int getMask()
{
    mDefineStaticLocalObject( bool, maskgot, = false );
    mDefineStaticLocalObject( int, themask, = 0 );
    if ( maskgot ) return themask;
    maskgot = true;

    BufferString envmask = GetEnvVar( "DTECT_DEBUG" );
    const char* buf = envmask.buf();
    themask = toInt( buf );
    if ( toBool(buf,false) ) themask = 0xffff;

    const char* dbglogfnm = GetEnvVar( "DTECT_DEBUG_LOGFILE" );
    if ( dbglogfnm && !themask )
	themask = 0xffff;

    if ( themask )
    {
	BufferString msg;
	if ( dbglogfnm )
	{
	    dbglogstrm = new od_ostream( dbglogfnm );
	    if ( dbglogstrm && !dbglogstrm->isOK() )
	    {
		dbglogstrm = 0;
		msg = "Cannot open debug log file '";
		msg += dbglogfnm;
		msg += "': reverting to stdout";
		message( msg );
	    }
	}
    }

    const char* crashspec = GetEnvVar( "DTECT_FORCE_IMMEDIATE_CRASH" );
    if ( crashspec && *crashspec )
	forceCrash( *crashspec == 'd' || *crashspec == 'D' );

    return themask;
}


bool isOn( int flag )
{
    if ( !GetSoftwareDir(1) )	//Check if we are in an od environment at all
	return false;

    const int mask = getMask();
    return flag & mask;
}


void forceCrash( bool withdump )
{
    if ( withdump )
	SignalHandling::SH().doStop( 6, false ); // 6 = SIGABRT
    else
	{ char* ptr = 0; *ptr = 0; }
}


void message( const char* msg )
{
    if ( !isOn() ) return;

    BufferString msg_;
    mDefineStaticLocalObject( bool, wantpid,
			      = GetEnvVarYN("DTECT_ADD_DBG_PID") );
    if ( wantpid )
    {
	msg_ = "[";
	msg_ += GetPID();
	msg_ += "] ";
    }
    msg_ += msg;

    if ( dbglogstrm )
	*dbglogstrm.ptr() << msg_ << od_endl;
    else
	std::cerr << msg_ << std::endl;
}


void message( int flag, const char* msg )
{
    if ( isOn(flag) ) message(msg);
}

void putProgInfo( int argc, char** argv )
{
    if ( GetEnvVarYN("OD_NO_PROGINFO") ) return;

    const bool ison = isOn( DBG_PROGSTART );

    BufferString msg;
    if ( ison )
    {
	msg = "\n---------\n";
	msg += argv[0];
    }
    else
    {
	FilePath fp( argv[0] );
	msg = fp.fileName();
    }
    msg += " started on "; msg += GetLocalHostName();
    msg += " at "; msg += Time::getDateTimeString();
    if ( !ison ) msg += "\n";
    message( msg );
    if ( !ison ) return;

    msg = "PID: "; msg += GetPID();
    msg += "; Platform: "; msg += OD::Platform::local().longName();

#ifdef __GNUC__
    msg += "; gcc ";
    msg += __GNUC__; msg += "."; msg += __GNUC_MINOR__;
    msg += "."; msg += __GNUC_PATCHLEVEL__;
#endif
    message( msg );

    if ( argc > 1 )
    {
	msg = "Program args:";
	for ( int idx=1; idx<argc; idx++ )
	    { msg += " '"; msg += argv[idx]; msg += "'"; }
	message( msg );
    }
    message( "---------\n" );
}

} // namespace DBG


extern "C" int od_debug_isOn( int flag )
    { return DBG::isOn(flag); }
extern "C" void od_debug_message( const char* msg )
    { DBG::message(msg); }
extern "C" void od_debug_messagef( int flag, const char* msg )
    { DBG::message(flag,msg); }
extern "C" void od_debug_putProgInfo( int argc, char** argv )
    { DBG::putProgInfo(argc,argv); }

extern "C" void od_putProgInfo( int argc, char** argv )
{ od_debug_putProgInfo(argc,argv); }



mExpClass(Basic) ErrMsgClass : public MsgClass
{
public:

			ErrMsgClass( const char* s, bool p )
			: MsgClass(s,p?ProgrammerError:Error)	{}

    static bool		printProgrammerErrs;

};

bool ErrMsgClass::printProgrammerErrs =
# ifdef __debug__
    true;
# else
    false;
# endif



namespace OD {

Export_Basic od_ostream& logMsgStrm()
{
    mDefineStaticLocalObject( od_ostream*, strm, = 0 );
    if ( strm )
	return *strm;

    BufferString errmsg;
    if ( gLogFilesRedirectCode > 0 && !GetEnvVarYN("OD_LOG_STDERR") )
    {
	const char* basedd = GetBaseDataDir();
	if ( !File::isDirectory(basedd) )
	    errmsg = "Directory for data storage is invalid";
	else
	{
	    FilePath fp( basedd, "LogFiles" );
	    const BufferString dirnm = fp.fullPath();
	    if ( !File::exists(dirnm) )
		File::createDir( dirnm );
	    if ( !File::isDirectory(dirnm) )
		errmsg = "Cannot create proper directory for log file";
	    else
	    {
		const FilePath pfp( GetPersonalDir() );
		BufferString fnm( pfp.fileName() );
		const char* odusr = GetSoftwareUser();
		if ( odusr && *odusr )
		    { fnm += "_"; fnm += odusr; }
		BufferString datestr = Time::getDateTimeString();
		datestr.replace( ", ", "-" );
		datestr.replace( ':', '.' );
		datestr.replace( ' ', '_' );
		fnm += "_"; fnm += datestr.buf();
		fnm += ".txt";

		fp.add( fnm );
		BufferString logmsgfnm = fp.fullPath();
		strm = new od_ostream( logmsgfnm );
		if ( !strm->isOK() )
		{
		    errmsg.set( "Cannot create log file '" )
			  .add( logmsgfnm ).add( "'" );
		    delete strm; strm = 0;
		}
	    }
	}
    }

    if ( !strm )
    {
#	ifdef __win__
	    strm = &od_ostream::nullStream();
#	else
	    strm = new od_ostream( std::cerr );
#	endif
	if ( !errmsg.isEmpty() )
	    *strm << errmsg;
    }

    return *strm;
}


Export_Basic void programmerErrMsg( const char* inpmsg, const char* cname,
				    const char* fname, int linenr )
{
    BufferString msg( cname, " | " );
    msg.add( fname ).add( ":" ).add( linenr )
	.add( " >> " ).add( inpmsg ).add( " <<" );
    ErrMsg( msg.buf(), true );
}


}


void UsrMsg( const char* msg, MsgClass::Type t )
{
    if ( !MsgClass::theCB().willCall() )
	OD::logMsgStrm() << msg << od_endl;
    else
    {
	MsgClass obj( msg, t );
	MsgClass::theCB().doCall( &obj );
    }
}




void ErrMsg( const char* msg, bool progr )
{
    if ( progr && !ErrMsgClass::printProgrammerErrs )
	return;

    if ( !MsgClass::theCB().willCall() )
    {
	if ( progr )
	    std::cerr << "(PE) " << msg << std::endl;
	else if ( msg && *msg )
	{
	    const char* start = *msg == '[' ? "" : "Err: ";
	    OD::logMsgStrm() << start  << msg << od_endl;
	}
    }
    else
    {
	ErrMsgClass obj( msg, progr );
	MsgClass::theCB().doCall( &obj );
    }

    if ( progr && crashonprogerror )
	DBG::forceCrash( false );
}


CallBack& MsgClass::theCB( const CallBack* cb )
{
    mDefineStaticLocalObject( CallBack, thecb, );
    if ( cb ) thecb = *cb;
    return thecb;
}


const char* MsgClass::nameOf( MsgClass::Type typ )
{
    const char* strs[] =
	{ "Information", "Message", "Warning", "Error", "PE", 0 };
    return strs[ (int)typ ];
}


#ifdef mUseCrashDumper

namespace System
{

/*!Segmentation fault core dumper that sends dump to dGB. */

mExpClass(Basic) CrashDumper
{
public:
    static CrashDumper&	getInstance();
			//!Creates and installs at first run.

    void		sendDump(const char* filename);

    void		setSendAppl(const char* a)    { sendappl_ = a; }

    static FixedString	sSenderAppl();		//od_ReportIssue
    static FixedString	sUiSenderAppl();	//od_uiReportIssue

private:
					CrashDumper();

    void				init();

    static CrashDumper*			theinst_;

    BufferString			sendappl_;
    google_breakpad::ExceptionHandler*	handler_;
};

} // namespace System


using namespace System;

// CrashDumper
CrashDumper::CrashDumper()
    : sendappl_( sSenderAppl() )
    , handler_(0)
{
    init();
}


CrashDumper* CrashDumper::theinst_ = 0;


static bool MinidumpCB( const wchar_t* dump_path, const wchar_t* id,
			void* context, EXCEPTION_POINTERS *exinfo,
			MDRawAssertionInfo *assertion, bool succeeded )
{
    const BufferString dmppath( QString::fromWCharArray(dump_path) );
    const BufferString dmpid( QString::fromWCharArray(id) );
    FilePath dmpfp( dmppath, dmpid );
    dmpfp.setExtension( "dmp" );
    System::CrashDumper::getInstance().sendDump( dmpfp.fullPath() );
    return succeeded;
}


void CrashDumper::init()
{
    if ( handler_ )
	return;

    const QString dmppath = FilePath::getTempDir();
    const std::wstring wpath = dmppath.toStdWString();
    handler_ = new google_breakpad::ExceptionHandler(
		    wpath, NULL, MinidumpCB, NULL,
		    google_breakpad::ExceptionHandler::HANDLER_ALL );
}


void CrashDumper::sendDump( const char* filename )
{
    if ( sendappl_.isEmpty() || !File::exists(filename) )
	return;

    const BufferString cmd( sendappl_, " --binary ", filename );
    ExecOSCmd( cmd, true, true );
}


CrashDumper& CrashDumper::getInstance()
{
    if ( !theinst_ )
	theinst_ = new CrashDumper;

    return *theinst_;
}


FixedString CrashDumper::sSenderAppl()
{ return FixedString("od_ReportIssue" ); }

FixedString CrashDumper::sUiSenderAppl()
{ return FixedString( "od_uiReportIssue" ); }

#endif // mUseCrashDumper
