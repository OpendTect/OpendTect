#ifndef errh_H
#define errh_H

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		19-10-1995
 Contents:	Error handler
 RCS:		$Id: errh.h,v 1.10 2003-11-10 14:35:07 arend Exp $
________________________________________________________________________

*/

#include <msgh.h>
#include <bufstring.h>

/*!\brief MsgClass holding an error message.

Programmer errors are only outputed when printProgrammerErrs is true. This
is set to true by default only if __debug__ is defined.

*/


class ErrMsgClass : public MsgClass
{
public:

			ErrMsgClass( const char* s, bool p )
			: MsgClass(s,p?ProgrammerError:Error)	{}

    static bool		printProgrammerErrs;

};


inline void ErrMsg( const char* msg, bool progr = false )
{
    if ( !ErrMsgClass::printProgrammerErrs && progr ) return;

    if ( !MsgClass::theCB().willCall() )
	cerr << (progr?"(PE) ":"") << msg << endl;
    else
    {
	ErrMsgClass obj( msg, progr );
	MsgClass::theCB().doCall( &obj );
    }
}


inline void programmerErrMsg( const char* msg, const char* cname,
				const char* fname, int linenr )
{
    BufferString str( cname );
    str += "-";
    str += fname;
    str += "/";
    str += linenr;
    str += ": ";
    str += msg;

    ErrMsg( str, true );
}

#ifdef __debug__
# define pErrMsg(msg) programmerErrMsg(msg,::className(*this),__FILE__,__LINE__)
//!< Usual access point for programmer error messages
# define pFreeFnErrMsg(msg,fn) programmerErrMsg( msg, fn, __FILE__, __LINE__ )
//!< Usual access point for programmer error messages in free functions
#else
# define pErrMsg(msg)
# define pFreeFnErrMsg(msg,fn)
#endif

#endif
