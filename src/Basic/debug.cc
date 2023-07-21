/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "debug.h"

#include "atomic.h"
#include "bufstring.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "genc.h"
#include "keystrs.h"
#include "legal.h"
#include "math2.h"
#include "moddepmgr.h"
#include "msgh.h"
#include "od_ostream.h"
#include "odcomplex.h"
#include "oddirs.h"
#include "odplatform.h"
#include "oscommand.h"
#include "ptrman.h"
#include "sighndl.h"
#include "stringview.h"
#include "timefun.h"
#include "uistring.h"
#include "undefval.h"

#include <iostream>
#include <signal.h>

#ifdef HAS_BREAKPAD

#include <QString>

#ifdef __win__
# include "client/windows/handler/exception_handler.h"
#endif

#ifdef __lux__
# include "client/linux/handler/exception_handler.h"
#endif

#ifdef __mac__
# include "client/mac/handler/exception_handler.h"
#endif

#endif


static const char* sStdIO = od_stream::sStdIO();
static const char* sStdErr = od_stream::sStdErr();
static BufferString log_file_name_ = sStdIO;
static bool crash_on_programmer_error_ = false;
static PtrMan<od_ostream> dbg_log_strm_ = nullptr;


namespace OD
{

void SetGlobalLogFile( const char* fnm )
{
    log_file_name_.set( fnm );
}

Export_Basic od_ostream& logMsgStrm();
class ForcedCrash {};

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


static OD::StaticStringRepos& getStaticStringRepos()
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


void od_init_test_program( int argc, char** argv, bool withdatabase )
{
    SetProgramArgs( argc, argv, withdatabase );
    signal( SIGSEGV, od_test_prog_crash_handler );
    DBG::setCrashOnProgError( true );

    OD::ModDeps().ensureLoaded( "Basic" );
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
    const bool res = crash_on_programmer_error_;
    crash_on_programmer_error_ = yn;
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

    const BufferString envmask = GetEnvVar( "DTECT_DEBUG" );
    if ( envmask.isEmpty() )
        themask = 0;
    else
    {
        themask = Conv::to<int>( envmask );
        if ( mIsUdf(themask) )
            themask = GetEnvVarYN( "DTECT_DEBUG" ) ? 0xffff : 0x0000;
    }

    const char* dbglogfnm = GetEnvVar( "DTECT_DEBUG_LOGFILE" );
    if ( dbglogfnm && !themask )
	themask = 0xffff;

    if ( themask )
    {
	BufferString msg;
	if ( dbglogfnm )
	{
	    dbg_log_strm_ = new od_ostream( dbglogfnm );
	    if ( dbg_log_strm_ && !dbg_log_strm_->isOK() )
	    {
		dbg_log_strm_ = nullptr;
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


static void printMessage( const char* inpmsg )
{
    BufferString msg;
    mDefineStaticLocalObject(bool, wantpid,
	= GetEnvVarYN("DTECT_ADD_DBG_PID"));
    if (wantpid)
	msg.add("[").add(GetPID()).add("] ");
    msg.add( inpmsg );

    if (dbg_log_strm_)
	*dbg_log_strm_ << msg << od_endl;
    else
	std::cerr << msg.buf() << std::endl;
}


void message( const char* inpmsg )
{
    if ( !isOn() )
	return;

    printMessage( inpmsg );
}


void message( int flag, const char* msg )
{
    if ( isOn(flag) )
	printMessage( msg );
}


static BufferString getDetailedMessage( const char* inpmsg, const char* cname,
    const char* fname, int linenr )
{
    BufferString msg( cname, " | " );
    msg.add( fname ).add(":").add( linenr )
	.add( " >> " ).add( inpmsg ).add( " <<" );

    return msg;
}


void message( const char* inpmsg, const char* cname,
	      const char* fname, int linenr )
{
    if ( !isOn() )
	return;

    printMessage( getDetailedMessage(inpmsg,cname,fname,linenr) );
}

void message( int flag,const char* inpmsg, const char* cname,
    const char* fname, int linenr )
{
    if ( isOn(flag) )
	printMessage(getDetailedMessage(inpmsg, cname, fname, linenr));
}


void putProgInfo( int argc, char** argv )
{
    if ( GetEnvVarYN("OD_NO_PROGINFO") )
	return;

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
    if ( !ison )
	msg += "\n";
    message( msg );
    if ( !ison )
	return;

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
    mDefineStaticLocalObject( PtrMan<od_ostream>, logstrm, = nullptr );
    if ( logstrm )
	return *logstrm;

    if ( GetEnvVarYN("OD_LOG_STDOUT") )
	log_file_name_.set( sStdIO );
    else if ( GetEnvVarYN("OD_LOG_STDERR") )
	log_file_name_.set( sStdErr );

    BufferString errmsg;
    if ( log_file_name_.isEmpty() )
    {
	const char* basedd = GetBaseDataDir();
	if ( !basedd || !*basedd )
	    errmsg = "Survey Data Root folder is not set\n";
	else if ( !File::isDirectory(basedd) )
	    errmsg = "Survey Data Root folder is invalid\n";
	else
	{
	    FilePath fp( basedd, "LogFiles" );
	    const BufferString dirnm = fp.fullPath();
	    if ( !File::exists(dirnm) )
		File::createDir( dirnm );
	    if ( !File::isDirectory(dirnm) )
		errmsg = "Cannot create proper folder for log file";
	    else
	    {
		const FilePath pfp( GetPersonalDir() );
		BufferString fnm( pfp.fileName() );
		const char* odusr = GetSoftwareUser();
		if ( odusr && *odusr )
		    { fnm += "_"; fnm += odusr; }
		fnm += "_";
		fp.add( fnm.add(FilePath::getTimeStampFileName(".txt")) );

		log_file_name_ = fp.fullPath();
	    }
	}
    }

    if ( !log_file_name_.isEmpty() && log_file_name_ != sStdErr )
    {
	logstrm = new od_ostream( log_file_name_ );
	if ( !logstrm->isOK() )
	{
	    errmsg.set( "Cannot create log file '" )
		  .add( log_file_name_ ).add( "'" );
	    logstrm = nullptr;
	}
    }

    if ( !logstrm )
    {
	logstrm = new od_ostream( std::cout );
	if ( !errmsg.isEmpty() )
	    *logstrm << errmsg;
    }

    return *logstrm;
}


Export_Basic void programmerErrMsg( const char* inpmsg, const char* cname,
				    const char* fname, int linenr )
{
    bool isdebug = DBG::isOn();
#ifdef __debug__
    isdebug = true;
#endif
    if ( !isdebug )
	return;

    BufferString msg( cname, " | " );
    msg.add( fname ).add( ":" ).add( linenr )
	.add( " >> " ).add( inpmsg ).add( " <<" );
    ErrMsg( msg.buf(), true );
}


} // namespace OD



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


void ErrMsg( const char* msg, bool isprogrammer )
{
    if ( !MsgClass::theCB().willCall() )
    {
	mDefineStaticLocalObject( bool, wantsilence,
			= GetEnvVarYN("OD_PROGRAMMER_ERRS_SILENCE",false) );
	if ( isprogrammer )
	{
	    if ( !wantsilence )
		std::cerr << "(PE) " << msg << std::endl;
	}
	else if ( msg && *msg )
	{
	    const char* start = *msg == '[' ? "" : "Err: ";
	    static int usestdout = -1;
	    if ( usestdout == -1 )
	    {
		const StringView strmfnm( OD::logMsgStrm().fileName() );
		usestdout = !strmfnm.isEmpty() &&
			     strmfnm != od_stream::sStdIO() ? 1 : 0;
	    }

	    od_ostream& strm = usestdout==1 ? OD::logMsgStrm() : od_cerr();
	    strm << start  << msg << od_endl;
	}
    }
    else
    {
	ErrMsgClass obj( msg, isprogrammer );
	MsgClass::theCB().doCall( &obj );
    }

    if ( isprogrammer && crash_on_programmer_error_ )
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



#ifdef HAS_BREAKPAD
#define mUseCrashDumper
#endif

namespace System
{

CrashDumper::CrashDumper()
{
    init();
}


CrashDumper* CrashDumper::theinst_ = nullptr;


static Threads::Atomic<int> dumpsent( 0 );

#ifdef mUseCrashDumper
static uiString* legalText();
static const char* breakpadname = "Google Breakpad";
# ifdef __win__
static bool MinidumpCB( const wchar_t* dump_path, const wchar_t* id,
			void* context, EXCEPTION_POINTERS *exinfo,
			MDRawAssertionInfo *assertion, bool succeeded )
{
    if ( !dumpsent.setIfValueIs(0,1,nullptr) )
	return succeeded;

    const BufferString dmppath( QString::fromWCharArray(dump_path) );
    const BufferString dmpid( QString::fromWCharArray(id) );
    FilePath dmpfp( dmppath, dmpid );
    dmpfp.setExtension( "dmp" );
    CrashDumper::getInstance().sendDump( dmpfp.fullPath() );
    return succeeded;
}
# elif __lux__
static bool MinidumpCB( const google_breakpad::MinidumpDescriptor& minidumpdesc,
    void* context, bool succeeded )
{
    if ( !dumpsent.setIfValueIs( 0, 1, nullptr ) )
	return succeeded;

    FilePath dmpfp( minidumpdesc.path() );
    System::CrashDumper::getInstance().sendDump( dmpfp.fullPath() );
    return succeeded;
}
# endif
#endif

void CrashDumper::init()
{
    if ( handler_ )
	return;

#ifdef mUseCrashDumper
# ifdef __win__
    const QString dmppath = FilePath::getTempDir();
    const std::wstring wpath = dmppath.toStdWString();
    handler_ = new google_breakpad::ExceptionHandler(
		    wpath, NULL, MinidumpCB, NULL,
		    google_breakpad::ExceptionHandler::HANDLER_ALL );
    legalInformation().addCreator( legalText, breakpadname );
# elif __lux__
    const BufferString dmppathbuf = FilePath::getTempDir();
    const google_breakpad::MinidumpDescriptor minidumpdesc( dmppathbuf.buf() );
    handler_ = new google_breakpad::ExceptionHandler(
			minidumpdesc, NULL, MinidumpCB, NULL, true, -1 );
    legalInformation().addCreator( legalText, breakpadname );
# endif
#endif
}


void CrashDumper::sendDump( const char* filename )
{
    if ( sendappl_.isEmpty() || !File::exists(filename) )
	return;

    BufferString newfilename = FilePath( GetArgV()[0] ).baseName();
    newfilename.add( "_error_report.dmp" );
    FilePath newfp( filename );
    newfp.setFileName( newfilename );
    newfilename = newfp.fullPath();
    if ( File::exists(newfilename) )
	File::remove( newfilename );

    if ( File::rename(filename,newfilename) )
	filename = newfilename.buf();

    OS::MachineCommand mc( sendappl_ );
    mc.addKeyedArg( sKeyDumpFile(), filename );
    mc.addFlag( sKey::Binary() );
    mc.execute( OS::RunInBG );
}


CrashDumper& CrashDumper::getInstance()
{
    if ( !theinst_ )
    {
	theinst_ = new CrashDumper;

#ifdef mUseCrashDumper
	const char* crashspec = GetEnvVar( "DTECT_FORCE_IMMEDIATE_DUMP" );
	if ( crashspec && *crashspec )
	{
	    DBG::forceCrash( false );
	}
#endif
    }

    return *theinst_;
}


#ifdef mUseCrashDumper
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
#endif

} // namespace System
