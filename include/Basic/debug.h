#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "gendefs.h"
#include "bufstring.h"

namespace google_breakpad { class ExceptionHandler; }

/*!\brief Defines a generic interface for supplying debug/runtime info.

    The isOn() is controlled by the environment variable DTECT_DEBUG.
    If DTECT_DEBUG starts with a "Y" or "y" then the mask is set to 0xffff.
    isOn returns the bitwise "and" of the passed flag and the internal
    environment mask, converted to a boolean.

    The reserved/defined masks are defined below.
    if you want the output in a file, set the full path in DTECT_DEBUG_LOGFILE.

*/

namespace DBG
{
    mGlobal(Basic) void	turnOn( int flag );
    //!<Overrides the envirnonment variable
    mGlobal(Basic) bool isOn( int flag=0xffff );
    mGlobal(Basic) void message(const char*);	  //!< default: to stderr
    mGlobal(Basic) void message(const char*, const char* cname,
			        const char* fname, int linrnr);
    mGlobal(Basic) void message(int flag, const char* msg);
    mGlobal(Basic) void message(int flag, const char*, const char* cname,
				const char* fname, int linrnr);
    mGlobal(Basic) void putProgInfo(int,char**);
			//!< one line; more if isOn()
    mGlobal(Basic) void forceCrash(bool withdump);
    mGlobal(Basic) bool crashOnNaN();
    mGlobal(Basic) bool setCrashOnProgError(bool yn);
			//!<Returns old status
};

extern "C" {

    mGlobal(Basic) int od_debug_isOn( int flag );
    mGlobal(Basic) void od_debug_message( const char* msg );
    mGlobal(Basic) void od_debug_messagef( int flag, const char* msg );
    mGlobal(Basic) void od_debug_putProgInfo(int,char**);
    mGlobal(Basic) void od_putProgInfo(int,char**);
    mGlobal(Basic) void od_init_test_program(int,char**,
					     bool withdataroot=false);
	/*!<Calls SetProgramArgs, sets crash on programmer error, and installs
	    signal handling for crashes. */
}

# define pDebugMsg(msg) \
    DBG::message(msg,::className(*this),__FILE__,__LINE__)
# define pFreeDebugMsg(msg) \
    DBG::message(msg,__func__,__FILE__,__LINE__)
# define pFDebugMsg(flag,msg) \
    DBG::message(flag,msg,::className(*this),__FILE__,__LINE__)
# define pFreeFDebugMsg(flag,msg) \
    DBG::message(flag,msg,__func__,__FILE__,__LINE__)


namespace System
{

/*!Segmentation fault core dumper that sends dump to dGB. */

mExpClass(Basic) CrashDumper
{
public:
    static CrashDumper& getInstance();
			//!Creates and installs at first run.

    bool		isOK() const { return handler_; }

    void		sendDump(const char* filename);

    void		setSendAppl( const char* a )    { sendappl_ = a; }

    static const char*	sKeyDumpFile()	{ return "dumpfile"; }

private:
			CrashDumper();

    void		init();

    static CrashDumper* theinst_;

    BufferString	sendappl_;
    google_breakpad::ExceptionHandler*	handler_ = nullptr;
};

} // namespace System



/*
    This is a list of reserved debug masks, in order to avoid conflicts.
    Don't just throw any of these away or change a value, adding new masks
    should be OK.
*/

#define	DBG_DBG		0x0001	// general, low frequency stuff
#define	DBG_MT		0x0002	// multi-threaded stuff
#define	DBG_UI		0x0004	// ui-related stuff
#define	DBG_IO		0x0008	// general I/O stuff
#define	DBG_SOCKIO	0x0010	// socket I/O
#define	DBG_MM		0x0020	// Multi-machine batch processing
#define	DBG_SETTINGS	0x0040	// User settings
#define	DBG_PROGSTART	0x0080	// Program start and stop
#define	DBG_FILEPATH	0x0100	// File name handling, conversion, etc.
#define	DGB_SERVICES	0x0200	// Network service managers
