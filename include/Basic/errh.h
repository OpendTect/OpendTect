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
#include "errmsg.h"
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


mGlobal(Basic) void SetCrashOnProgrammerError(int yn);
//!<Default is off. Normally only turned on in test-programs.


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

#endif

#endif

