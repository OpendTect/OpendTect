#ifndef sharedlibs_h
#define sharedlibs_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Jun 2006
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "gendefs.h"
#ifdef __win__
#   include "windows.h"
    typedef HMODULE Handletype;
#else
    typedef void* Handletype;
#endif


/*!
\ingroup Basic
\brief Gives access to shared libs on runtime. Plugins should be loaded via the Plugin Manager (see plugins.h).
*/

mClass(Basic) SharedLibAccess
{
public:

    		SharedLibAccess(const char* file_name);
		//!< handle is only closed if you do it explicitly.
    bool	isOK() const		{ return handle_; }

    void	close();

    void*	getFunction(const char* function_name) const;
    		//!< Difficult for C++ functions as the names are mangled.

    Handletype	handle()		{ return handle_; }

    static void	getLibName(const char* modnm,char*);
    		//!< returns lib name with ".dll" or "lib" and ".so"/".dylib"

protected:

    Handletype	handle_;

};


#endif

