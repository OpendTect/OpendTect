#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Jun 2006
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstring.h"
#include "gendefs.h"
#include "uistrings.h"

#ifdef __win__
#   include "windows.h"
    typedef HMODULE Handletype;
#else
    typedef void* Handletype;
#endif


/*!
\brief Gives access to shared libs on runtime. Plugins should be loaded via 
 the Plugin Manager (see plugins.h).
*/

mExpClass(Basic) SharedLibAccess
{
public:

		SharedLibAccess(const char* file_name);
		//!< handle is only closed if you do it explicitly.
    bool	isOK() const		{ return handle_; }
    const uiString&  errMsg() const		{ return errmsg_; }

    void	close();

    void*	getFunction(const char* function_name) const;
		//!< Difficult for C++ functions as the names are mangled.

    Handletype	handle()		{ return handle_; }

    static void	getLibName(const char* modnm,char*);
		//!< returns lib name with ".dll" or "lib" and ".so"/".dylib"
		//!< output can be up to 255 chars long (guaranteed maximum)

protected:

    Handletype		handle_;
    uiString		errmsg_;

};
