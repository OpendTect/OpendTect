#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstring.h"
#include "gendefs.h"

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
    const char*	errMsg() const		{ return errmsg_.buf(); }

    void	close();

    void*	getFunction(const char* function_name) const;
		//!< Difficult for C++ functions as the names are mangled.

    Handletype	handle()		{ return handle_; }

    static void	getLibName(const char* modnm,char*,int sz);
		//!< returns lib name with ".dll" or "lib" and ".so"/".dylib"

protected:

    Handletype		handle_;
    BufferString	errmsg_;

};


/*!
\brief Gives access to any runtime library (DLL/so),
 from a full filepath.
 The library does thus no need to be inside the current path.
 This library stays loaded until the object is destroyed.
*/

mExpClass(Basic) RuntimeLibLoader
{
public:
		    RuntimeLibLoader(const char* libfilenm,
				     const char* subdir=nullptr);
		    ~RuntimeLibLoader();

    bool	    isOK() const;

private:

    SharedLibAccess* sha_ = nullptr;

};
