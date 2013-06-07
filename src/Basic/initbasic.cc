/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "moddepmgr.h"

#define OD_EXT_KEYSTR_EXPAND 1

#define mKeyStrsNameSpace(ns) namespace ns
#ifdef __msvc__
# define mKeyStrsDecl(nm,str) mBasicExtern FixedString nm = str
#else
# define mKeyStrsDecl(nm,str) FixedString nm = str
#endif

#include "keystrs.h"

mDefModInitFn(Basic)
{
    mIfNotFirstTime( return );

#undef keystrs_h
# define mKeyStrsNameSpace(ns) /* empty */
# define mKeyStrsDecl(nm,str) \
        static const char* backup_str_for_##nm = str; \
        sKey::nm = backup_str_for_##nm

#include "keystrs.h"

}
