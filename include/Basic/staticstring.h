#pragma once

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris/Bert
 Date:		Jan 2014/Jul 2017
________________________________________________________________________
-*/


#include "basicmod.h"
#include "perthreadrepos.h"

typedef PerThreadObjectRepository<BufferString> StaticStringManager;

#define _mDeclStaticString(nm) \
    mDefineStaticLocalObject( StaticStringManager, nm##_ssm, \
			      = StaticStringManager() ) \
    BufferString& nm = nm##_ssm.getObject()

#ifdef __debug__
#define mDeclStaticString(nm) \
    _mDeclStaticString(nm); \
    addToStaticStringRepos( &nm )
#else
#define mDeclStaticString(nm) \
    _mDeclStaticString(nm)
#endif

/*!For Debugging only. Keeps a list of static strings and can tell
   if a particular string is a static string or not. */

mGlobal(Basic) void addToStaticStringRepos(const OD::String*);
mGlobal(Basic) bool isStaticString(const OD::String*);
