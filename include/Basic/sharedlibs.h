#ifndef sharedlibs_h
#define sharedlibs_h
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Jun 2006
 RCS:		$Id: sharedlibs.h,v 1.3 2006-07-03 16:38:15 cvsbert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
#ifdef __win__
#   include "windows.h"
    typedef HMODULE Handletype;
#else
    typedef void* Handletype;
#endif


/* Gives access to shared libs on runtime.
   Plugins should be loaded via the Plugin Manager (see plugins.h).
   */


class SharedLibAccess
{
public:
    		SharedLibAccess(const char* file_name);
		//!< handle is only closed if you do it explicitly.
    bool	isOK() const		{ return handle_; }

    void	close();

    void*	getFunction(const char* function_name);
    		//!< Difficult for C++ functions as the names are mangled.

    Handletype	handle()		{ return handle_; }

protected:

    Handletype	handle_;

};


#endif
