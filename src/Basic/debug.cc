/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          June 2003
________________________________________________________________________

-*/


#include "debug.h"
#include "atomic.h"
#include "genc.h"
#include "bufstring.h"
#include "timefun.h"
#include "filepath.h"
#include "file.h"
#include "envvars.h"
#include "legal.h"
#include "oddirs.h"
#include "ptrman.h"
#include "sighndl.h"
#include "undefval.h"
#include "math2.h"
#include "odplatform.h"
#include "oscommand.h"
#include "odcomplex.h"
#include "moddepmgr.h"
#include "od_ostream.h"
#include "msgh.h"
#include "uistring.h"
#include "fixedstring.h"

#include <iostream>
#include <signal.h>

#ifdef HAS_BREAKPAD

#include <QString>

#ifdef __win__
#include "client/windows/handler/exception_handler.h"
#endif

#ifdef __lux__
#include "client/linux/handler/exception_handler.h"
#endif

#ifdef __mac__
#include "client/mac/handler/exception_handler.h"
#endif

#endif


namespace OD { Export_Basic od_ostream& logMsgStrm(); class ForcedCrash {}; }
Export_Basic int gLogFilesRedirectCode = -1; // 0 = stderr, 1 = log file
static bool crashonprogerror = false;
static PtrMan<od_ostream> dbglogstrm = 0;


namespace OD
{

class StaticStringRepos
{
public:
    void			addIfNew( const OD::String* str )
				{
				    Threads::Locker locker( lock_ );
				    strings_.addIfNew( str );
				}

    bool			isPresent( const OD::String* str ) const
				{
				    Threads::Locker locker( lock_ );
				    return strings_.isPresent(str);
				}

private:

    ObjectSet<const OD::String>	strings_;
    mutable Threads::Lock	lock_;

};

} // namespace OD


static OD::StaticStringRepos getStaticStringRepos()
{
    mDefineStaticLocalObject( OD::StaticStringRepos, repos, );
    return repos;
}


Export_Basic void addToStaticStringRepos( const OD::String* str )
{
    getStaticStringRepos().addIfNew( str );
}


Export_Basic bool isStaticString( const OD::String* str )
{
    return getStaticStringRepos().isPresent( str );
}

void od_test_prog_crash_handler(int)
{
    std::cout << "Program crashed.\n";
    exit( 1 );
}


void od_init_test_program(int argc, char** argv )
{
    SetProgramArgs( argc, argv, false );
    signal(SIGSEGV, od_test_prog_crash_handler );
    DBG::setCrashOnProgError( true );

    OD::ModDeps().ensureLoaded("Basic");
}



template <class fT>
static bool isUdfImplImpl( fT val )
{
    const bool isnorm = Math::IsNormalNumber( val );

#ifdef __debug__
    if ( !isnorm )
    {
	if ( DBG::crashOnNaN() )
	{
	    pFreeFnErrMsg("Bad fp value found" );
	    DBG::forceCrash(false); return true;
	}
    }
#endif

    return !isnorm || Values::isUdf( val );
}


bool isUdfImpl( float val )
{ return isUdfImplImpl(val); }

bool isUdfImpl( double val )
{ return isUdfImplImpl(val); }


bool isUdfImpl( float_complex val )
{
    return isUdfImplImpl(val.real()) || isUdfImplImpl(val.imag());
}


namespace DBG
{


bool setCrashOnProgError( bool yn )
{
    const bool res = crashonprogerror;
    crashonprogerror = yn;
    return res;
}


bool crashOnNaN()
{
    bool defval = true;
#ifdef __debug__
    defval = false;
#endif
    mDefineStaticLocalObject( bool, dohide,
		(GetEnvVarYN("OD_DONT_CRASH_ON_NOT_NORMAL_NUMBER",defval)));
    return !dohide;
}

static bool maskgot = false;
static int themask = 0;

static int getMask()
{
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


void turnOn( int mask )
{
    themask = mask;
    maskgot = true;
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
    {
	throw( OD::ForcedCrash() );
    }
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
	std::cerr << msg_.buf() << std::endl;
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
	File::Path fp( argv[0] );
	msg = fp.fileName();
    }
    msg += " started on "; msg += GetLocalHostName();
    msg += " at "; msg += Time::getISOUTCDateTimeString();
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

};


namespace OD {

Export_Basic od_ostream& logMsgStrm()
{
    mDefineStaticLocalObject( PtrMan<od_ostream>, strm, = 0 );
    if ( strm )
	return *strm;

    BufferString errmsg;
    if ( gLogFilesRedirectCode > 0 && !GetEnvVarYN("OD_LOG_STDERR") )
    {
	const char* basedd = GetBaseDataDir();
	if ( !basedd || !*basedd )
	    errmsg = "Directory for data storage is not set\n";
	else if ( !File::isDirectory(basedd) )
	    errmsg = "Directory for data storage is invalid\n";
	else
	{
	    File::Path fp( basedd, "LogFiles" );
	    const BufferString dirnm = fp.fullPath();
	    if ( !File::exists(dirnm) )
		File::createDir( dirnm );
	    if ( !File::isDirectory(dirnm) )
		errmsg = "Cannot create proper directory for log file";
	    else
	    {
		const File::Path pfp( GetPersonalDir() );
		BufferString fnm( pfp.fileName() );
		const char* odusr = GetSoftwareUser();
		if ( odusr && *odusr )
		    { fnm += "_"; fnm += odusr; }
		fnm += "_";
		fp.add( fnm.add(File::Path::getTimeStampFileName(".txt")) );

		BufferString logmsgfnm = fp.fullPath();

		strm = new od_ostream( logmsgfnm );
		if ( !strm->isOK() )
		{
		    errmsg.set( "Cannot create log file '" )
			  .add( logmsgfnm ).add( "'" );
		    strm = 0;
		}
	    }
	}
    }

    if ( !strm )
    {
	strm = new od_ostream( std::cerr );
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


void UsrMsg( const uiString& msg, MsgClass::Type t )
{
    const BufferString str( toString(msg) );
    UsrMsg( str, t );
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


void ErrMsg( const uiString& msg )
{
    ErrMsg( toString(msg), false );
}


void ErrMsg( const char* msg, bool progr )
{
    if ( !MsgClass::theCB().willCall() )
    {
	mDefineStaticLocalObject( bool, wantsilence,
			= GetEnvVarYN("OD_PROGRAMMER_ERRS_SILENCE",false) );
	if ( progr )
	{
	    if ( !wantsilence )
		std::cerr << "(PE) " << msg << std::endl;
	}
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

CrashDumper::CrashDumper()
    : handler_(0)
{
    init();
}


CrashDumper* CrashDumper::theinst_ = 0;

static uiString* legalText();
static const char* breakpadname = "Google Breakpad";

static Threads::Atomic<int> dumpsent( 0 );

#ifdef __win__

static bool MinidumpCB( const wchar_t* dump_path, const wchar_t* id,
			void* context, EXCEPTION_POINTERS *exinfo,
			MDRawAssertionInfo *assertion, bool succeeded )
{
    if ( !dumpsent.setIfValueIs(0,1,0) )
	return succeeded;

    const BufferString dmppath( QString::fromWCharArray(dump_path) );
    const BufferString dmpid( QString::fromWCharArray(id) );
    File::Path dmpfp( dmppath, dmpid );
    dmpfp.setExtension( "dmp" );
    CrashDumper::getInstance().sendDump( dmpfp.fullPath() );
    return succeeded;
}


void CrashDumper::init()
{
    if ( handler_ )
	return;

    const QString dmppath = File::Path::getTempDir();
    const std::wstring wpath = dmppath.toStdWString();
    handler_ = new google_breakpad::ExceptionHandler(
		    wpath, NULL, MinidumpCB, NULL,
		    google_breakpad::ExceptionHandler::HANDLER_ALL );
    legalInformation().addCreator( legalText, breakpadname );
}

#endif // __win__

#ifdef __lux__

static bool MinidumpCB( const google_breakpad::MinidumpDescriptor& minidumpdesc,
			void* context, bool succeeded )
{
    if ( !dumpsent.setIfValueIs(0,1,0) )
	return succeeded;

    File::Path dmpfp( minidumpdesc.path() );
    System::CrashDumper::getInstance().sendDump( dmpfp.fullPath() );
    return succeeded;
}


void CrashDumper::init()
{
    if ( handler_ )
	return;

    const BufferString dmppathbuf = File::Path::getTempDir();
    const google_breakpad::MinidumpDescriptor minidumpdesc( dmppathbuf.buf() );
    handler_ = new google_breakpad::ExceptionHandler(
		    minidumpdesc, NULL, MinidumpCB, NULL,
		    true, -1 );
    legalInformation().addCreator( legalText, breakpadname );
}

#endif // __lux__

void CrashDumper::sendDump( const char* filename )
{
    if ( !File::exists(filename) )
	return;

    const BufferString processscript =
#if defined( __win__ )
	"process_dumpfile.cmd";
#else
	"process_dumpfile.sh";
#endif

    const File::Path script( GetShellScript(processscript) );
    const File::Path symboldir( GetExecPlfDir(), "symbols" );

#ifdef __win__
    const File::Path dumphandler(GetExecPlfDir(), "minidump_stackwalk.exe");
#else
    const File::Path dumphandler( GetExecPlfDir(), "minidump_stackwalk" );
#endif

    const BufferString prefix =  File::Path( GetArgV()[0] ).baseName();

    BufferString cmd( "\"",script.fullPath(), "\"" );
    cmd += BufferString( " \"", filename, "\"" );
    cmd += BufferString( " \"", symboldir.fullPath(), "\"" );
    cmd += BufferString( " \"", dumphandler.fullPath(), "\"" );
    cmd += BufferString( " ", prefix );
    if ( !sendappl_.isEmpty() )
	cmd += BufferString( " \"",
		File::Path(GetExecPlfDir(),sendappl_).fullPath(), "\"" );

    std::cout << cmd.str() << std::endl;

    ExecCommand( cmd, OS::RunInBG );
}


CrashDumper& CrashDumper::getInstance()
{
    if ( !theinst_ )
    {
	theinst_ = new CrashDumper;

	const char* crashspec = GetEnvVar( "DTECT_FORCE_IMMEDIATE_DUMP" );
	if ( crashspec && *crashspec )
	{
	    DBG::forceCrash( false );
	}
    }

    return *theinst_;
}


//!Obsolete since we don't do non-ui
FixedString CrashDumper::sSenderAppl()
{ return FixedString("" ); }

FixedString CrashDumper::sUiSenderAppl()
{
#ifdef __win__
    return FixedString( "od_uiReportIssue.exe" );
#else
    return FixedString( "od_uiReportIssue" );
#endif
}



static uiString* legalText()
{
    uiString* res = new uiString;
    *res = toUiString(
"Copyright (c) 2006, Google Inc.\n"
"All rights reserved.\n"
"\n"
"Redistribution and use in source and binary forms, with or without\n"
"modification, are permitted provided that the following conditions are\n"
"met:\n"
"\n"
"    * Redistributions of source code must retain the above copyright\n"
"      notice, this list of conditions and the following disclaimer.\n"
"    * Redistributions in binary form must reproduce the above\n"
"      copyright notice, this list of conditions and the following disclaimer\n"
"      in the documentation and/or other materials provided with the\n"
"      distribution.\n"
"    * Neither the name of Google Inc. nor the names of its\n"
"      contributors may be used to endorse or promote products derived from\n"
"      this software without specific prior written permission.\n"
"\n"
"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n"
"\"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n"
"LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n"
"A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT\n"
"OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,\n"
"SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT\n"
"LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,\n"
"DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\n"
"THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n"
"(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n"
"OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n"
"\n"
"--------------------------------------------------------------------\n"
"\n"
"Copyright 2001-2004 Unicode, Inc.\n"
"\n"
"Disclaimer\n"
"\n"
"This source code is provided as is by Unicode, Inc. No claims are\n"
"made as to fitness for any particular purpose. No warranties of any\n"
"kind are expressed or implied. The recipient agrees to determine\n"
"applicability of information provided. If this file has been\n"
"purchased on magnetic or optical media from Unicode, Inc., the\n"
"sole remedy for any claim will be exchange of defective media\n"
"within 90 days of receipt.\n"
"\n"
"Limitations on Rights to Redistribute This Code\n"
"\n"
"Unicode, Inc. hereby grants the right to freely use the information\n"
"supplied in this file in the creation of products supporting the\n"
"Unicode Standard, and to make copies of this file in any form\n"
"for internal or external distribution as long as this notice\n"
"remains attached.");
    return res;
}

} // namespace System



#endif // mUseCrashDumper
