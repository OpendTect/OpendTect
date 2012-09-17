/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          June 2003
 RCS:           $Id: debug.cc,v 1.34 2010/10/14 09:58:06 cvsbert Exp $
________________________________________________________________________

-*/

static const char* rcsID = "$Id: debug.cc,v 1.34 2010/10/14 09:58:06 cvsbert Exp $";

#include "debug.h"
#include "debugmasks.h"
#include "genc.h"
#include "bufstring.h"
#include "timefun.h"
#include "filepath.h"
#include "envvars.h"
#include "oddirs.h"
#include "sighndl.h"
#include "undefval.h"
#include "math2.h"
#include "errh.h"
#include "odplatform.h"

#include <iostream>
#include <fstream>

static std::ostream* dbglogstrm = 0;

static bool doisudfmsgs = GetEnvVarYN( "OD_SHOW_NOT_NORMAL_NUMBER_MSGS" );
bool dbgIsUdf( float val )
{
    if ( !Math::IsNormalNumber(val) )
	{ if ( doisudfmsgs ) pFreeFnErrMsg("Bad fp value found","dbgIsUdf(f)");
			     return true; }
    return Values::isUdf( val );
}
bool dbgIsUdf( double val )
{
    if ( !Math::IsNormalNumber(val) )
	{ if ( doisudfmsgs ) pFreeFnErrMsg("Bad fp value found","dbgIsUdf(d)");
			     return true; }
    return Values::isUdf( val );
}


namespace DBG
{

static int getMask()
{
    static bool maskgot = false;
    static int themask = 0;
    if ( maskgot ) return themask;
    maskgot = true;

    BufferString envmask = GetEnvVar( "DTECT_DEBUG" );
    const char* buf = envmask.buf();
    themask = toInt( buf );
    if ( buf[0] == 'y' || buf[0] == 'Y' ) themask = 0xffff;

    const char* dbglogfnm = GetEnvVar( "DTECT_DEBUG_LOGFILE" );
    if ( dbglogfnm && !themask )
	themask = 0xffff;

    if ( themask )
    {
	BufferString msg;
	if ( dbglogfnm )
	{
	    dbglogstrm = new std::ofstream( dbglogfnm );
	    if ( dbglogstrm && !dbglogstrm->good() )
	    {
		delete dbglogstrm; dbglogstrm = 0;
		msg = "Cannot open debug log file '";
		msg += dbglogfnm; msg == "': reverting to stdout";
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
	SignalHandling::theinst_.doStop( 6, false ); // 6 = SIGABRT
    else
	{ char* ptr = 0; *ptr = 0; }
}


void message( const char* msg )
{
    if ( !isOn() ) return;

    BufferString msg_;
    static bool wantpid = GetEnvVarYN("DTECT_ADD_DBG_PID");
    if ( wantpid )
    {
	msg_ = "[";
	msg_ += GetPID();
	msg_ += "] ";
    }
    msg_ += msg;

    if ( dbglogstrm )
	*dbglogstrm << msg_ << std::endl;
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

};


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
