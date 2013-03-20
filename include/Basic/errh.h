#ifndef errh_h
#define errh_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		19-10-1995
 Contents:	Error handler
 RCS:		$Id$
________________________________________________________________________

*/

#include "basicmod.h"
#include "msgh.h"
#include "bufstring.h"
#include "fixedstring.h"



namespace google_breakpad { class ExceptionHandler; }

/*!
\brief MsgClass holding an error message.
  
  Programmer errors are only outputed when printProgrammerErrs is true. This
  is set to true by default only if __debug__ is defined.
*/

mExpClass(Basic) ErrMsgClass : public MsgClass
{
public:

			ErrMsgClass( const char* s, bool p )
			: MsgClass(s,p?ProgrammerError:Error)	{}

    static bool		printProgrammerErrs;

};


mGlobal(Basic) void ErrMsg(const char*,bool progr=false);


inline void programmerErrMsg( const char* msg, const char* cname,
				const char* fname, int linenr )
{
    BufferString str( cname );
    str += " | "; str += fname;
    str += ":"; str += linenr;
    str += " | "; str += msg;
    ErrMsg( str.buf(), true );
}



#ifdef __debug__
# define pErrMsg(msg) programmerErrMsg(msg,::className(*this),__FILE__,__LINE__)
//!< Usual access point for programmer error messages
# define pFreeFnErrMsg(msg,fn) programmerErrMsg( msg, fn, __FILE__, __LINE__ )
//!< Usual access point for programmer error messages in free functions
#else
# define pErrMsg(msg)			EmptyFunction()
# define pFreeFnErrMsg(msg,fn)		EmptyFunction()
#endif

#if defined ( __msvc__ )  && defined ( HAS_BREAKPAD )
# define mUseCrashDumper

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

}; //Namespace System

#endif mUseCrashDumper

#endif

